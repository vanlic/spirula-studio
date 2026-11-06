#include "data/PanoramaPartition.h"

#include "data/Knn.h"
#include "data/PartitionVisibility.h"
#include "sfm/geometry/LinAlg.h"

#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <map>
#include <numeric>
#include <queue>
#include <set>
#include <stdexcept>
#include <tuple>

namespace spirula {
namespace {

using Vec = std::array<double, 3>;
using sfm::Vec3;

double project(const Vec& axis, const double* p) {
    return axis[0] * p[0] + axis[1] * p[1] + axis[2] * p[2];
}

Vec as_array(Vec3 v) { return {v.x, v.y, v.z}; }
Vec3 as_vec(const Vec& v) { return {v[0], v[1], v[2]}; }

void stop_if(const std::atomic<bool>* cancel) {
    if (graph::cancelled(cancel)) throw PartitionCancelled();
}

struct Station {
    Vec center{};
    std::vector<int32_t> frames;
};

std::vector<Station> stations_of(const ParsedDataset& ds, const Covisibility& cov) {
    if (cov.frame_station.empty() || cov.frame_station.size() != (size_t)ds.num_cameras)
        throw std::runtime_error("axis-aligned partition: missing panorama station mapping");
    const auto bounds = std::minmax_element(cov.frame_station.begin(), cov.frame_station.end());
    if (*bounds.first != 0 || *bounds.second >= ds.num_cameras)
        throw std::runtime_error("axis-aligned partition: invalid panorama station mapping");
    const int count = *bounds.second + 1;
    std::vector<Station> stations((size_t)count);
    for (size_t f = 0; f < cov.frame_station.size(); f++) {
        Station& s = stations[(size_t)cov.frame_station[f]];
        s.frames.push_back((int32_t)f);
        for (int a = 0; a < 3; a++) s.center[a] += ds.c2w[f * 12 + a * 4 + 3];
    }
    for (size_t i = 0; i < stations.size(); i++) {
        Station& s = stations[i];
        if (s.frames.empty()) throw std::runtime_error("axis-aligned partition: empty panorama station");
        for (int a = 0; a < 3; a++) {
            s.center[a] = cov.station_centers.empty() ? s.center[a] / s.frames.size() + ds.center[a]
                                                       : cov.station_centers.at(i)[a];
            if (!std::isfinite(s.center[a])) throw std::runtime_error("axis-aligned partition: nonfinite station position");
        }
    }
    return stations;
}

std::vector<float> local_centers(const std::vector<Station>& stations, const Vec& origin) {
    std::vector<float> xyz;
    xyz.reserve(stations.size() * 3);
    for (const Station& s : stations)
        for (int a = 0; a < 3; a++) xyz.push_back((float)(s.center[a] - origin[a]));
    return xyz;
}

double median(std::vector<double>& values) {
    if (values.empty()) return 0;
    auto mid = values.begin() + values.size() / 2;
    std::nth_element(values.begin(), mid, values.end());
    return *mid;
}

Vec3 loop_normal(const std::vector<Station>& stations, const Vec& origin, Vec3 reference) {
    double covariance[9]{};
    for (const Station& s : stations) {
        const Vec d = as_array(as_vec(s.center) - as_vec(origin));
        for (int a = 0; a < 3; a++)
            for (int b = 0; b < 3; b++) covariance[a * 3 + b] += d[a] * d[b];
    }
    double values[3], vectors[9];
    sfm::jacobiEigenSymmetric(covariance, 3, values, vectors);
    int order[] = {0, 1, 2};
    std::sort(order, order + 3, [&](int a, int b) { return values[a] < values[b]; });
    if (values[order[1]] <= 0.01 * values[order[2]] || values[order[0]] > 0.05 * values[order[1]]) return {};
    const int a = order[0];
    Vec3 normal{vectors[a], vectors[3 + a], vectors[6 + a]};
    if (normal.dot(reference) < 0) normal = normal * -1;
    return normal;
}

struct Heading {
    double angle = 0;
    double confidence = 0;
};

Heading road_heading(const std::vector<Station>& stations, const Vec& origin, Vec3 u, Vec3 v,
                     const std::atomic<bool>* cancel) {
    double sx = 0, sy = 0, xx = 0, yy = 0, xy = 0;
    for (size_t i = 0; i < stations.size(); i++) {
        if (i % 1024 == 0) stop_if(cancel);
        const Vec3 d = as_vec(stations[i].center) - as_vec(origin);
        const double x = d.dot(u), y = d.dot(v);
        sx += x;
        sy += y;
        xx += x * x;
        yy += y * y;
        xy += x * y;
    }
    const double n = (double)stations.size();
    xx -= sx * sx / n;
    yy -= sy * sy / n;
    xy -= sx * sy / n;
    const double trace = xx + yy;
    if (trace <= 0) return {};
    return {0.5 * std::atan2(2 * xy, xx - yy), std::min(1.0, std::hypot(xx - yy, 2 * xy) / trace)};
}

std::set<std::vector<size_t>> capture_sequences(const ParsedDataset& ds, const std::vector<Station>& stations) {
    using NumberedFrame = std::pair<uint64_t, size_t>;
    std::map<std::pair<std::filesystem::path, std::string>, std::vector<NumberedFrame>> groups;
    for (size_t i = 0; i < stations.size(); i++) {
        for (int32_t frame : stations[i].frames) {
            const auto path = std::filesystem::u8path(ds.image_filenames[(size_t)frame]);
            const std::string stem = path.stem().u8string();
            const size_t last_prefix = stem.find_last_not_of("0123456789");
            const size_t digits = last_prefix == std::string::npos ? 0 : last_prefix + 1;
            uint64_t number = 0;
            const auto parsed = std::from_chars(stem.data() + digits, stem.data() + stem.size(), number);
            if (parsed.ec == std::errc{})
                groups[{path.parent_path(), stem.substr(0, digits)}].emplace_back(number, i);
        }
    }
    std::set<std::vector<size_t>> sequences;
    for (auto& entry : groups) {
        auto& frames = entry.second;
        std::sort(frames.begin(), frames.end());
        frames.erase(std::unique(frames.begin(), frames.end()), frames.end());
        if (frames.size() < 3 || std::adjacent_find(frames.begin(), frames.end(),
                [](const auto& a, const auto& b) { return a.first == b.first; }) != frames.end()) continue;
        std::vector<size_t> sequence;
        for (const auto& frame : frames) sequence.push_back(frame.second);
        sequences.insert(std::move(sequence));
    }
    return sequences;
}

Heading loop_heading(const ParsedDataset& ds, const std::vector<Station>& stations, const Vec& origin,
                     Vec3 u, Vec3 v, const std::atomic<bool>* cancel) {
    const auto sequences = capture_sequences(ds, stations);
    std::vector<Vec3> xy;
    std::vector<double> xs, ys;
    for (const Station& station : stations) {
        const Vec3 d = as_vec(station.center) - as_vec(origin);
        xy.push_back({d.dot(u), d.dot(v), 0});
        xs.push_back(xy.back().x);
        ys.push_back(xy.back().y);
    }
    auto extent = [](std::vector<double>& values) {
        std::sort(values.begin(), values.end());
        const size_t trim = values.size() / 20;
        return values[values.size() - 1 - trim] - values[trim];
    };
    const double span = std::hypot(extent(xs), extent(ys));
    if (span <= 0) return {};
    constexpr std::array<double, 3> scales = {0.10, 0.16, 0.24};
    constexpr double kMinDirectionAgreement = 0.8660254037844386;  // cos(30 degrees)
    struct Vote { double angle = 0, weight = 0; };
    std::vector<std::array<Vote, scales.size()>> votes(stations.size());
    for (const auto& sequence : sequences) {
        std::vector<double> step(sequence.size()), arc(sequence.size()), steps;
        for (size_t i = 1; i < sequence.size(); i++) {
            step[i] = (xy[sequence[i]] - xy[sequence[i - 1]]).norm();
            arc[i] = arc[i - 1] + step[i];
            if (step[i] > 0) steps.push_back(step[i]);
        }
        const double spacing = median(steps);
        if (spacing <= 0) continue;
        const double gap = std::max(8 * spacing, 0.06 * span);
        std::vector<size_t> gaps(sequence.size());
        for (size_t i = 1; i < sequence.size(); i++) gaps[i] = gaps[i - 1] + (step[i] > gap);
        for (size_t i = 0; i + 2 < sequence.size(); i++) {
            if (i % 1024 == 0) stop_if(cancel);
            const double station_weight = std::min(step[i + 1], 2 * spacing);
            for (size_t k = 0; k < scales.size(); k++) {
                const size_t j = std::lower_bound(arc.begin() + i + 1, arc.end(),
                                                   arc[i] + scales[k] * span) - arc.begin();
                if (j >= sequence.size() || j - i < 2 || gaps[i] != gaps[j]) continue;
                const auto halfway = std::lower_bound(arc.begin() + i + 1, arc.begin() + j, (arc[i] + arc[j]) * 0.5);
                const size_t mid = std::min(j - 1, (size_t)(halfway - arc.begin()));
                const Vec3 first = xy[sequence[mid]] - xy[sequence[i]];
                const Vec3 second = xy[sequence[j]] - xy[sequence[mid]];
                const double lengths = first.norm() * second.norm();
                if (lengths <= 0) continue;
                const double agreement = first.dot(second) / lengths;
                const double weight = station_weight * (agreement - kMinDirectionAgreement);
                Vote& vote = votes[sequence[i]][k];
                if (weight > vote.weight) {
                    const Vec3 chord = first + second;
                    vote = {std::atan2(chord.y, chord.x), weight};
                }
            }
        }
    }
    double c4 = 0, s4 = 0, mass = 0;
    for (const auto& station : votes)
        for (const Vote& vote : station) {
            c4 += vote.weight * std::cos(4 * vote.angle);
            s4 += vote.weight * std::sin(4 * vote.angle);
            mass += vote.weight;
        }
    if (mass <= 0) return {};
    return {std::atan2(s4, c4) / 4, std::min(1.0, std::hypot(c4, s4) / mass)};
}

ScenePartition::Axes estimate_axes(const ParsedDataset& ds, const std::vector<Station>& stations,
                                   PanoramaRoute route, const std::atomic<bool>* cancel) {
    ScenePartition::Axes axes;
    std::vector<double> values(stations.size());
    for (int a = 0; a < 3; a++) {
        for (size_t i = 0; i < stations.size(); i++) values[i] = stations[i].center[a];
        axes.origin[a] = median(values);
    }
    Vec3 up{0, 0, 1};
    axes.up_source = "dataset";
    if (!ds.gauge_oriented) {
        const auto& r = ds.normalized_rotation;
        up = Vec3{r[6], r[7], r[8]}.normalized();
        axes.up_source = "pose";
    }
    const auto horizontal_basis = [](Vec3 up) {
        const Vec3 reference = std::abs(up.x) < 0.9 ? Vec3{1, 0, 0} : Vec3{0, 1, 0};
        const Vec3 u = (reference - up * reference.dot(up)).normalized();
        return std::pair{u, up.cross(u)};
    };
    auto [u, v] = horizontal_basis(up);
    Heading heading = road_heading(stations, axes.origin, u, v, cancel);
    if (route == PanoramaRoute::Loop) {
        constexpr double kMaxTransverseVarianceRatio = 0.01;
        constexpr double kRoadConfidence = (1 - kMaxTransverseVarianceRatio) / (1 + kMaxTransverseVarianceRatio);
        axes.road_fallback = heading.confidence >= kRoadConfidence;
        if (!axes.road_fallback && !ds.gauge_oriented) {
            const Vec3 normal = loop_normal(stations, axes.origin, up);
            if (normal.norm() > 0) {
                up = normal;
                axes.up_source = "trajectory";
                std::tie(u, v) = horizontal_basis(up);
            }
        }
        if (!axes.road_fallback) heading = loop_heading(ds, stations, axes.origin, u, v, cancel);
    }
    axes.confidence = heading.confidence;
    axes.heading_fallback = axes.confidence < 0.25;
    const double angle = axes.heading_fallback ? 0 : heading.angle;
    const Vec3 x = u * std::cos(angle) + v * std::sin(angle);
    axes.basis = {as_array(x), as_array(up.cross(x)), as_array(up)};
    return axes;
}

std::vector<int> loop_successors(const std::vector<std::array<double, 2>>& xy) {
    std::array<double, 2> center{};
    for (int a = 0; a < 2; a++) {
        const auto bounds = std::minmax_element(xy.begin(), xy.end(),
            [a](const auto& x, const auto& y) { return x[a] < y[a]; });
        center[a] = (*bounds.first)[a] + ((*bounds.second)[a] - (*bounds.first)[a]) / 2;
    }
    std::vector<std::pair<double, int>> order;
    for (size_t i = 0; i < xy.size(); i++)
        order.emplace_back(std::atan2(xy[i][1] - center[1], xy[i][0] - center[0]), (int)i);
    std::sort(order.begin(), order.end());
    std::vector<int> next(xy.size());
    for (size_t i = 0; i < order.size(); i++) next[(size_t)order[i].second] = order[(i + 1) % order.size()].second;
    return next;
}

Vec source_point(const ParsedDataset& ds, size_t i) {
    return {ds.points.xyz[i * 3] + ds.center[0], ds.points.xyz[i * 3 + 1] + ds.center[1],
            ds.points.xyz[i * 3 + 2] + ds.center[2]};
}

struct Sample {
    std::array<double, 2> xy;
    size_t point;
};

struct Candidate {
    double score, cut;
    int axis, left_parts;
};

class SpatialCut {
public:
    SpatialCut(const Covisibility& cov, const std::vector<double>& cost, const std::vector<int32_t>& initial,
               const std::vector<std::array<double, 2>>& xy, const std::vector<Sample>& samples,
               const std::vector<int>& next, int budget, const std::atomic<bool>* cancel,
               ScenePartition::Axes& axes)
        : cov(cov), cost(cost), initial(initial), xy(xy), samples(samples), next(next),
          budget(budget), cancel(cancel), axes(axes) {}

    bool run(int count) {
        std::vector<int> stations(xy.size()), points(samples.size());
        std::iota(stations.begin(), stations.end(), 0);
        std::iota(points.begin(), points.end(), 0);
        axes.splits.clear();
        parts = 0;
        attempts = 0;
        int root = 0;
        return split(stations, points, count, root);
    }

    int parts = 0;

private:
    const Covisibility& cov;
    const std::vector<double>& cost;
    const std::vector<int32_t>& initial;
    const std::vector<std::array<double, 2>>& xy;
    const std::vector<Sample>& samples;
    const std::vector<int>& next;
    int budget;
    const std::atomic<bool>* cancel;
    ScenePartition::Axes& axes;
    int attempts = 0;

    void coverage(const std::vector<double>& cuts, const std::vector<int>& points, const std::vector<int>& rank,
                  int axis, std::vector<double>& penalty) const {
        std::vector<int> count_low(cuts.size() + 1), miss_low(count_low), miss_high(count_low);
        int total = 0;
        auto after = [&](double v) { return std::upper_bound(cuts.begin(), cuts.end(), v) - cuts.begin(); };
        for (int i : points) {
            const Sample& s = samples[(size_t)i];
            const auto pivot = after(s.xy[axis]);
            double low = std::numeric_limits<double>::infinity(), high = -low;
            for (int64_t j = cov.beg[s.point]; j < cov.beg[s.point + 1]; j++) {
                const size_t f = (size_t)cov.frame[(size_t)j];
                if (rank[f] < 0) continue;
                low = std::min(low, xy[f][axis]);
                high = std::max(high, xy[f][axis]);
            }
            total++;
            count_low[pivot]++;
            // A point switches sides at its own coordinate and gains coverage at its extreme observers.
            if (low > s.xy[axis]) { miss_low[pivot]++; miss_low[after(low)]--; }
            if (high < s.xy[axis]) { miss_high[after(high)]++; miss_high[pivot]--; }
        }
        int lo = 0, ml = 0, mh = 0;
        for (size_t k = 0; k < cuts.size(); k++) {
            lo += count_low[k]; ml += miss_low[k]; mh += miss_high[k];
            const int hi = total - lo;
            penalty[k] = ((lo ? (double)ml / lo : 0) + (hi ? (double)mh / hi : 0)) /
                         std::max(1, (lo > 0) + (hi > 0));
        }
    }

    void candidates(const std::vector<int>& stations, const std::vector<int>& points, int count,
                    int axis, std::vector<Candidate>& out) const {
        std::vector<int> sorted = stations;
        std::sort(sorted.begin(), sorted.end(), [&](int a, int b) { return xy[a][axis] < xy[b][axis]; });
        const int n = (int)sorted.size();
        std::vector<int> rank(xy.size(), -1);
        for (int i = 0; i < n; i++) rank[sorted[i]] = i;
        std::vector<double> crossing((size_t)n + 1);
        std::vector<int> cycle_low((size_t)n + 1), cycle_high(cycle_low);
        double total = 0;
        for (int i = 0; i < n; i++) {
            const int s = sorted[i];
            for (uint32_t e = cov.cameras.offs[s]; e < cov.cameras.offs[s + 1]; e++) {
                const int j = rank[cov.cameras.adj[e]];
                if (j <= i) continue;
                const double w = cov.cameras.w[e];
                crossing[i + 1] += w;
                crossing[j + 1] -= w;
                total += w;
            }
            if (!next.empty() && rank[next[s]] >= 0) {
                const int j = rank[next[s]];
                cycle_low[std::max(i, j) + 1]++;
                cycle_high[0]++;
                cycle_high[std::min(i, j) + 1]--;
            }
        }
        std::vector<double> cuts;
        for (int i = 1; i < n; i++) {
            const double a = xy[sorted[i - 1]][axis], b = xy[sorted[i]][axis];
            if (a < b) {
                const double mid = a + (b - a) / 2;
                cuts.push_back(mid > a ? mid : b);
            }
        }
        std::vector<double> penalty(cuts.size());
        if (!points.empty()) coverage(cuts, points, rank, axis, penalty);
        std::vector<int> left(xy.size()), right(xy.size());
        double load = 0, total_load = 0, cut_weight = 0;
        int broken = 0, low_edges = 0, high_edges = cycle_high[0], k = 0;
        for (int s : sorted) { right[initial[s]]++; total_load += cost[s]; }
        for (int i = 1; i < n; i++) {
            const int s = sorted[i - 1], label = initial[s];
            broken -= std::min(left[label], right[label]);
            ++left[label]; --right[label];
            broken += std::min(left[label], right[label]);
            load += cost[s]; cut_weight += crossing[i];
            low_edges += cycle_low[i]; high_edges += cycle_high[i];
            if (xy[s][axis] == xy[sorted[i]][axis]) continue;
            const double cut = cuts[k];
            const double missing = penalty[k++];
            const double route_penalty = next.empty() ? 0 :
                (std::max(0, i - low_edges - 1) + std::max(0, n - i - high_edges - 1));
            const double base = (total > 0 ? cut_weight / total : 0) + 0.5 * broken / n +
                                missing + route_penalty;
            if (count > 0) {
                const int min_left = std::max(1, count - (n - i));
                const int max_left = std::min(count - 1, i);
                for (int leaves = min_left; leaves <= max_left; leaves++)
                    out.push_back({base + 0.5 * std::abs(load / total_load - (double)leaves / count), cut, axis, leaves});
            } else {
                out.push_back({base + 0.5 * std::abs(2 * load / total_load - 1), cut, axis, 0});
            }
        }
    }

    bool split(const std::vector<int>& stations, const std::vector<int>& points, int count, int& node) {
        stop_if(cancel);
        if (++attempts > 4096 || parts >= 254) return false;
        double load = 0;
        for (int i : stations) load += cost[i];
        if (count == 1 || (count == 0 && load <= budget)) { node = ~parts++; return true; }
        if (stations.size() < 2) return false;
        std::vector<Candidate> options;
        candidates(stations, points, count, 0, options);
        if (!next.empty()) candidates(stations, points, count, 1, options);
        std::sort(options.begin(), options.end(), [](const Candidate& a, const Candidate& b) {
            return std::tie(a.score, a.axis, a.cut, a.left_parts) < std::tie(b.score, b.axis, b.cut, b.left_parts);
        });
        const int saved_parts = parts;
        node = (int)axes.splits.size();
        for (const Candidate& c : options) {
            axes.splits.push_back({c.axis, c.cut, 0, 0});
            std::vector<int> low, high, low_points, high_points;
            for (int i : stations) (xy[i][c.axis] < c.cut ? low : high).push_back(i);
            for (int i : points) (samples[i].xy[c.axis] < c.cut ? low_points : high_points).push_back(i);
            int left, right;
            if (split(low, low_points, c.left_parts, left) && split(high, high_points, count - c.left_parts, right)) {
                axes.splits[(size_t)node].low = left;
                axes.splits[(size_t)node].high = right;
                return true;
            }
            axes.splits.resize((size_t)node);
            parts = saved_parts;
            if (attempts > 4096) break;
        }
        return false;
    }
};

}  // namespace

int ScenePartition::Axes::owner(const double p[3]) const {
    if (splits.empty()) return 0;
    int i = 0;
    while (i >= 0) {
        const AxisSplit& s = splits[(size_t)i];
        i = project(basis[s.axis], p) >= s.cut ? s.high : s.low;
    }
    return ~i;
}

std::shared_ptr<const Region> ScenePartition::Axes::region(int part) const {
    auto r = std::make_shared<CsgRegion>();
    r->op = CsgOp::Intersection;
    if (splits.empty()) {
        auto all = std::make_shared<HalfSpaceRegion>();
        std::fill_n(all->normal, 3, 0.0);
        all->offset = 1;
        return all;
    }
    using Bounds = std::array<double, 2>;
    const double inf = std::numeric_limits<double>::infinity();
    auto find = [&](auto&& self, int i, Bounds lo, Bounds hi) -> bool {
        if (i < 0) {
            if (~i != part) return false;
            for (int a = 0; a < 2; a++) {
                if (std::isfinite(lo[a])) {
                    auto plane = std::make_shared<HalfSpaceRegion>();
                    std::copy(basis[a].begin(), basis[a].end(), plane->normal);
                    plane->offset = -lo[a];
                    r->children.push_back(plane);
                }
                if (std::isfinite(hi[a])) {
                    auto plane = std::make_shared<HalfSpaceRegion>();
                    std::copy(basis[a].begin(), basis[a].end(), plane->normal);
                    plane->offset = -hi[a];
                    auto complement = std::make_shared<CsgRegion>();
                    complement->op = CsgOp::Complement;
                    complement->children.push_back(plane);
                    r->children.push_back(complement);
                }
            }
            return true;
        }
        const AxisSplit& s = splits[(size_t)i];
        Bounds left = hi, right = lo;
        left[s.axis] = std::min(left[s.axis], s.cut);
        right[s.axis] = std::max(right[s.axis], s.cut);
        return self(self, s.low, lo, left) || self(self, s.high, right, hi);
    };
    return find(find, 0, {-inf, -inf}, {inf, inf}) ? r : nullptr;
}

void panorama_proximity(const ParsedDataset& ds, Covisibility& cov, int neighbours,
                        const std::atomic<bool>* cancel) {
    const auto stations = stations_of(ds, cov);
    const auto xyz = local_centers(stations, stations.front().center);
    const knn::KdTree3 tree(xyz.data(), (int64_t)stations.size());
    graph::EdgeAccumulator graph;
    const int k = std::clamp(neighbours, 1, (int)stations.size());
    std::vector<float> dist((size_t)k);
    std::vector<int32_t> neighbours_of_station((size_t)k);
    for (size_t i = 0; i < stations.size(); i++) {
        if (i % 1024 == 0) stop_if(cancel);
        const int count = tree.query(&xyz[i * 3], (int32_t)i, k, dist.data(), neighbours_of_station.data());
        for (int j = 0; j < count; j++) graph.add((uint32_t)i, (uint32_t)neighbours_of_station[j], 1.0);
    }
    cov.cameras = graph.build(stations.size());
    cov.beg.clear();
    cov.frame.clear();
    cov.source = CovisibilitySource::Proximity;
}

namespace {

struct Borrow {
    int station;
    double share;
    int64_t hits;
    size_t gain = 0;
    size_t revision = 0;
};

using Observed = std::vector<std::vector<size_t>>;

struct BorrowCandidates {
    std::vector<Borrow> regular;
    std::vector<Borrow> coverage;
};

Observed station_observations(const Covisibility& cov, const std::atomic<bool>* cancel) {
    Observed observed(cov.cameras.n());
    for (int64_t i = 0; i < cov.num_points(); i++) {
        if (i % 65536 == 0) stop_if(cancel);
        for (int64_t j = cov.beg[i]; j < cov.beg[i + 1]; j++) observed[cov.frame[j]].push_back((size_t)i);
    }
    return observed;
}

std::vector<BorrowCandidates> borrow_candidates(const ParsedDataset& ds, const Covisibility& cov,
                                                const std::vector<Station>& stations, const Observed& observed,
                                                const std::vector<int>& owner, const ScenePartition& p,
                                                const std::atomic<bool>* cancel) {
    std::vector<BorrowCandidates> all((size_t)p.num_parts);
    std::vector<double> weight, scratch, mass((size_t)p.num_parts);
    std::vector<int64_t> hits((size_t)p.num_parts);
    for (size_t s = 0; s < stations.size(); s++) {
        stop_if(cancel);
        std::fill(mass.begin(), mass.end(), 0);
        std::fill(hits.begin(), hits.end(), 0);
        double total = 0;
        if (cov.has_tracks()) {
            if (observed[s].empty()) continue;
            weight.clear();
            for (size_t i : observed[s]) {
                const Vec3 d = as_vec(source_point(ds, i)) - as_vec(stations[s].center);
                weight.push_back(1.0 / std::max(d.dot(d), 1e-12));
            }
            normalize_observation_weights(weight.data(), weight.size(), scratch);
            for (size_t j = 0; j < observed[s].size(); j++) {
                const double w = weight[j];
                const int k = p.point_label[observed[s][j]];
                hits[k]++;
                mass[k] += w;
                total += w;
            }
        } else {
            const auto& g = cov.cameras;
            for (uint32_t j = g.offs[s]; j < g.offs[s + 1]; j++) {
                mass[owner[g.adj[j]]] += g.w[j];
                total += g.w[j];
            }
        }
        if (total <= 0) continue;
        for (int k = 0; k < p.num_parts; k++) {
            if (k == owner[s] || mass[k] <= 0 || (cov.has_tracks() && hits[k] < p.options.ring_min_points))
                continue;
            const double share = mass[k] / total;
            if (share >= p.options.ring_fraction)
                all[k].regular.push_back({(int)s, share, hits[k]});
            else if (cov.has_tracks())
                all[k].coverage.push_back({(int)s, share, hits[k]});
        }
    }
    return all;
}

std::vector<int> choose_borrows(const std::vector<Borrow>& candidates, const std::vector<double>& cost,
                                const Observed& observed, const std::vector<int32_t>& point_owner,
                                std::vector<int>& seen, int part, int remaining, size_t min_gain,
                                const std::atomic<bool>* cancel) {
    std::vector<int> selected;
    if (remaining <= 0) return selected;
    auto mark = [&](int s) {
        for (size_t i : observed[s]) seen[i] = part;
        selected.push_back(s);
    };
    double all = 0;
    for (const Borrow& b : candidates) all += cost[b.station];
    if (min_gain == 0 && all <= remaining) {
        for (const Borrow& b : candidates) mark(b.station);
        return selected;
    }

    auto count_gain = [&](int s) {
        size_t gain = 0;
        for (size_t i : observed[s]) gain += point_owner[i] == part && seen[i] != part;
        return gain;
    };
    auto priority = [&](const Borrow& b) {
        return std::make_tuple((double)b.gain / cost[b.station], b.share, b.hits, -b.station);
    };
    auto lower = [&](const Borrow& a, const Borrow& b) { return priority(a) < priority(b); };
    std::priority_queue<Borrow, std::vector<Borrow>, decltype(lower)> heap(lower);
    for (Borrow b : candidates) {
        if (cost[b.station] > remaining) continue;
        b.gain = count_gain(b.station);
        if (b.gain >= min_gain) heap.push(b);
    }
    while (!heap.empty() && remaining > 0) {
        stop_if(cancel);
        Borrow b = heap.top();
        heap.pop();
        if (cost[b.station] > remaining) continue;
        if (b.revision != selected.size()) {
            b.gain = count_gain(b.station);
            b.revision = selected.size();
            if (b.gain >= min_gain) heap.push(b);
            continue;
        }
        mark(b.station);
        remaining -= (int)cost[b.station];
    }
    return selected;
}

void collect_parts(const Covisibility& cov, const std::vector<Station>& stations, const Observed& observed,
                   const std::vector<int>& owners, const std::vector<double>& cost,
                   const std::vector<BorrowCandidates>& candidates,
                   ScenePartition& p, const PartitionLog& log, const std::atomic<bool>* cancel) {
    const int count = p.num_parts;
    p.frame_label.resize(cov.frame_station.size());
    for (size_t f = 0; f < p.frame_label.size(); f++) p.frame_label[f] = owners[cov.frame_station[f]];
    p.ring.resize((size_t)count);
    p.part_points.resize((size_t)count);
    p.core_pieces.resize((size_t)count);
    p.view_share.resize((size_t)count);
    std::vector<std::vector<uint32_t>> core((size_t)count);
    for (size_t s = 0; s < stations.size(); s++) core[owners[s]].push_back((uint32_t)s);
    std::vector<int> seen(p.point_label.size(), -1), added(p.point_label.size(), -1);
    std::vector<int64_t> track_count((size_t)count), no_track((size_t)count);
    for (size_t i = 0; i < p.point_label.size(); i++) {
        const int k = p.point_label[i];
        if (cov.has_tracks() && cov.beg[i] != cov.beg[i + 1]) track_count[k]++;
        else no_track[k]++;
        if (!cov.has_tracks()) p.part_points[k].push_back((int64_t)i);
    }
    const uint32_t keep = (uint32_t)(std::clamp(p.options.outside_seed_fraction, 0.0f, 1.0f) * 65536.0f);
    for (int k = 0; k < count; k++) {
        stop_if(cancel);
        size_t core_images = 0;
        int64_t before = track_count[k];
        for (uint32_t s : core[k]) {
            core_images += stations[s].frames.size();
            for (size_t i : observed[s]) {
                if (seen[i] != k && p.point_label[i] == k) before--;
                seen[i] = k;
            }
        }
        const int remaining = p.options.parts > 0 ? (int)cov.frame_station.size() : p.options.max_images - (int)core_images;
        auto ring = choose_borrows(candidates[k].regular, cost, observed, p.point_label, seen, k, remaining, 0, cancel);
        const size_t regular_count = ring.size();
        int extra_budget = p.options.max_images - (int)core_images;
        for (int s : ring) extra_budget -= (int)cost[s];
        const auto extra = choose_borrows(candidates[k].coverage, cost, observed, p.point_label, seen, k, extra_budget,
                                          (size_t)std::max(1, p.options.ring_min_points), cancel);
        ring.insert(ring.end(), extra.begin(), extra.end());
        p.core_pieces[k] = (int32_t)graph::connected_components(cov.cameras, core[k]).size();
        int64_t after = track_count[k];
        double share = 0;
        auto add_points = [&](int s) {
            int64_t own = 0;
            for (size_t i : observed[s]) {
                const bool inside = p.point_label[i] == k;
                own += inside;
                if (added[i] == k) continue;
                added[i] = k;
                after -= inside;
                if (inside || keep_outside_seed(i, keep))
                    p.part_points[k].push_back((int64_t)i);
            }
            if (!observed[s].empty()) share += (double)own / (double)observed[s].size();
        };
        for (uint32_t s : core[k]) add_points((int)s);
        for (int s : ring) {
            p.ring[k].insert(p.ring[k].end(), stations[s].frames.begin(), stations[s].frames.end());
            add_points(s);
        }
        p.view_share[k] = (float)(share / (core[k].size() + ring.size()));
        std::sort(p.ring[k].begin(), p.ring[k].end());
        std::sort(p.part_points[k].begin(), p.part_points[k].end());
        if (log) {
            char buf[384];
            const auto image_count = core_images + p.ring[k].size();
            const auto over = image_count > (size_t)p.options.max_images ? image_count - p.options.max_images : 0;
            if (cov.has_tracks()) {
                const double denom = (double)std::max<int64_t>(1, track_count[k]);
                std::snprintf(buf, sizeof buf,
                    "  part %d: stations %zu + %zu, coverage stations %zu, images %zu, over budget %zu, dropped stations %zu, points %zu, "
                    "uncovered %lld (%.1f%%) -> %lld (%.1f%%), untracked %lld",
                    k, core[k].size(), ring.size(), extra.size(), image_count, over,
                    candidates[k].regular.size() - regular_count, p.part_points[k].size(), (long long)before,
                    100.0 * before / denom, (long long)after, 100.0 * after / denom, (long long)no_track[k]);
            } else {
                std::snprintf(buf, sizeof buf,
                    "  part %d: stations %zu + %zu, images %zu, over budget %zu, dropped stations %zu, points %zu, coverage unavailable",
                    k, core[k].size(), ring.size(), image_count, over, candidates[k].regular.size() - regular_count,
                    p.part_points[k].size());
            }
            log(buf);
        }
    }
}

}  // namespace

void partition_panorama(const ParsedDataset& ds, const Covisibility& cov, ScenePartition& p,
                        const PartitionLog& log, const std::atomic<bool>* cancel) {
    const auto stations = stations_of(ds, cov);
    const auto& opt = p.options;
    if (opt.parts > 254 || opt.parts > (int)stations.size() || opt.max_images < 1)
        throw std::runtime_error("axis-aligned partition: invalid image or part limit");
    if (cov.cameras.n() != stations.size() || (cov.has_tracks() && cov.num_points() != ds.points.num()))
        throw std::runtime_error("axis-aligned partition: invalid station graph");
    for (size_t f = 0; f < p.frame_centers.size(); f++) p.frame_centers[f] += (float)ds.center[f % 3];
    const auto axis_frame = estimate_axes(ds, stations, opt.panorama_route, cancel);
    const auto route = axis_frame.road_fallback ? PanoramaRoute::Road : opt.panorama_route;
    if (log && axis_frame.road_fallback) log("panorama loop: straight trajectory detected; cutting across the road");
    std::vector<double> cost(stations.size());
    std::vector<std::array<double, 2>> xy(stations.size());
    for (size_t i = 0; i < stations.size(); i++) {
        cost[i] = (double)stations[i].frames.size();
        for (int a = 0; a < 2; a++) xy[i][a] = project(axis_frame.basis[a], stations[i].center.data());
    }
    const auto next = route == PanoramaRoute::Loop ? loop_successors(xy) : std::vector<int>{};
    const size_t n_points = (size_t)ds.points.num();
    std::vector<Sample> sample;
    if (cov.has_tracks()) {
        const size_t stride = std::max<size_t>(1, (n_points + 8191) / 8192);
        for (size_t i = 0; i < n_points; i += stride) {
            if (cov.beg[i] == cov.beg[i + 1]) continue;
            const Vec q = source_point(ds, i);
            sample.push_back({{project(axis_frame.basis[0], q.data()), project(axis_frame.basis[1], q.data())}, i});
        }
    }
    const Observed observed = station_observations(cov, cancel);
    const int largest = (int)*std::max_element(cost.begin(), cost.end());
    if (opt.parts <= 0 && largest > opt.max_images)
        throw std::runtime_error("axis-aligned partition: a panorama station exceeds the image limit");
    int budget = ds.num_cameras <= opt.max_images ? opt.max_images : std::max(largest, (int)(0.85 * opt.max_images));
    std::vector<BorrowCandidates> candidates;
    std::vector<int> owners(stations.size());
    std::vector<int32_t> initial;
    p.point_label.resize(n_points);
    for (int attempt = 0; attempt < 6; attempt++) {
        graph::LabelCutOptions options;
        options.parts = (size_t)std::max(0, opt.parts);
        options.leaf_max = budget;
        options.cost = cost.data();
        options.min_part = options.min_final = 1;
        options.cancel = cancel;
        auto labels = graph::cut_labels(cov.cameras, options);
        stop_if(cancel);

        auto axes = std::make_shared<ScenePartition::Axes>(axis_frame);
        SpatialCut cut(cov, cost, labels, xy, sample, next, budget, cancel, *axes);
        if (!cut.run(std::max(0, opt.parts))) {
            if (p.axes) break;
            throw std::runtime_error("axis-aligned partition: no spatial cut satisfies the image or part limit");
        }
        p.axes = std::move(axes);
        p.num_parts = cut.parts;
        initial = std::move(labels);
        for (size_t s = 0; s < stations.size(); s++) owners[s] = p.axes->owner(stations[s].center.data());
        for (size_t i = 0; i < n_points; i++) {
            if (i % 65536 == 0) stop_if(cancel);
            p.point_label[i] = p.axes->owner(source_point(ds, i).data());
        }
        candidates = borrow_candidates(ds, cov, stations, observed, owners, p, cancel);
        if (opt.parts > 0) break;
        std::vector<int> loads((size_t)p.num_parts);
        for (size_t s = 0; s < stations.size(); s++) loads[owners[s]] += (int)cost[s];
        for (int k = 0; k < p.num_parts; k++)
            for (const Borrow& b : candidates[k].regular) loads[k] += (int)cost[b.station];
        const int heaviest = *std::max_element(loads.begin(), loads.end());
        if (heaviest <= opt.max_images) break;
        const int smaller = std::max({largest, opt.max_images / 2, (int)((double)budget * opt.max_images / heaviest)});
        if (smaller >= budget) break;
        budget = smaller;
    }
    p.initial_frame_label.reserve(cov.frame_station.size());
    for (int32_t s : cov.frame_station) p.initial_frame_label.push_back(initial[s]);
    double total = 0, crossing = 0;
    for (size_t s = 0; s < stations.size(); s++)
        for (uint32_t j = cov.cameras.offs[s]; j < cov.cameras.offs[s + 1]; j++) {
            total += cov.cameras.w[j];
            if (owners[s] != owners[cov.cameras.adj[j]]) crossing += cov.cameras.w[j];
        }
    p.cut_fraction = total > 0 ? crossing / total : 0;
    if (log) {
        std::vector<int> initial_owner(stations.size(), -1);
        std::vector<bool> fragmented(stations.size());
        for (size_t s = 0; s < stations.size(); s++) {
            const int g = initial[s];
            if (initial_owner[g] < 0) initial_owner[g] = owners[s];
            else if (initial_owner[g] != owners[s]) fragmented[g] = true;
        }
        char buf[256];
        std::snprintf(buf, sizeof buf,
            "panorama %s: parts %d, stations %zu, up %s, heading confidence %.2f%s, cut %.1f%%, split initial groups %zu",
            panorama_route_name(route), p.num_parts, stations.size(), p.axes->up_source.c_str(),
            p.axes->confidence, p.axes->heading_fallback ? " (fallback)" : "", p.cut_fraction * 100,
            (size_t)std::count(fragmented.begin(), fragmented.end(), true));
        log(buf);
    }
    collect_parts(cov, stations, observed, owners, cost, candidates, p, log, cancel);
}

}  // namespace spirula
