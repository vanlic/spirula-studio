#include "data/LccPartition.h"

#include "data/PointCloudFile.h"
#include "data/RegionMesh.h"
#include "data/SceneProjection.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <limits>
#include <map>
#include <queue>
#include <random>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace spirula {

std::array<int, 2> lcc_raster_dimensions(int width, int height) {
    if (width <= 0 || height <= 0) return {0, 0};
    const double scale = double(kLccRasterSize) / std::max(width, height);
    return {std::clamp((int)std::ceil(width * scale), 1, kLccRasterSize),
            std::clamp((int)std::ceil(height * scale), 1, kLccRasterSize)};
}

namespace {

constexpr double kCoverage = 0.03;
constexpr double kGiPixel = 1073741824.0;
constexpr int64_t kMinPercentileSeeds = 101;  // One sample per 1% tail with the rank based on n - 1.
constexpr double kLidarLasFraction = 0.03;
constexpr double kLidarPlyFraction = 0.2;

struct Voxel {
    std::array<double, 3> center;
    double load = 0;
};

struct ImageCoverage {
    std::vector<std::pair<int, int>> cells;
    int cols = 0, rows = 0;
    int valid_pixels = 0;
    double cell_load = 0;
};

struct SeedRows {
    int64_t count;
    std::vector<int64_t> kept;

    int64_t operator[](int64_t i) const { return kept.empty() ? i : kept[(size_t)i]; }
};

void check_cancel(const std::atomic<bool>* cancel) {
    if (cancel && cancel->load(std::memory_order_relaxed)) throw PartitionCancelled();
}

std::vector<double> sample_lidar(const ParsedDataset& ds, const PartitionOptions& opt,
                                 const std::vector<double>& full,
                                 const PartitionLog& log, const std::atomic<bool>* cancel) {
    check_cancel(cancel);
    std::unique_ptr<cloud::Reader> reader;
    if (full.empty()) reader = std::make_unique<cloud::Reader>(opt.lcc_lidar_cloud);
    std::string ext = std::filesystem::u8path(opt.lcc_lidar_cloud).extension().u8string();
    for (char& ch : ext) ch = (char)std::tolower((unsigned char)ch);
    const double fraction = opt.lcc_lidar_sample_fraction > 0 ? opt.lcc_lidar_sample_fraction :
        (ext == ".ply" ? kLidarPlyFraction : kLidarLasFraction);
    const uint64_t limit = (uint64_t)opt.max_projected_points;
    uint64_t pcg_state = 0x853c49e6748fea9bULL;
    std::mt19937_64 reservoir(42);
    uint64_t selected = 0, valid = 0;
    std::vector<double> xyz;
    const uint64_t count = reader ? (uint64_t)std::max<int64_t>(0, reader->info().points) : full.size() / 3;
    xyz.reserve((size_t)std::min(limit, count) * 3);
    const auto add = [&](const double* point) {
        const double q[] = {point[0] - ds.center[0], point[1] - ds.center[1], point[2] - ds.center[2]};
        if (!std::all_of(q, q + 3, [](double v) { return std::isfinite(v); })) return;
        ++valid;
        const uint64_t old = pcg_state;
        pcg_state = old * 0x5851f42d4c957f2dULL + 0xda3e39cb94b95bdbULL;
        const uint32_t bits = (uint32_t)(((old >> 18) ^ old) >> 27);
        const unsigned rot = (unsigned)(old >> 59);
        const uint32_t draw = (bits >> rot) | (bits << ((32 - rot) % 32));
        if ((double)draw / 4294967296.0 >= fraction) return;
        ++selected;
        if (selected <= limit) xyz.insert(xyz.end(), q, q + 3);
        else {
            const uint64_t row = std::uniform_int_distribution<uint64_t>(0, selected - 1)(reservoir);
            if (row < limit) std::copy(q, q + 3, xyz.data() + (size_t)row * 3);
        }
    };
    if (reader) {
        const bool finished = reader->read([&](const e57::Point* points, size_t n) {
            for (size_t i = 0; i < n; ++i) {
                if (i % 4096 == 0) check_cancel(cancel);
                add(points[i].xyz);
            }
        }, cancel);
        if (!finished) throw PartitionCancelled();
    } else {
        for (size_t i = 0; i < count; ++i) {
            if (i % 4096 == 0) check_cancel(cancel);
            add(full.data() + i * 3);
        }
    }
    if (xyz.empty()) throw std::runtime_error("partition: no LiDAR points sampled; check the scan or raise the sample fraction");
    if (log) log("LCC: sampled " + std::to_string(xyz.size() / 3) + " of " + std::to_string(valid) +
                 " LiDAR points for voxelization");
    return xyz;
}

Aabb bound_points(const double* xyz, SeedRows& rows, bool robust, const char* name,
                  const PartitionLog& log, const std::atomic<bool>* cancel) {
    Aabb bounds;
    for (int64_t i = 0; i < rows.count; ++i) {
        if (i % 4096 == 0) check_cancel(cancel);
        const double* q = &xyz[(size_t)rows[i] * 3];
        if (!std::all_of(q, q + 3, [](double v) { return std::isfinite(v); }))
            throw std::runtime_error("partition: LCC requires finite point coordinates");
        bounds.expand(q);
    }
    if (!robust || rows.count == 0) return bounds;
    if (rows.count < kMinPercentileSeeds) {
        if (log) log(std::string("LCC: robust bounds kept all ") + name + " in a cloud with fewer than 101 points");
        return bounds;
    }

    check_cancel(cancel);
    const Aabb clip = robust_bounds(xyz, rows.count, rows.kept.empty() ? nullptr : rows.kept.data());
    SeedRows clipped{0, {}};
    clipped.kept.reserve((size_t)rows.count);
    bounds = {};
    for (int64_t i = 0; i < rows.count; ++i) {
        if (i % 4096 == 0) check_cancel(cancel);
        const int64_t row = rows[i];
        const double* q = &xyz[(size_t)row * 3];
        if (!clip.contains(q)) continue;
        clipped.kept.push_back(row);
        bounds.expand(q);
    }
    clipped.count = (int64_t)clipped.kept.size();
    if (log) log("LCC: kept " + std::to_string(clipped.count) + " of " + std::to_string(rows.count) +
                 " " + name + " within robust bounds");
    if (clipped.count == 0)
        throw std::runtime_error(std::string("partition: LCC robust bounds removed all ") + name);
    rows = std::move(clipped);
    return bounds;
}

std::vector<Voxel> make_voxels(const double* xyz, const SeedRows& seeds, const double origin[3],
                              double size, int limit, const std::atomic<bool>* cancel) {
    const int64_t stride = 1 + (seeds.count - 1) / limit;
    std::map<std::array<int64_t, 3>, Voxel> grid;
    for (int64_t i = 0; i < seeds.count; i += stride) {
        if ((i / stride) % 4096 == 0) check_cancel(cancel);
        const double* q = &xyz[(size_t)seeds[i] * 3];
        std::array<int64_t, 3> cell;
        Voxel voxel;
        for (int axis = 0; axis < 3; ++axis) {
            const double coordinate = (q[axis] - origin[axis]) / size;
            if (std::abs(coordinate) >= double(int64_t(1) << 62))
                throw std::runtime_error("partition: voxel size is too small for the scene");
            cell[axis] = (int64_t)std::floor(coordinate);
            voxel.center[axis] = origin[axis] + (double(cell[axis]) + 0.5) * size;
        }
        grid.try_emplace(cell, voxel);
    }
    std::vector<Voxel> voxels;
    voxels.reserve(grid.size());
    for (const auto& entry : grid) voxels.push_back(entry.second);
    return voxels;
}

ImageCoverage rasterize(const FrameView& view, const std::vector<Voxel>& voxels, double size,
                        std::vector<int>& counts, const std::atomic<bool>* cancel,
                        const LccRasterMask* mask,
                        std::vector<int>* pixel_voxel = nullptr) {
    const int w = view.cam.width, h = view.cam.height;
    ImageCoverage result;
    const auto [cols, rows] = lcc_raster_dimensions(w, h);
    if (cols == 0) return result;
    result.cols = cols;
    result.rows = rows;
    const int area = cols * rows;
    result.valid_pixels = mask ? (int)mask->count() : area;
    if (result.valid_pixels == 0) {
        if (pixel_voxel) pixel_voxel->assign((size_t)area, -1);
        return result;
    }
    // A kept raster cell represents the same number of source pixels regardless of the mask.
    result.cell_load = 1.15 * double(w) * h / (kGiPixel * area);
    const double scale = double(kLccRasterSize) / std::max(w, h);
    const double focal = std::max(std::abs(view.cam.fx), std::abs(view.cam.fy));
    const bool wrap = camhost::full_longitude(view.cam);
    std::vector<double> depth((size_t)area, std::numeric_limits<double>::infinity());
    std::vector<int> owner(depth.size(), -1);
    for (size_t i = 0; i < voxels.size(); ++i) {
        if (i % 4096 == 0 && cancel && cancel->load(std::memory_order_relaxed)) return {};
        double xy[2], distance;
        if (!frame_sees(view, voxels[i].center.data(), xy, &distance)) continue;
        const double u = xy[0] * cols / w, v = xy[1] * rows / h;
        const int x = std::clamp((int)u, 0, cols - 1), y = std::clamp((int)v, 0, rows - 1);
        const double radius = std::clamp(0.5 * focal * size * scale / distance, 0.75, 4.0);
        const int reach = (int)std::ceil(radius);
        const int x0 = wrap ? x - reach : std::max(0, x - reach);
        const int x1 = wrap ? x + reach : std::min(cols - 1, x + reach);
        for (int row = std::max(0, y - reach); row <= std::min(rows - 1, y + reach); ++row)
            for (int col = x0; col <= x1; ++col) {
                const int px = wrap ? (col % cols + cols) % cols : col;
                const size_t at = (size_t)row * cols + px;
                if (mask && !(*mask)[at]) continue;
                const double dx = col + 0.5 - u, dy = row + 0.5 - v;
                if (distance < depth[at] &&
                    ((row == y && col == x) || dx * dx + dy * dy <= radius * radius)) {
                    depth[at] = distance;
                    owner[at] = (int)i;
                }
            }
    }
    for (int i : owner)
        if (i >= 0 && counts[(size_t)i]++ == 0) result.cells.emplace_back(i, 0);
    for (auto& [i, count] : result.cells) {
        count = counts[(size_t)i];
        counts[(size_t)i] = 0;
    }
    if (pixel_voxel) *pixel_voxel = std::move(owner);
    return result;
}

struct AxisLoads {
    std::vector<std::pair<double, double>> prefix;

    void accumulate() {
        std::sort(prefix.begin(), prefix.end());
        double sum = 0;
        size_t distinct = 0;
        for (const auto item : prefix) {
            sum += item.second;
            if (distinct && prefix[distinct - 1].first == item.first)
                prefix[distinct - 1].second = sum;
            else
                prefix[distinct++] = {item.first, sum};
        }
        prefix.resize(distinct);
    }
    double total() const { return prefix.empty() ? 0 : prefix.back().second; }
    size_t quantile(double fraction) const {
        const auto it = std::lower_bound(prefix.begin(), prefix.end(), fraction * total(),
                                         [](const auto& item, double weight) { return item.second < weight; });
        return std::min((size_t)(it - prefix.begin()), prefix.size() - 1);
    }
    double before(double coordinate) const {
        const auto it = std::lower_bound(prefix.begin(), prefix.end(), coordinate,
                                         [](const auto& item, double value) { return item.first < value; });
        return it == prefix.begin() ? 0 : (it - 1)->second;
    }
};

double score_cut(double parent, double left, double right, double target) {
    const auto improvement = [&](double load) {
        return load > target ? std::min(1.0, 0.1 + 10.0 * (parent - load) / parent) : 1.0;
    };
    const auto balance = [&](double load) {
        const double ratio = load / target;
        return std::max(0.001, ratio < 1.0 ? ratio : std::min(1.0, 0.4 * ratio - 0.3));
    };
    const double waste = std::max(0.001, std::max(0.0, left + right - parent) + 0.05 * target);
    return -waste / std::max(0.001, std::min(left, right))
                  / improvement(left) / improvement(right) / balance(left) / balance(right);
}

bool split_block(const SpatialPart& parent, const std::vector<Voxel>& voxels, double voxel_size,
                 double overlap_limit, double target, SpatialPart& left, SpatialPart& right,
                 const std::atomic<bool>* cancel) {
    std::array<AxisLoads, 3> core;
    for (const Voxel& voxel : voxels)
        if (voxel.load > 0 && parent.owns(voxel.center.data()))
            for (int axis = 0; axis < 3; ++axis)
                core[axis].prefix.emplace_back(voxel.center[axis], voxel.load);
    if (core[0].prefix.size() < 2) return false;
    double span[3];
    std::array<int, 3> axes{0, 1, 2};
    for (int axis : axes) {
        core[axis].accumulate();
        span[axis] = core[axis].prefix[core[axis].quantile(0.95)].first -
                     core[axis].prefix[core[axis].quantile(0.05)].first;
    }
    std::stable_sort(axes.begin(), axes.end(), [&](int a, int b) { return span[a] > span[b]; });
    double overlap = std::min(overlap_limit, std::max(span[axes.front()] * 0.05, 3.0 * voxel_size));
    constexpr int samples[] = {150, 100, 50};
    const auto find_cut = [&](double padding) {
        const Aabb expanded = SpatialPart{parent.core, padding}.expanded();
        const bool exhaustive = padding == 0;
        double best = -std::numeric_limits<double>::infinity();
        for (int rank = 0; rank < 3; ++rank) {
            check_cancel(cancel);
            const int axis = axes[(size_t)rank];
            const AxisLoads& inner = core[axis];
            if (inner.prefix.size() < 2) continue;
            AxisLoads outer;
            if (!exhaustive) {
                for (const Voxel& voxel : voxels)
                    if (voxel.load > 0 && expanded.contains(voxel.center.data()))
                        outer.prefix.emplace_back(voxel.center[axis], voxel.load);
                outer.accumulate();
            }
            double last = -std::numeric_limits<double>::infinity();
            const size_t count = exhaustive ? inner.prefix.size() - 1 : samples[rank];
            for (size_t i = 0; i < count; ++i) {
                if (i % 4096 == 0) check_cancel(cancel);
                const size_t j = exhaustive ? i : inner.quantile(double(i + 1) / (count + 1));
                if (j + 1 == inner.prefix.size()) continue;
                const double a = inner.prefix[j].first, b = inner.prefix[j + 1].first;
                const double cut = a * 0.5 + b * 0.5;
                if (cut <= a || cut >= b || cut <= last ||
                    cut <= parent.core.lo[axis] || cut >= parent.core.hi[axis]) continue;
                last = cut;
                const double l = exhaustive ? inner.prefix[j].second :
                    outer.before(std::nextafter(cut + padding, std::numeric_limits<double>::infinity()));
                const double r = exhaustive ? inner.total() - l :
                    outer.total() - outer.before(cut - padding);
                const double peak = std::max(l, r);
                if (!(l > 0 && r > 0 && peak < parent.gpixels)) continue;
                const double score = exhaustive ? -peak : score_cut(parent.gpixels, l, r, target);
                if (score <= best || !std::isfinite(score)) continue;
                best = score;
                left = {parent.core, padding, l};
                right = {parent.core, padding, r};
                left.core.hi[axis] = cut;
                right.core.lo[axis] = cut;
            }
        }
        return exhaustive ? std::isfinite(best) : best >= -10.0;
    };
    for (int attempt = 0; attempt < 4 && overlap > 0; ++attempt, overlap *= 0.5)
        if (find_cut(overlap)) return true;
    return find_cut(0);
}

bool trim_overlap(SpatialPart& block, const std::vector<Voxel>& voxels, double target,
                  const std::atomic<bool>* cancel) {
    if (!(block.overlap > 0)) return false;
    const auto load_at = [&](double overlap) {
        check_cancel(cancel);
        const Aabb box = SpatialPart{block.core, overlap}.expanded();
        double sum = 0;
        for (const Voxel& voxel : voxels)
            if (box.contains(voxel.center.data())) sum += voxel.load;
        return sum;
    };
    double lo = 0, hi = block.overlap, load = load_at(0);
    if (load > target) return false;
    for (int i = 0; i < 12; ++i) {
        const double mid = lo * 0.5 + hi * 0.5;
        const double candidate = load_at(mid);
        if (candidate > target) hi = mid;
        else { lo = mid; load = candidate; }
    }
    block.overlap = lo;
    block.gpixels = load;
    return true;
}

std::vector<SpatialPart> split_space(const std::vector<Voxel>& voxels, SpatialPart root, double size,
                                     const PartitionOptions& opt, const std::atomic<bool>* cancel) {
    const auto lower_priority = [](const SpatialPart& a, const SpatialPart& b) {
        if (a.gpixels != b.gpixels) return a.gpixels < b.gpixels;
        return std::lexicographical_compare(b.core.lo, b.core.lo + 3, a.core.lo, a.core.lo + 3);
    };
    std::priority_queue<SpatialPart, std::vector<SpatialPart>, decltype(lower_priority)> queue(lower_priority);
    queue.push(root);
    const double cap = opt.lcc_overlap > 0 ? opt.lcc_overlap : std::numeric_limits<double>::infinity();
    while (queue.top().gpixels > opt.lcc_gpixels) {
        check_cancel(cancel);
        SpatialPart block = queue.top();
        const double overlap_limit = queue.size() == 1 ? cap : std::min(cap, block.overlap);
        SpatialPart left, right;
        if (split_block(block, voxels, size, overlap_limit, opt.lcc_gpixels, left, right, cancel)) {
            if (queue.size() >= 254) throw std::runtime_error("partition: more than 254 parts");
            queue.pop();
            queue.push(left);
            queue.push(right);
        } else if (trim_overlap(block, voxels, opt.lcc_gpixels, cancel)) {
            queue.pop();
            queue.push(block);
        } else {
            std::ostringstream error;
            error << "partition: LCC cannot split a " << block.gpixels << " GPixels block to fit "
                  << opt.lcc_gpixels << " GPixels at voxel size " << size;
            throw std::runtime_error(error.str());
        }
    }
    std::vector<SpatialPart> out;
    while (!queue.empty()) {
        out.push_back(queue.top());
        queue.pop();
    }
    return out;
}

void count_part_pixels(const ImageCoverage& image, const std::vector<std::vector<int>>& voxel_parts,
                       std::vector<int>& counts) {
    std::fill(counts.begin(), counts.end(), 0);
    for (const auto& [voxel, count] : image.cells)
        for (int part : voxel_parts[(size_t)voxel]) counts[(size_t)part] += count;
}

std::vector<std::vector<int>> assign_training_data(const ParsedDataset& ds, const SeedRows& seeds,
                                                   const std::vector<Voxel>& voxels,
                                                   const std::vector<ImageCoverage>& images, ScenePartition& p,
                                                   const std::atomic<bool>* cancel) {
    std::vector<Aabb> expanded;
    for (const SpatialPart& block : p.spatial_parts) expanded.push_back(block.expanded());
    std::vector<std::vector<int>> in_voxel(voxels.size());
    for (size_t i = 0; i < voxels.size(); ++i) {
        if (i % 4096 == 0) check_cancel(cancel);
        if (voxels[i].load <= 0) continue;
        for (size_t k = 0; k < expanded.size(); ++k)
            if (expanded[k].contains(voxels[i].center.data())) in_voxel[i].push_back((int)k);
    }
    const size_t n = expanded.size();
    p.frame_label.assign(images.size(), -1);
    p.ring.resize(n);
    p.part_points.resize(n);
    p.core_pieces.assign(n, 0);
    p.view_share.assign(n, 0.0f);
    std::vector<int> best_frame(n, -1), frame_count(n, 0), coverage(n);
    std::vector<double> best_share(n, 0), share_sum(n, 0);
    for (size_t c = 0; c < images.size(); ++c) {
        check_cancel(cancel);
        const ImageCoverage& image = images[c];
        if (image.cells.empty()) continue;
        count_part_pixels(image, in_voxel, coverage);
        const auto primary = std::max_element(coverage.begin(), coverage.end());
        if (*primary > kCoverage * image.valid_pixels) p.frame_label[c] = (int32_t)(primary - coverage.begin());
        for (size_t k = 0; k < n; ++k) {
            const double share = double(coverage[k]) / image.valid_pixels;
            if (share > best_share[k]) {
                best_share[k] = share;
                best_frame[k] = (int)c;
            }
            if (share <= kCoverage) continue;
            if (p.frame_label[c] != (int32_t)k) p.ring[k].push_back((int32_t)c);
            ++frame_count[k];
            share_sum[k] += share;
        }
    }
    for (size_t k = 0; k < n; ++k) {
        if (best_frame[k] < 0) throw std::runtime_error("partition: LCC block has no visible training image");
        if (frame_count[k] == 0) p.ring[k].push_back(best_frame[k]);
        p.view_share[k] = (float)(frame_count[k] ? share_sum[k] / frame_count[k] : best_share[k]);
    }
    p.point_label.assign((size_t)ds.points.num(), -1);
    for (int64_t i = 0; i < seeds.count; ++i) {
        if (i % 4096 == 0) check_cancel(cancel);
        const int64_t row = seeds[i];
        const double* q = &ds.points.xyz[(size_t)row * 3];
        for (size_t k = 0; k < n; ++k) {
            if (expanded[k].contains(q)) p.part_points[k].push_back(row);
            if (p.spatial_parts[k].owns(q)) p.point_label[(size_t)row] = (int32_t)k;
        }
    }
    return in_voxel;
}

}  // namespace

struct LccProjectionDebug::Data {
    std::vector<Voxel> voxels;
    std::vector<std::vector<int>> voxel_parts;
    LccProjectionMasks masks;
    double voxel_size;
    int num_parts;
};

LccFrameProjection LccProjectionDebug::project(const ParsedDataset& ds, int64_t camera,
                                                const std::atomic<bool>* cancel) const {
    if (!_data || camera < 0 || camera >= ds.num_cameras) return {};
    check_cancel(cancel);
    LccFrameProjection frame;
    std::vector<int> counts(_data->voxels.size(), 0);
    const auto& masks = _data->masks.frames;
    const auto* mask = masks.empty() ? nullptr : masks[(size_t)camera].get();
    const ImageCoverage image = rasterize(frame_view(ds, camera), _data->voxels, _data->voxel_size,
                                          counts, cancel, mask, &frame.pixel_voxel);
    check_cancel(cancel);
    frame.width = image.cols;
    frame.height = image.rows;
    frame.valid_pixels = image.valid_pixels;
    frame.part_pixels.resize((size_t)_data->num_parts);
    count_part_pixels(image, _data->voxel_parts, frame.part_pixels);
    return frame;
}

const std::vector<int>& LccProjectionDebug::voxel_parts(int voxel) const {
    return _data->voxel_parts[(size_t)voxel];
}

void LccLidarCache::prepare(const std::string& path) {
    const auto file = std::filesystem::u8path(path);
    const auto modified = std::filesystem::last_write_time(file);
    const auto size = std::filesystem::file_size(file);
    if (_path == path && _modified == modified && _size == size) return;
    *this = {};
    _path = path;
    _modified = modified;
    _size = size;
}

void LccLidarCache::retain_full(std::vector<double> xyz) {
    _full = std::move(xyz);
    _sample.clear();
}

const std::vector<double>& LccLidarCache::sample(const ParsedDataset& ds, const PartitionOptions& opt,
                                                const PartitionLog& log, const std::atomic<bool>* cancel) {
    check_cancel(cancel);
    prepare(opt.lcc_lidar_cloud);
    if (!_sample.empty() && _center == ds.center && _limit == opt.max_projected_points &&
        _fraction == opt.lcc_lidar_sample_fraction) {
        if (log) log("LCC: using cached LiDAR sample (" + std::to_string(_sample.size() / 3) + " points)");
        return _sample;
    }
    if (!_full.empty() && log) log("LCC: sampling LiDAR points from memory");
    _sample = sample_lidar(ds, opt, _full, log, cancel);
    _center = ds.center;
    _limit = opt.max_projected_points;
    _fraction = opt.lcc_lidar_sample_fraction;
    return _sample;
}

void partition_lcc(const ParsedDataset& ds, const SparseStats* tracks, ScenePartition& p,
                   const PartitionLog& log, const std::atomic<bool>* cancel,
                   LccLidarCache* lidar_cache, LccProjectionDebug* projection, const LccProjectionMasks* masks) {
    const PartitionOptions& opt = p.options;
    const bool use_lidar = !opt.lcc_lidar_cloud.empty();
    if (opt.parts != 0 || opt.max_projected_points < 1 || !std::isfinite(opt.lcc_gpixels) ||
        opt.lcc_gpixels <= 0 || !std::isfinite(opt.lcc_voxel_size) || opt.lcc_voxel_size < 0 ||
        !std::isfinite(opt.lcc_overlap) || opt.lcc_overlap < 0)
        throw std::runtime_error("partition: invalid LCC pixel budget, voxel size, overlap or part count");
    if (use_lidar && (!std::isfinite(opt.lcc_lidar_sample_fraction) || opt.lcc_lidar_sample_fraction < 0 ||
                      opt.lcc_lidar_sample_fraction > 1))
        throw std::runtime_error("partition: LCC LiDAR sample fraction must be between 0 and 1");
    if (!use_lidar && ds.points.num() == 0)
        throw std::runtime_error("partition: LCC needs a seed point cloud");
    if (opt.lcc_min_track_length < 0)
        throw std::runtime_error("partition: LCC minimum track length must be nonnegative");
    SeedRows seeds{ds.points.num(), {}};
    if (opt.lcc_min_track_length > 0) {
        if (!tracks || !tracks_usable(ds, *tracks))
            throw std::runtime_error("partition: LCC track filtering requires matching COLMAP tracks");
        const auto& begins = tracks->track_beg;
        for (int64_t i = 0; i < ds.points.num(); ++i) {
            if (i % 4096 == 0) check_cancel(cancel);
            if (begins[(size_t)i + 1] - begins[(size_t)i] >= opt.lcc_min_track_length)
                seeds.kept.push_back(i);
        }
        seeds.count = (int64_t)seeds.kept.size();
        if (log) log("LCC: kept " + std::to_string(seeds.count) + " of " +
                     std::to_string(ds.points.num()) + " seed points after track filtering");
        if (seeds.count == 0)
            throw std::runtime_error("partition: LCC track filtering removed all seed points");
    }
    const size_t n = (size_t)ds.num_cameras;
    const bool masked = masks && !masks->frames.empty();
    if (masked && masks->frames.size() != n) throw std::runtime_error("partition: LCC mask count mismatch");
    if (ds.image_filenames.size() != n || ds.widths.size() != n || ds.heights.size() != n || ds.intrins.size() != 4 * n ||
        ds.c2w.size() != 12 * n || (!ds.camera_models.empty() && ds.camera_models.size() != n) ||
        (!ds.camera_distortions.empty() && ds.camera_distortions.size() != n) ||
        (!ds.dist_coeffs.empty() && ds.dist_coeffs.size() != 8 * n))
        throw std::runtime_error("partition: LCC needs calibrated cameras");
    Aabb bounds = bound_points(ds.points.xyz.data(), seeds, opt.lcc_robust_bounds,
                               "seed points", log, cancel);
    SeedRows lidar_rows{0, {}};
    const double* lidar_xyz = nullptr;
    LccLidarCache local_cache;
    if (use_lidar) {
        const auto& lidar = (lidar_cache ? *lidar_cache : local_cache).sample(ds, opt, log, cancel);
        lidar_rows.count = (int64_t)(lidar.size() / 3);
        lidar_xyz = lidar.data();
        bounds = bound_points(lidar_xyz, lidar_rows, opt.lcc_robust_bounds, "LiDAR points", log, cancel);
    }
    const double extent = std::max({bounds.hi[0] - bounds.lo[0], bounds.hi[1] - bounds.lo[1],
                                    bounds.hi[2] - bounds.lo[2]});
    const double size = opt.lcc_voxel_size > 0 ? opt.lcc_voxel_size : extent / 256.0;
    if (!(size > 0) || !std::isfinite(extent) || !std::isfinite(extent + size))
        throw std::runtime_error("partition: LCC cannot determine a voxel size for this cloud");
    const double centered_origin[3] = {0, 0, 0};
    std::vector<Voxel> voxels = use_lidar
        ? make_voxels(lidar_xyz, lidar_rows, centered_origin, size, opt.max_projected_points, cancel)
        : make_voxels(ds.points.xyz.data(), seeds, bounds.lo, size, opt.max_projected_points, cancel);
    std::vector<ImageCoverage> images(n);
    if (log) log("LCC: projecting " + std::to_string(voxels.size()) + " voxels");
#pragma omp parallel
    {
        std::vector<int> counts(voxels.size(), 0);
#pragma omp for schedule(dynamic, 4)
        for (int64_t i = 0; i < ds.num_cameras; ++i) {
            if (cancel && cancel->load(std::memory_order_relaxed)) continue;
            const auto* mask = masked ? masks->frames[(size_t)i].get() : nullptr;
            images[(size_t)i] = rasterize(frame_view(ds, i), voxels, size, counts, cancel, mask);
        }
    }
    check_cancel(cancel);
    if (masked && std::none_of(images.begin(), images.end(),
                               [](const ImageCoverage& image) { return image.valid_pixels > 0; }))
        throw std::runtime_error("partition: LCC masks exclude all image pixels");
    SpatialPart root;
    root.core = bounds;
    for (int axis = 0; axis < 3; ++axis) {
        const double infinity = std::numeric_limits<double>::infinity();
        root.core.lo[axis] = std::nextafter(bounds.lo[axis] - size * 0.5, -infinity);
        root.core.hi[axis] = std::nextafter(bounds.hi[axis] + size * 0.5, infinity);
    }
    for (const ImageCoverage& image : images) {
        check_cancel(cancel);
        for (const auto& [voxel, count] : image.cells) {
            const double load = count * image.cell_load;
            voxels[(size_t)voxel].load += load;
            root.gpixels += load;
        }
    }
    if (root.gpixels <= 0)
        throw std::runtime_error(use_lidar ? "partition: no sampled LiDAR points are visible; check scan alignment and sampling"
                                           : "partition: LCC found no visible seed geometry");
    const auto heaviest = std::max_element(voxels.begin(), voxels.end(),
        [](const Voxel& a, const Voxel& b) { return a.load < b.load; });
    if (heaviest->load > opt.lcc_gpixels) {
        std::ostringstream error;
        error << "partition: one LCC voxel carries " << heaviest->load << " GPixels, above the "
              << opt.lcc_gpixels << " GPixels budget at voxel size " << size
              << ". Use a smaller voxel size.";
        throw std::runtime_error(error.str());
    }
    p.spatial_parts = split_space(voxels, root, size, opt, cancel);
    p.num_parts = (int)p.spatial_parts.size();
    p.source = CovisibilitySource::Projection;
    auto voxel_parts = assign_training_data(ds, seeds, voxels, images, p, cancel);
    for (SpatialPart& block : p.spatial_parts)
        for (int axis = 0; axis < 3; ++axis) {
            block.core.lo[axis] += ds.center[(size_t)axis];
            block.core.hi[axis] += ds.center[(size_t)axis];
        }
    if (log)
        for (int k = 0; k < p.num_parts; ++k)
            log("LCC part " + std::to_string(k) + ": " +
                std::to_string(p.spatial_parts[(size_t)k].gpixels) + " GPixels");
    if (projection)
        projection->_data = std::make_shared<LccProjectionDebug::Data>(
            LccProjectionDebug::Data{std::move(voxels), std::move(voxel_parts),
                                    masked ? *masks : LccProjectionMasks{}, size, p.num_parts});
}

}  // namespace spirula
