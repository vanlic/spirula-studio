#include "data/PanoramaPartitionRefine.h"

#include "data/Knn.h"
#include "data/PanoramaPartitionGeometry.h"

#include <algorithm>
#include <cmath>
#include <limits>
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

struct AxisCell {
    int label = 0;
    int parent = -1;
    bool high_child = false;
    std::array<double, 2> low{}, upper{};
    std::vector<int> stations, points;
};

void axis_cells(const ScenePartition::Axes& axes, int node, AxisCell cell, std::vector<AxisCell>& leaves) {
    if (node < 0) {
        cell.label = ~node;
        leaves.push_back(std::move(cell));
        return;
    }
    const auto& split = axes.splits[node];
    AxisCell low = cell;
    low.parent = cell.parent = node;
    low.high_child = false;
    cell.high_child = true;
    low.upper[split.axis] = cell.low[split.axis] = split.cut;
    axis_cells(axes, split.low, std::move(low), leaves);
    axis_cells(axes, split.high, std::move(cell), leaves);
}

struct OwnershipSample {
    std::array<double, 2> xy;
    std::vector<std::pair<int, double>> votes;
    double reliability = 0;
};

constexpr double kMinOwnershipSupport = 0.55;
constexpr double kCloseGainFraction = 0.02;

std::vector<OwnershipSample> ownership_samples(const ParsedDataset& ds, const Covisibility& cov,
                                               const std::vector<Vec>& stations,
                                               const std::vector<int>& owners, int parts,
                                               const ScenePartition::Axes& axes,
                                               const std::atomic<bool>* cancel) {
    struct Track {
        Vec point;
        size_t index;
        double nearest_dist2;
        bool votes_for_cut;
    };
    const size_t count = (size_t)ds.points.num();
    constexpr size_t kMaxSamples = 32768;
    const size_t stride = std::max<size_t>(1, (count + kMaxSamples - 1) / kMaxSamples);
    std::vector<Track> tracks;
    std::vector<double> distances;
    for (size_t i = 0; i < count; i += stride) {
        if ((i / stride) % 1024 == 0) stop_if(cancel);
        // Jitter the stride because feature IDs can follow image or spatial order.
        const size_t j = i + (((i / stride) * 2654435761u) % std::min(stride, count - i));
        const auto observations = cov.beg[j + 1] - cov.beg[j];
        if (observations == 0) continue;
        const Vec point = source_point(ds, j);
        double nearest_dist2 = std::numeric_limits<double>::infinity();
        for (int64_t e = cov.beg[j]; e < cov.beg[j + 1]; e++) {
            const Vec3 d = as_vec(point) - as_vec(stations[cov.frame[e]]);
            nearest_dist2 = std::min(nearest_dist2, d.dot(d));
        }
        if (!std::isfinite(nearest_dist2)) continue;
        tracks.push_back({point, j, nearest_dist2, observations >= 2});
        if (observations >= 2) distances.push_back(nearest_dist2);
    }
    const double typical = median(distances);
    if (typical <= 0) return {};
    std::vector<OwnershipSample> samples;
    samples.reserve(tracks.size());
    std::vector<double> votes(parts);
    for (const auto& track : tracks) {
        if (samples.size() % 1024 == 0) stop_if(cancel);
        OwnershipSample sample;
        for (int a = 0; a < 2; a++) sample.xy[a] = project(axes.basis[a], track.point.data());
        std::fill(votes.begin(), votes.end(), 0);
        double total = 0;
        for (int64_t e = cov.beg[track.index]; e < cov.beg[track.index + 1]; e++) {
            const int station = cov.frame[e];
            const Vec3 d = as_vec(track.point) - as_vec(stations[station]);
            const double vote = 1.0 / (0.25 * typical + d.dot(d));
            votes[owners[station]] += vote;
            total += vote;
        }
        // Single-station tracks count toward coverage and carry no boundary vote.
        if (track.votes_for_cut) sample.reliability = typical / (typical + track.nearest_dist2);
        for (int label = 0; label < parts; label++)
            if (votes[label] > 0) sample.votes.emplace_back(label, sample.reliability * votes[label] / total);
        samples.push_back(std::move(sample));
    }
    return samples;
}

struct OwnershipCut {
    int axis;
    double position;
    bool high;

    bool contains(const std::array<double, 2>& xy) const { return (xy[axis] >= position) == high; }
    void clip(AxisCell& cell) const { (high ? cell.low : cell.upper)[axis] = position; }
    OwnershipCut opposite() const { return {axis, position, !high}; }
};

bool face_neighbours(const AxisCell& a, const AxisCell& b, double clearance) {
    for (int axis = 0; axis < 2; axis++) {
        if (a.low[axis] != b.upper[axis] && a.upper[axis] != b.low[axis]) continue;
        const int transverse = 1 - axis;
        const double overlap = std::min(a.upper[transverse], b.upper[transverse]) -
                               std::max(a.low[transverse], b.low[transverse]);
        if (overlap >= clearance)
            return true;
    }
    return false;
}

struct OwnershipPatch {
    OwnershipCut first{}, second{};
    bool corner = false;
    int label = 0;
    double gain = 0;
    int uncovered_delta = 0;
};

struct OwnershipCuts {
    std::vector<OwnershipCut> single, corner;
};

OwnershipCuts ownership_cuts(const AxisCell& cell, int axis, std::vector<int> ranked,
                             const std::vector<int>& receivers, const std::vector<std::array<double, 2>>& xy,
                             const std::vector<OwnershipSample>& samples, double clearance, int min_samples) {
    std::sort(ranked.begin(), ranked.end(), [&](int i, int j) {
        return std::tie(samples[i].xy[axis], i) < std::tie(samples[j].xy[axis], j);
    });
    const int count = (int)ranked.size();
    auto position = [&](int i) { return samples[ranked[i]].xy[axis]; };
    auto gap = [&](int i) { return position(i) - position(i - 1); };
    auto midpoint = [&](int i) { return position(i - 1) + gap(i) / 2; };
    auto fits = [&](double cut) {
        return cut - cell.low[axis] >= clearance && cell.upper[axis] - cut >= clearance;
    };

    std::vector<double> shared, singles;
    for (int octile = 0; octile < 8; octile++) {
        const int start = count * octile / 8;
        const int end = count * (octile + 1) / 8;
        if (start > 0) shared.push_back(midpoint(start));
        int widest = -1;
        for (int i = start + 1; i < end; i++)
            if (gap(i) > 0 && (widest < 0 || gap(i) > gap(widest)) && fits(midpoint(i))) widest = i;
        if (widest >= 0) singles.push_back(midpoint(widest));
    }
    const auto [lo, hi] = std::minmax_element(cell.stations.begin(), cell.stations.end(), [&](int i, int j) {
        return xy[i][axis] < xy[j][axis];
    });
    shared.push_back(xy[*lo][axis] - clearance);
    shared.push_back(xy[*hi][axis] + clearance);

    const int window = std::max(min_samples / 2, count / 32);
    std::vector<double> mass(count + 1), votes(count + 1), transitions(count);
    for (int i = 0; i < count; i++) mass[i + 1] = mass[i] + samples[ranked[i]].reliability;
    for (int label : receivers) {
        for (int i = 0; i < count; i++) {
            const auto& support = samples[ranked[i]].votes;
            const auto it = std::lower_bound(support.begin(), support.end(), label,
                                             [](const auto& vote, int owner) { return vote.first < owner; });
            votes[i + 1] = votes[i] + (it != support.end() && it->first == label ? it->second : 0);
        }
        for (int i = window; i + window <= count; i++) {
            if (gap(i) <= 0 || !fits(midpoint(i))) continue;
            const double left = mass[i] - mass[i - window];
            const double right = mass[i + window] - mass[i];
            if (left <= 0 || right <= 0) continue;
            const double change = std::abs((votes[i] - votes[i - window]) / left -
                                           (votes[i + window] - votes[i]) / right);
            if (change >= 0.15) transitions[i] = std::max(transitions[i], change * std::min(left, right));
        }
    }
    constexpr int kMaxTransitions = 4;
    constexpr int kMaxCornerTransitions = 2;
    int added = 0;
    while (added < kMaxTransitions) {
        const auto strongest = std::max_element(transitions.begin(), transitions.end());
        if (*strongest <= 0) break;
        const int i = (int)(strongest - transitions.begin());
        const double cut = midpoint(i);
        *strongest = 0;
        if (std::find(shared.begin(), shared.end(), cut) != shared.end()) continue;
        if (added < kMaxCornerTransitions) shared.push_back(cut);
        else singles.push_back(cut);
        std::fill(transitions.begin() + std::max(0, i - window + 1),
                  transitions.begin() + std::min(count, i + window), 0);
        added++;
    }

    auto cuts = [&](std::vector<double>& boundaries) {
        std::sort(boundaries.begin(), boundaries.end());
        boundaries.erase(std::unique(boundaries.begin(), boundaries.end()), boundaries.end());
        std::vector<OwnershipCut> result;
        for (double cut : boundaries) {
            if (!fits(cut)) continue;
            result.push_back({axis, cut, false});
            result.push_back({axis, cut, true});
        }
        return result;
    };
    singles.insert(singles.end(), shared.begin(), shared.end());
    return {cuts(singles), cuts(shared)};
}

OwnershipPatch best_ownership_patch(const AxisCell& cell, const std::vector<AxisCell>& cells,
                                    const std::vector<int>& cell_counts,
                                    const std::vector<std::array<double, 2>>& xy,
                                    const std::vector<OwnershipSample>& samples,
                                    double clearance, int min_samples) {
    std::vector<const AxisCell*> neighbours;
    std::vector<const AxisCell*> connected;
    std::vector<int> receivers;
    for (const auto& other : cells) {
        if (!face_neighbours(cell, other, clearance)) continue;
        if (other.label == cell.label) connected.push_back(&other);
        else if (cell_counts[other.label] < ScenePartition::Axes::kMaxCellsPerPart) {
            neighbours.push_back(&other);
            receivers.push_back(other.label);
        }
    }
    if (neighbours.empty()) return {};
    std::sort(receivers.begin(), receivers.end());
    receivers.erase(std::unique(receivers.begin(), receivers.end()), receivers.end());

    std::vector<int> ranked;
    for (int i : cell.points)
        if (samples[i].reliability > 0) ranked.push_back(i);
    if ((int)ranked.size() < min_samples) return {};
    const auto x = ownership_cuts(cell, 0, ranked, receivers, xy, samples, clearance, min_samples);
    const auto y = ownership_cuts(cell, 1, std::move(ranked), receivers, xy, samples, clearance, min_samples);

    std::vector<double> support(cell_counts.size());
    std::vector<int> observed(cell_counts.size());
    std::vector<int> voters;
    std::vector<OwnershipPatch> candidates;
    double max_gain = 0;
    auto assess = [&](OwnershipCut first, OwnershipCut second, bool corner) {
        for (int s : cell.stations) {
            auto guarded = [&](OwnershipCut cut) {
                if (cut.contains(xy[s])) return true;
                return cut.high ? xy[s][cut.axis] + clearance > cut.position
                                : xy[s][cut.axis] - clearance < cut.position;
            };
            if (guarded(first) && (!corner || guarded(second))) return;
        }
        AxisCell patch;
        patch.low = cell.low;
        patch.upper = cell.upper;
        AxisCell remainder = patch;
        first.clip(patch);
        first.opposite().clip(remainder);
        AxisCell corner_remainder = patch;
        if (corner) second.clip(patch);
        voters.clear();
        for (const AxisCell* other : neighbours)
            if (face_neighbours(patch, *other, clearance) &&
                std::find(voters.begin(), voters.end(), other->label) == voters.end())
                voters.push_back(other->label);
        if (voters.empty()) return;
        if (corner) second.opposite().clip(corner_remainder);
        // Prior patches may depend on this cell for their connection to the donor.
        for (const AxisCell* other : connected)
            if (!face_neighbours(remainder, *other, clearance) &&
                !(corner && face_neighbours(corner_remainder, *other, clearance)))
                return;
        std::fill(support.begin(), support.end(), 0);
        std::fill(observed.begin(), observed.end(), 0);
        double total = 0;
        int points = 0;
        for (int i : cell.points) {
            const auto& sample = samples[i];
            if (!first.contains(sample.xy) || (corner && !second.contains(sample.xy))) continue;
            points += sample.reliability > 0;
            total += sample.reliability;
            for (const auto& [label, weight] : sample.votes) {
                support[label] += weight;
                observed[label]++;
            }
        }
        if (points < min_samples) return;
        for (int label : voters) {
            if (support[label] < kMinOwnershipSupport * total) continue;
            const double gain = support[label] - support[cell.label] - 0.15 * total -
                                (corner ? 0.5 : 0.25) * min_samples;
            if (gain <= 0) continue;
            // Only transferred points change the combined uncovered count of the two parts.
            candidates.push_back({first, second, corner, label, gain, observed[cell.label] - observed[label]});
            max_gain = std::max(max_gain, gain);
        }
    };
    for (auto cut : x.single) assess(cut, {}, false);
    for (auto cut : y.single) assess(cut, {}, false);
    if (cell_counts[cell.label] < ScenePartition::Axes::kMaxCellsPerPart)
        for (auto first : x.corner)
            for (auto second : y.corner) assess(first, second, true);

    const double gain_floor = (1 - kCloseGainFraction) * max_gain;
    OwnershipPatch best;
    auto preference = [](const OwnershipPatch& patch) {
        return std::tuple{patch.uncovered_delta, patch.corner, -patch.gain};
    };
    for (const auto& patch : candidates) {
        if (patch.gain < gain_floor) continue;
        if (best.gain == 0 || preference(patch) < preference(best)) best = patch;
    }
    return best;
}

std::pair<AxisCell, AxisCell> cut_cell(AxisCell cell, OwnershipCut cut, ScenePartition::Axes& axes,
                                       const std::vector<std::array<double, 2>>& xy,
                                       const std::vector<OwnershipSample>& samples) {
    AxisCell inside, outside;
    inside.label = outside.label = cell.label;
    inside.low = outside.low = cell.low;
    inside.upper = outside.upper = cell.upper;
    cut.clip(inside);
    cut.opposite().clip(outside);
    for (int i : cell.stations) (cut.contains(xy[i]) ? inside : outside).stations.push_back(i);
    for (int i : cell.points) (cut.contains(samples[i].xy) ? inside : outside).points.push_back(i);
    // The manifest requires every child split to follow its parent.
    const int node = (int)axes.splits.size();
    auto& parent = axes.splits[cell.parent];
    (cell.high_child ? parent.high : parent.low) = node;
    axes.splits.push_back({cut.axis, cut.position, ~cell.label, ~cell.label});
    inside.parent = outside.parent = node;
    inside.high_child = cut.high;
    outside.high_child = !cut.high;
    return {std::move(inside), std::move(outside)};
}

}  // namespace

void refine_panorama_ownership(const ParsedDataset& ds, const Covisibility& cov,
                               const std::vector<Vec>& stations,
                               const std::vector<std::array<double, 2>>& xy, const std::vector<int>& owners,
                               int parts, const std::atomic<bool>* cancel, ScenePartition::Axes& axes) {
    if (axes.splits.empty() || !cov.has_tracks()) return;
    const auto samples = ownership_samples(ds, cov, stations, owners, parts, axes, cancel);
    const int voting_samples = (int)std::count_if(samples.begin(), samples.end(), [](const auto& sample) {
        return sample.reliability > 0;
    });
    if (voting_samples < 16) return;

    const auto& origin = xy.front();
    std::vector<float> local;
    for (const auto& p : xy) {
        local.push_back((float)(p[0] - origin[0]));
        local.push_back((float)(p[1] - origin[1]));
        local.push_back(0);
    }
    knn::KdTree3 tree(local.data(), xy.size());
    std::vector<double> distances;
    for (int i = 0; i < (int)xy.size(); i++) {
        float distance;
        if (tree.query(local.data() + 3 * i, i, 1, &distance) && distance > 0) distances.push_back(distance);
    }
    const double clearance = std::sqrt(median(distances)) / 2;
    if (clearance <= 0) return;

    const int min_samples = std::max(12, voting_samples / 2048);
    const double inf = std::numeric_limits<double>::infinity();
    std::vector<AxisCell> cells;
    AxisCell root;
    root.low.fill(-inf);
    root.upper.fill(inf);
    axis_cells(axes, 0, std::move(root), cells);
    std::vector<std::array<int, 2>> leaf_at(axes.splits.size());
    std::vector<int> cell_counts(parts);
    for (int i = 0; i < (int)cells.size(); i++) {
        const auto& cell = cells[i];
        cell_counts[cell.label]++;
        leaf_at[cell.parent][cell.high_child] = i;
    }
    auto cell_of = [&](const std::array<double, 2>& point) -> AxisCell& {
        for (int node = 0;;) {
            const auto& split = axes.splits[node];
            const bool high = point[split.axis] >= split.cut;
            const int child = high ? split.high : split.low;
            if (child < 0) return cells[leaf_at[node][high]];
            node = child;
        }
    };
    for (int i = 0; i < (int)xy.size(); i++) cell_of(xy[i]).stations.push_back(i);
    for (int i = 0; i < (int)samples.size(); i++) cell_of(samples[i].xy).points.push_back(i);

    for (int pass = 0; pass < 2; pass++) {
        bool changed = false;
        const size_t size = cells.size();
        for (size_t i = 0; i < size; i++) {
            stop_if(cancel);
            const auto& cell = cells[i];
            if (cell.stations.empty() || (int)cell.points.size() < min_samples) continue;
            const auto patch = best_ownership_patch(cell, cells, cell_counts, xy, samples, clearance, min_samples);
            if (patch.gain <= 0) continue;
            auto [inside, outside] = cut_cell(std::move(cells[i]), patch.first, axes, xy, samples);
            cells[i] = std::move(outside);
            if (patch.corner) {
                auto [corner, remainder] = cut_cell(std::move(inside), patch.second, axes, xy, samples);
                inside = std::move(corner);
                cell_counts[remainder.label]++;
                cells.push_back(std::move(remainder));
            }
            auto& parent = axes.splits[inside.parent];
            (inside.high_child ? parent.high : parent.low) = ~patch.label;
            inside.label = patch.label;
            cell_counts[patch.label]++;
            cells.push_back(std::move(inside));
            changed = true;
        }
        if (!changed) break;
    }
}

}  // namespace spirula
