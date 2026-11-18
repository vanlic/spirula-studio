#include "data/PanoramaPartition.h"

#include "data/Knn.h"
#include "data/PanoramaPartitionGeometry.h"
#include "data/PanoramaPartitionRefine.h"
#include "data/PartitionVisibility.h"

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

using panorama::Vec;
using panorama::as_vec;
using panorama::median;
using panorama::project;
using panorama::source_point;
using panorama::stop_if;
using sfm::Vec3;

Vec as_array(Vec3 v) { return {v.x, v.y, v.z}; }

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

using CaptureSequences = std::set<std::vector<size_t>>;

CaptureSequences capture_sequences(const ParsedDataset& ds, const std::vector<Station>& stations,
                                   const std::atomic<bool>* cancel) {
    if (ds.image_filenames.size() != (size_t)ds.num_cameras) return {};
    using NumberedFrame = std::pair<uint64_t, size_t>;
    std::map<std::pair<std::filesystem::path, std::string>, std::vector<NumberedFrame>> groups;
    for (size_t i = 0; i < stations.size(); i++) {
        if (i % 1024 == 0) stop_if(cancel);
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
    CaptureSequences sequences;
    for (auto& entry : groups) {
        auto& frames = entry.second;
        std::sort(frames.begin(), frames.end());
        frames.erase(std::unique(frames.begin(), frames.end()), frames.end());
        if (frames.size() < 3 || std::adjacent_find(frames.begin(), frames.end(),
                [](const auto& a, const auto& b) { return a.first == b.first; }) != frames.end()) continue;
        std::set<size_t> unique_stations;
        std::vector<double> strides;
        for (size_t i = 0; i < frames.size(); i++) {
            unique_stations.insert(frames[i].second);
            if (i > 0) strides.push_back((double)(frames[i].first - frames[i - 1].first));
        }
        if (unique_stations.size() != frames.size()) continue;
        const double gap = 8 * median(strides);
        std::vector<size_t> sequence{frames.front().second};
        for (size_t i = 1; i < frames.size(); i++) {
            if ((double)(frames[i].first - frames[i - 1].first) > gap) {
                if (sequence.size() >= 3) sequences.insert(std::move(sequence));
                sequence.clear();
            }
            sequence.push_back(frames[i].second);
        }
        if (sequence.size() >= 3) sequences.insert(std::move(sequence));
    }
    return sequences;
}

struct CapturePath {
    std::vector<Vec3> steps;
    double spacing = 0;

    Vec3 chord(int first, int stride, bool closed) const {
        const int n = (int)steps.size();
        Vec3 sum{};
        for (int k = 0; k < 3; k++, first += stride) {
            if (!closed && (first < 0 || first >= n)) break;
            const Vec3 step = steps[(first + n) % n];
            if (step.norm() > 4 * spacing) break;
            sum = sum + step;
        }
        return sum;
    }
};

template <typename Displacement>
CapturePath capture_path(const std::vector<size_t>& sequence, Displacement displacement) {
    CapturePath path;
    std::vector<double> distances;
    for (size_t i = 1; i < sequence.size(); i++) {
        path.steps.push_back(displacement(sequence[i - 1], sequence[i]));
        const double distance = path.steps.back().norm();
        if (distance > 0) distances.push_back(distance);
    }
    path.spacing = median(distances);
    return path;
}

bool has_fold(const CaptureSequences& sequences, const std::vector<Station>& stations, Vec3 u, Vec3 v,
              const std::atomic<bool>* cancel) {
    auto displacement = [&](size_t a, size_t b) {
        const Vec3 d = as_vec(stations[b].center) - as_vec(stations[a].center);
        return Vec3{d.dot(u), d.dot(v), 0};
    };
    for (const auto& sequence : sequences) {
        const auto path = capture_path(sequence, displacement);
        if (path.spacing <= 0) continue;
        const double min_leg = 2 * path.spacing;
        const int n = (int)path.steps.size();
        for (int i = 2; i + 3 < n; i++) {
            if (i % 1024 == 0) stop_if(cancel);
            const Vec3 before = path.chord(i, -1, false);
            if (std::abs(before.x) < min_leg || std::abs(before.x) < 0.9 * before.norm()) continue;
            for (int j = i + 1; j < std::min(n - 2, i + 9); j++) {
                if (path.steps[j - 1].norm() > 4 * path.spacing) break;
                const Vec3 after = path.chord(j, 1, false);
                if (before.x * after.x >= 0 || std::abs(after.x) < min_leg ||
                    std::abs(after.x) < 0.9 * after.norm()) continue;
                const Vec3 separation = displacement(sequence[i + 1], sequence[j]) + (before + after) * 0.5;
                if (std::abs(separation.y) >= min_leg) return true;
            }
        }
    }
    return false;
}

Heading loop_heading(const CaptureSequences& sequences, const std::vector<Station>& stations, const Vec& origin,
                     Vec3 u, Vec3 v, const std::atomic<bool>* cancel) {
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
                                   PanoramaRoute route, const CaptureSequences& sequences,
                                   const std::atomic<bool>* cancel) {
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
        const Vec3 road = u * std::cos(heading.angle) + v * std::sin(heading.angle);
        axes.road_fallback = heading.confidence >= kRoadConfidence &&
                             !has_fold(sequences, stations, road, up.cross(road), cancel);
        if (!axes.road_fallback && !ds.gauge_oriented) {
            const Vec3 normal = loop_normal(stations, axes.origin, up);
            if (normal.norm() > 0) {
                up = normal;
                axes.up_source = "trajectory";
                std::tie(u, v) = horizontal_basis(up);
            }
        }
        if (!axes.road_fallback) heading = loop_heading(sequences, stations, axes.origin, u, v, cancel);
    }
    axes.confidence = heading.confidence;
    axes.heading_fallback = axes.confidence < 0.25;
    const double angle = axes.heading_fallback ? 0 : heading.angle;
    const Vec3 x = u * std::cos(angle) + v * std::sin(angle);
    axes.basis = {as_array(x), as_array(up.cross(x)), as_array(up)};
    return axes;
}

struct CaptureRoute {
    std::vector<std::vector<std::pair<size_t, double>>> links;
    std::vector<std::vector<std::pair<size_t, size_t>>> short_runs;
};

CaptureRoute capture_route(const CaptureSequences& sequences, const std::vector<std::array<double, 2>>& xy,
                           const Covisibility& cov, const std::atomic<bool>* cancel) {
    std::map<std::pair<size_t, size_t>, double> links;
    std::set<std::array<size_t, 3>> short_runs;
    auto add = [&](size_t a, size_t b, double weight) {
        auto& stored = links[std::minmax(a, b)];
        stored = std::max(stored, weight);
    };
    auto displacement = [&](size_t a, size_t b) {
        return Vec3{xy[b][0] - xy[a][0], xy[b][1] - xy[a][1], 0};
    };
    for (const auto& sequence : sequences) {
        stop_if(cancel);
        auto path = capture_path(sequence, displacement);
        const double spacing = path.spacing;
        if (spacing <= 0) continue;
        const double max_step = 4 * spacing;
        const size_t first = sequence.front(), last = sequence.back();
        const Vec3 seam = displacement(last, first);
        const Vec3 departure = path.chord(0, 1, false);
        const Vec3 arrival = path.chord((int)path.steps.size() - 1, -1, false);
        bool closed = false;
        if (cov.has_tracks() && sequence.size() >= 4 && seam.norm() <= 2 * spacing &&
            seam.dot(departure) >= 0 && seam.dot(arrival) >= 0 &&
            std::none_of(path.steps.begin(), path.steps.end(), [=](Vec3 step) { return step.norm() > max_step; })) {
            for (uint32_t e = cov.cameras.offs[first]; e < cov.cameras.offs[first + 1]; e++)
                closed |= cov.cameras.adj[e] == last;
            if (closed) path.steps.push_back(seam);
        }
        for (int i = 0; i < (int)path.steps.size(); i++) {
            if (i % 1024 == 0) stop_if(cancel);
            if (path.steps[i].norm() > max_step) continue;
            const Vec3 before = path.chord(i - 1, -1, closed), after = path.chord(i + 1, 1, closed);
            const double lengths = before.norm() * after.norm();
            const double turn = lengths > 0 ? 1 - std::clamp(before.dot(after) / lengths, 0.0, 1.0) : 0;
            add(sequence[i], sequence[(i + 1) % sequence.size()], 0.25 + 0.75 * turn);
        }
        const int n = (int)sequence.size();
        for (int i = 0; i < n; i++) {
            for (int radius : {1, 2, 3, 4}) {
                if (2 * radius >= n || (!closed && (i < radius || i + radius >= n))) continue;
                bool contiguous = true;
                for (int j = i - radius; j < i + radius; j++)
                    contiguous &= path.steps[(j + n) % n].norm() <= max_step;
                if (!contiguous) continue;
                const size_t a = sequence[(i - radius + n) % n], b = sequence[(i + radius) % n];
                short_runs.insert({sequence[i], std::min(a, b), std::max(a, b)});
            }
        }
    }
    CaptureRoute out;
    out.links.resize(xy.size());
    out.short_runs.resize(xy.size());
    for (const auto& entry : links) out.links[entry.first.first].emplace_back(entry.first.second, entry.second);
    for (const auto& run : short_runs) out.short_runs[run[0]].emplace_back(run[1], run[2]);
    return out;
}

struct Sample {
    std::array<double, 2> xy;
    size_t point;
};

struct Candidate {
    double score, cut;
    int axis, left_parts;
    int tier = 0;
    bool fragments = false;
    bool creates_short_run = false;
};

class SpatialCut {
public:
    SpatialCut(const Covisibility& cov, const std::vector<double>& cost, const std::vector<int32_t>& initial,
               const std::vector<std::array<double, 2>>& xy, const std::vector<Sample>& samples,
               const CaptureRoute& route, bool two_axes, int budget, const std::atomic<bool>* cancel,
               ScenePartition::Axes& axes)
        : cov(cov), cost(cost), initial(initial), xy(xy), samples(samples), route(route),
          two_axes(two_axes), budget(budget), cancel(cancel), axes(axes) {
        if (!two_axes) return;
        for (int a = 0; a < 2; a++) {
            const auto [lo, hi] = std::minmax_element(xy.begin(), xy.end(), [a](const auto& x, const auto& y) {
                return x[a] < y[a];
            });
            minimum_gap = std::max(minimum_gap, (*hi)[a] - (*lo)[a]);
        }
        minimum_gap *= 8 * std::numeric_limits<float>::epsilon();
        core.resize(xy.size());
        group.resize((size_t)*std::max_element(initial.begin(), initial.end()) + 1);
        for (size_t s = 0; s < xy.size(); s++) {
            if (s % 1024 == 0) stop_if(cancel);
            const int label = initial[s];
            double inside = 0, outside = 0;
            for (uint32_t e = cov.cameras.offs[s]; e < cov.cameras.offs[s + 1]; e++)
                (initial[cov.cameras.adj[e]] == label ? inside : outside) += cov.cameras.w[e];
            if (inside > outside) core[s] = cost[s] * (inside - outside) / (inside + outside);
            group[label].images += cost[s];
            group[label].core += core[s];
        }
    }

    bool run(int count) {
        std::vector<int> stations(xy.size()), points(samples.size());
        std::iota(stations.begin(), stations.end(), 0);
        std::iota(points.begin(), points.end(), 0);
        const int max_tier = two_axes ? 2 : 0;
        for (tier_limit = 0; tier_limit <= max_tier; tier_limit++) {
            axes.splits.clear();
            parts = 0;
            attempts = 0;
            int root = 0;
            if (split(stations, points, count, root)) return true;
        }
        return false;
    }

    int parts = 0;
    int relaxation() const { return tier_limit; }

private:
    const Covisibility& cov;
    const std::vector<double>& cost;
    const std::vector<int32_t>& initial;
    const std::vector<std::array<double, 2>>& xy;
    const std::vector<Sample>& samples;
    const CaptureRoute& route;
    bool two_axes;
    int budget;
    const std::atomic<bool>* cancel;
    ScenePartition::Axes& axes;
    int attempts = 0;
    int tier_limit = 0;
    double minimum_gap = 0;

    struct Mass {
        double images = 0, core = 0;
    };
    std::vector<Mass> group;
    std::vector<double> core;

    double core_loss(const Mass& left, const Mass& right, int label) const {
        const double original = group[label].core;
        const double available = left.core + right.core;
        if (available <= original * 0.5) return 0;
        // The threshold is relative to the original group, across all levels of the tree.
        const double target = std::min(available, 0.75 * original);
        return std::max(0.0, target - std::max(left.core, right.core)) / original;
    }

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
        std::sort(sorted.begin(), sorted.end(), [&](int a, int b) {
            return std::tie(xy[a][axis], xy[a][1 - axis], a) < std::tie(xy[b][axis], xy[b][1 - axis], b);
        });
        const int n = (int)sorted.size();
        auto new_position = [&](int i) {
            if (i == 0) return true;
            const auto& a = xy[sorted[i - 1]];
            const auto& b = xy[sorted[i]];
            return two_axes ? a != b : a[axis] != b[axis];
        };
        int distinct = 0;
        if (count > 0) {
            for (int i = 0; i < n; i++) distinct += new_position(i);
            if (distinct < count) return;
        }
        std::vector<int> rank(xy.size(), -1);
        for (int i = 0; i < n; i++) rank[sorted[i]] = i;
        std::vector<double> crossing((size_t)n + 1);
        std::vector<double> route_crossing((size_t)n + 1);
        std::vector<int> short_crossing((size_t)n + 1);
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
            if (two_axes) {
                for (const auto& [other, weight] : route.links[s]) {
                    const int j = rank[other];
                    if (j < 0) continue;
                    route_crossing[std::min(i, j) + 1] += weight;
                    route_crossing[std::max(i, j) + 1] -= weight;
                }
                for (const auto& [first, last] : route.short_runs[s]) {
                    const auto [lo, hi] = std::minmax(rank[first], rank[last]);
                    if (hi < 0 || (lo >= 0 && i >= lo && i <= hi)) continue;
                    // An endpoint outside this node already has a different owner.
                    const int endpoint = i < lo ? lo : hi;
                    short_crossing[std::min(i, endpoint) + 1]++;
                    short_crossing[std::max(i, endpoint) + 1]--;
                }
            }
        }
        const size_t first_option = out.size();
        std::vector<Mass> left(xy.size()), right(xy.size());
        double load = 0, total_load = 0, cut_weight = 0, route_weight = 0;
        double broken = 0, group_loss = 0, core_damage = 0;
        int groups = 0, distinct_left = 0, short_runs = 0;
        for (int s : sorted) {
            Mass& mass = right[initial[s]];
            groups += mass.images == 0;
            mass.images += two_axes ? cost[s] : 1;
            if (two_axes) mass.core += core[s];
            total_load += cost[s];
        }
        const double small_load = 0.5 * std::min((double)budget, total_load / 2);
        for (int i = 1; i < n; i++) {
            if (count > 0) distinct_left += new_position(i - 1);
            const int s = sorted[i - 1], label = initial[s];
            Mass& lo = left[label];
            Mass& hi = right[label];
            const double old_minority = std::min(lo.images, hi.images);
            const double old_damage = two_axes ? core_loss(lo, hi, label) : 0;
            const double images = two_axes ? cost[s] : 1;
            lo.images += images;
            hi.images -= images;
            const double change = std::min(lo.images, hi.images) - old_minority;
            broken += change;
            if (two_axes) {
                lo.core += core[s];
                hi.core -= core[s];
                group_loss += change / group[label].images;
                core_damage += core_loss(lo, hi, label) - old_damage;
            }
            load += cost[s]; cut_weight += crossing[i];
            route_weight += route_crossing[i];
            short_runs += short_crossing[i];
            const double a = xy[s][axis], b = xy[sorted[i]][axis];
            if (a == b) continue;
            const bool narrow_gap = b - a <= minimum_gap;
            const double mid = a + (b - a) / 2;
            const double cut = mid > a ? mid : b;
            const double route_penalty = std::max(0.0, route_weight);
            double drift = 0.5 * broken / n;
            if (two_axes) drift = broken / total_load + group_loss / groups + std::max(0.0, core_damage);
            const double base = (total > 0 ? cut_weight / total : 0) + drift +
                                0.05 * route_penalty / (1 + route_penalty) + 0.1 * short_runs / (1 + short_runs);
            const int tier = core_damage > 1e-9 ? 1 : 0;
            auto intact = [&](int label, const std::vector<Mass>& side, double side_load) {
                return group[label].core > 0 && group[label].images == side_load && side[label].images == side_load;
            };
            const double high_load = total_load - load;
            const bool intact_low = two_axes && intact(label, left, load);
            const bool intact_high = two_axes && intact(initial[sorted[i]], right, high_load);
            const bool fragments = two_axes && broken > 0;
            const bool creates_short_run = short_runs > 0;
            if (count > 0) {
                const int min_left = std::max(1, count - (distinct - distinct_left));
                const int max_left = std::min(count - 1, distinct_left);
                for (int leaves = min_left; leaves <= max_left; leaves++) {
                    const double target = (double)leaves / count;
                    const bool thin = two_axes &&
                        ((load < 0.75 * target * total_load && !(leaves == 1 && intact_low)) ||
                         (high_load < 0.75 * (1 - target) * total_load && !(count - leaves == 1 && intact_high)));
                    const int order = thin || narrow_gap ? 2 : tier;
                    if (order <= tier_limit)
                        out.push_back({base + 0.5 * std::abs(load / total_load - target),
                                       cut, axis, leaves, order, fragments, creates_short_run});
                }
            } else {
                const bool thin = two_axes && ((load < small_load && !intact_low) ||
                                               (high_load < small_load && !intact_high));
                const int order = thin || narrow_gap ? 2 : tier;
                if (order <= tier_limit)
                    out.push_back({base + 0.5 * std::abs(2 * load / total_load - 1),
                                   cut, axis, 0, order, fragments, creates_short_run});
            }
        }
        if (points.empty() || out.size() == first_option) return;
        std::vector<double> cuts;
        for (size_t i = first_option; i < out.size(); i++)
            if (cuts.empty() || cuts.back() != out[i].cut) cuts.push_back(out[i].cut);
        std::vector<double> penalty(cuts.size());
        coverage(cuts, points, rank, axis, penalty);
        size_t k = 0;
        for (size_t i = first_option; i < out.size(); i++) {
            if (out[i].cut != cuts[k]) k++;
            out[i].score += penalty[k];
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
        if (two_axes) candidates(stations, points, count, 1, options);
        std::sort(options.begin(), options.end(), [](const Candidate& a, const Candidate& b) {
            return std::tie(a.tier, a.fragments, a.creates_short_run, a.score, a.axis, a.cut, a.left_parts) <
                   std::tie(b.tier, b.fragments, b.creates_short_run, b.score, b.axis, b.cut, b.left_parts);
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
    if (part < 0) return nullptr;
    if (splits.empty()) {
        if (part != 0) return nullptr;
        auto all = std::make_shared<HalfSpaceRegion>();
        std::fill_n(all->normal, 3, 0.0);
        all->offset = 1;
        return all;
    }
    auto cells = std::make_shared<CsgRegion>();
    cells->op = CsgOp::Union;
    using Bounds = std::array<double, 2>;
    const double inf = std::numeric_limits<double>::infinity();
    auto collect = [&](auto&& self, int i, Bounds lo, Bounds hi) -> void {
        if (i < 0) {
            if (~i != part) return;
            auto cell = std::make_shared<CsgRegion>();
            cell->op = CsgOp::Intersection;
            for (int a = 0; a < 2; a++) {
                if (std::isfinite(lo[a])) {
                    auto plane = std::make_shared<HalfSpaceRegion>();
                    std::copy(basis[a].begin(), basis[a].end(), plane->normal);
                    plane->offset = -lo[a];
                    cell->children.push_back(plane);
                }
                if (std::isfinite(hi[a])) {
                    auto plane = std::make_shared<HalfSpaceRegion>();
                    std::copy(basis[a].begin(), basis[a].end(), plane->normal);
                    plane->offset = -hi[a];
                    auto complement = std::make_shared<CsgRegion>();
                    complement->op = CsgOp::Complement;
                    complement->children.push_back(plane);
                    cell->children.push_back(complement);
                }
            }
            cells->children.push_back(cell);
            return;
        }
        const AxisSplit& s = splits[(size_t)i];
        Bounds left = hi, right = lo;
        left[s.axis] = std::min(left[s.axis], s.cut);
        right[s.axis] = std::max(right[s.axis], s.cut);
        self(self, s.low, lo, left);
        self(self, s.high, right, hi);
    };
    collect(collect, 0, {-inf, -inf}, {inf, inf});
    if (cells->children.empty()) return nullptr;
    if (cells->children.size() == 1) return cells->children.front();
    return cells;
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
    const auto sequences = opt.panorama_route == PanoramaRoute::Loop ? capture_sequences(ds, stations, cancel)
                                                                    : CaptureSequences{};
    const auto axis_frame = estimate_axes(ds, stations, opt.panorama_route, sequences, cancel);
    const auto route = axis_frame.road_fallback ? PanoramaRoute::Road : opt.panorama_route;
    const bool two_axes = route != PanoramaRoute::Road;
    if (log && axis_frame.road_fallback) log("panorama loop: straight trajectory detected; cutting across the road");
    std::vector<double> cost(stations.size());
    std::vector<Vec> coordinates(stations.size());
    std::vector<std::array<double, 2>> xy(stations.size());
    for (size_t i = 0; i < stations.size(); i++) {
        cost[i] = (double)stations[i].frames.size();
        coordinates[i] = stations[i].center;
        for (int a = 0; a < 2; a++) xy[i][a] = project(axis_frame.basis[a], stations[i].center.data());
    }
    const auto capture = two_axes ? capture_route(sequences, xy, cov, cancel) : CaptureRoute{};
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
    int relaxation = 0;
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
        SpatialCut cut(cov, cost, labels, xy, sample, capture, two_axes, budget, cancel, *axes);
        if (!cut.run(std::max(0, opt.parts))) {
            if (p.axes) break;
            throw std::runtime_error("axis-aligned partition: no spatial cut satisfies the image or part limit");
        }
        p.axes = std::move(axes);
        p.num_parts = cut.parts;
        relaxation = cut.relaxation();
        initial = std::move(labels);
        for (size_t s = 0; s < stations.size(); s++) owners[s] = p.axes->owner(stations[s].center.data());
        if (two_axes) refine_panorama_ownership(ds, cov, coordinates, xy, owners, p.num_parts, cancel, *p.axes);
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
        std::vector<std::map<int, int64_t>> final_loads((size_t)*std::max_element(initial.begin(), initial.end()) + 1);
        for (size_t s = 0; s < stations.size(); s++) final_loads[initial[s]][owners[s]] += (int64_t)cost[s];
        int64_t retained = 0;
        int fragmented = 0;
        for (const auto& loads : final_loads) {
            int64_t main_leaf = 0;
            for (const auto& load : loads) main_leaf = std::max(main_leaf, load.second);
            retained += main_leaf;
            fragmented += loads.size() > 1;
        }
        char buf[320];
        std::snprintf(buf, sizeof buf,
            "panorama %s: parts %d, stations %zu, up %s, heading confidence %.2f%s, cut %.1f%%, split initial groups %d, "
            "initial group images retained %lld/%lld (%.1f%%), fit relaxation %d",
            panorama_route_name(route), p.num_parts, stations.size(), p.axes->up_source.c_str(),
            p.axes->confidence, p.axes->heading_fallback ? " (fallback)" : "", p.cut_fraction * 100, fragmented,
            (long long)retained, (long long)ds.num_cameras, 100.0 * retained / ds.num_cameras, relaxation);
        log(buf);
    }
    collect_parts(cov, stations, observed, owners, cost, candidates, p, log, cancel);
}

}  // namespace spirula
