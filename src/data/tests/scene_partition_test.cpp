// scene_partition_test -- data/ScenePartition.h over a synthetic corridor:
// cameras along a line looking down at a strip of points. The cut must fall
// across the corridor, the rings must sit at the cut, the file must round-trip,
// applying a part must keep exactly its frames, and the merge must keep each
// model's splats on its own side (checkpoint/SplatMerge.h).

#include "checkpoint/SplatMerge.h"
#include "checkpoint/SplatPly.h"
#include "data/JsonWrite.h"
#include "data/RegionProgram.h"
#include "data/ScenePartition.h"
#include "data/SceneTransform.h"
#include "external/stb_image.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <random>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace {

int g_failures = 0;

void check(bool ok, const std::string& what) {
    std::printf("%s %s\n", ok ? "ok  " : "FAIL", what.c_str());
    if (!ok) g_failures++;
}

// `n_cam` cameras at x = 0..n_cam-1, z = 5, looking straight down (-z in the
// OpenGL convention is the identity rotation), over points on z = 0.
struct Corridor {
    ParsedDataset ds;
    spirula::SparseStats tracks;
};

// `reach`: how far along x a camera sees; `keep`: the chance it tracks a
// point it sees, which makes per-point observer counts noisy.
Corridor make_corridor(int n_cam, int pts_per_cam, unsigned seed, double reach = 2.5,
                       double keep = 1.0) {
    Corridor c;
    ParsedDataset& d = c.ds;
    d.num_cameras = n_cam;
    std::mt19937 rng(seed);
    std::uniform_real_distribution<double> u(-1.0, 1.0);
    for (int i = 0; i < n_cam; i++) {
        d.camera_models.push_back(0);
        d.camera_distortions.push_back(0);
        d.image_filenames.push_back("/nowhere/img_" + std::to_string(1000 + i) + ".jpg");
        d.widths.push_back(500);
        d.heights.push_back(500);
        float m[12] = {1, 0, 0, (float)i, 0, 1, 0, 0, 0, 0, 1, 5};
        d.c2w.insert(d.c2w.end(), m, m + 12);
        // f = 500 over a 500 px frame: half-angle 26.6 degrees, so a camera
        // at height 5 sees x within +-2.5 of itself.
        const float k[4] = {500, 500, 250, 250};
        d.intrins.insert(d.intrins.end(), k, k + 4);
        for (int j = 0; j < 8; j++) d.dist_coeffs.push_back(0.0f);
        d.train_indices.push_back(i);
        c.tracks.image_names.push_back("img_" + std::to_string(1000 + i) + ".jpg");
    }
    c.tracks.track_beg.push_back(0);
    for (int i = 0; i < n_cam; i++)
        for (int j = 0; j < pts_per_cam; j++) {
            const double x = i + u(rng), y = 1.5 * u(rng), z = 0.2 * u(rng);
            d.points.xyz.insert(d.points.xyz.end(), {x, y, z});
            d.points.rgb.insert(d.points.rgb.end(), {128, 128, 128});
            for (int k = 0; k < n_cam; k++)
                if (std::fabs(k - x) <= reach && (keep >= 1.0 || 0.5 * (u(rng) + 1.0) < keep))
                    c.tracks.track_image.push_back(k);
            c.tracks.track_beg.push_back((int64_t)c.tracks.track_image.size());
            c.tracks.error.push_back(0.5f);
        }
    return c;
}

// The cut of a corridor must be a single change of label along x.
int label_changes(const std::vector<int32_t>& labels) {
    int n = 0;
    for (size_t i = 1; i < labels.size(); i++) n += labels[i] != labels[i - 1];
    return n;
}

spirula::SplatCloud splats_along_x(double x0, double x1, int n, int sh_degree, float tag) {
    spirula::SplatCloud c;
    c.num = n;
    c.sh_degree = sh_degree;
    for (int i = 0; i < n; i++) {
        const double x = x0 + (x1 - x0) * (i + 0.5) / n;
        c.means.insert(c.means.end(), {(float)x, tag, 0.0f});
        c.quats.insert(c.quats.end(), {1, 0, 0, 0});
        c.scales.insert(c.scales.end(), {-2, -2, -2});
        c.opacities.push_back(2.0f);
        c.features_dc.insert(c.features_dc.end(), {0.5f, 0.2f, 0.1f});
        const int64_t k = c.dim_sh() - 1;
        for (int64_t j = 0; j < k * 3; j++) c.features_sh.push_back(0.01f * (float)j);
    }
    return c;
}

void write_text(const fs::path& p, const std::string& s) {
    std::ofstream f(p);
    f << s;
}

void panorama_tests(const fs::path& tmp) {
    using namespace spirula;
    Corridor road = make_corridor(18, 24, 5, 3.5);
    std::fill(road.ds.camera_models.begin(), road.ds.camera_models.end(), 3);
    road.ds.gauge_oriented = true;
    const double cosine = std::cos(0.61), sine = std::sin(0.61);
    auto rotate = [&](double x, double y) { return std::array<double, 2>{cosine * x - sine * y, sine * x + cosine * y}; };
    road.ds.center = {100.0, 150.0, 20.0};
    for (int64_t i = 0; i < road.ds.num_cameras; i++) {
        auto q = rotate((double)i, i % 2 ? 0.5 : -0.5);
        road.ds.c2w[i * 12 + 3] = (float)q[0];
        road.ds.c2w[i * 12 + 7] = (float)q[1];
    }
    for (int64_t i = 0; i < road.ds.points.num(); i++) {
        auto q = rotate(road.ds.points.xyz[i * 3], road.ds.points.xyz[i * 3 + 1]);
        road.ds.points.xyz[i * 3] = q[0];
        road.ds.points.xyz[i * 3 + 1] = q[1];
    }
    PartitionOptions opt;
    opt.method = PartitionMethod::AxisAligned;
    opt.parts = 2;
    opt.ring_min_points = 1;
    opt.outside_seed_fraction = 0;
    const auto cov = build_covisibility(road.ds, &road.tracks, opt);
    const auto p = partition_scene(road.ds, cov, opt);
    graph::LabelCutOptions group_options;
    group_options.parts = opt.parts;
    group_options.min_part = group_options.min_final = 1;
    check(p.initial_frame_label == graph::cut_labels(cov.cameras, group_options),
          "preview labels come from the initial covisibility cut");
    check(p.axes && p.num_parts == 2 && label_changes(p.frame_label) == 1 && p.axes->splits[0].axis == 0 &&
              std::abs(p.axes->basis[0][0] * cosine + p.axes->basis[0][1] * sine) > 0.999,
          "rotated panorama road recovers the overall axis despite zigzagging stations");
    check(!p.ring[0].empty() && !p.ring[1].empty(), "road borrows panoramas across the cut");
    PartitionOptions adaptive_opt = opt;
    adaptive_opt.panorama_route = PanoramaRoute::Loop;
    const auto adaptive = partition_scene(road.ds, cov, adaptive_opt);
    check(adaptive.axes && adaptive.axes->road_fallback && adaptive.axes->basis == p.axes->basis &&
              adaptive.initial_frame_label == p.initial_frame_label && adaptive.frame_label == p.frame_label &&
              adaptive.point_label == p.point_label,
          "loop mode cuts a nearly linear trajectory across the road");
    const auto path = (tmp / "panorama.json").string();
    write_partition(adaptive, path, "panorama");
    const auto back = read_partition(path);
    check(back.axes && back.axes->road_fallback && back.axes->basis == p.axes->basis &&
              back.axes->splits[0].cut == p.axes->splits[0].cut &&
              back.part_points == p.part_points && back.ring == p.ring, "panorama geometry and tables round-trip");

    bool consistent = true;
    std::vector<std::string> paths;
    for (int k = 0; k < back.num_parts; k++) {
        auto region = back.region(k);
        RegionProgram program;
        std::string error;
        const double shift[] = {-200, -300, -40};
        consistent &= compile_region(*region, program, error, 2.0, shift);
        auto data = road.ds;
        PartitionApplied applied;
        consistent &= apply_partition(data, back, k, applied) &&
                      applied.frames_after == (int64_t)back.frames_of(k).size() &&
                      applied.points_after == (int64_t)back.part_points[k].size();
        auto cloud = splats_along_x(0, 1, 2, 0, (float)k);
        for (int s = 0; s < 2; s++) {
            const int image = s ? 17 : 0;
            for (int a = 0; a < 3; a++) cloud.means[s * 3 + a] = back.frame_centers[image * 3 + a];
        }
        paths.push_back((tmp / ("panorama_" + std::to_string(k) + ".ply")).string());
        write_splat_ply(cloud, paths.back());
        const auto frames = back.frames_of(k);
        for (size_t i = 0; i < back.point_label.size(); i++) {
            double source[3], train[3];
            for (int a = 0; a < 3; a++) {
                train[a] = 2 * road.ds.points.xyz[i * 3 + a];
                source[a] = road.ds.points.xyz[i * 3 + a] + road.ds.center[a];
            }
            const bool own = back.point_label[i] == k;
            if (std::abs(back.axes->basis[0][0] * source[0] + back.axes->basis[0][1] * source[1] - back.axes->splits[0].cut) > 1e-4)
                consistent &= program_contains(program, train) == own;
            consistent &= region->contains(source) == own;
            bool observed = false;
            for (int64_t j = cov.beg[i]; j < cov.beg[i + 1]; j++)
                observed |= std::binary_search(frames.begin(), frames.end(), cov.frame[j]);
            consistent &= std::binary_search(back.part_points[k].begin(), back.part_points[k].end(), (int64_t)i) == (observed && own);
        }
    }
    check(consistent, "point ownership, training ROI and visible owned seed lists agree");
    MergeStats merged_stats;
    const auto merged = merge_partition_splats(back, paths, merged_stats);
    check(merged.num == 2 && merged_stats.parts[0].kept == 1 && merged_stats.parts[1].kept == 1,
          "axis-aligned merge retains each model's own side");

    Corridor loop = make_corridor(32, 1, 1);
    std::fill(loop.ds.camera_models.begin(), loop.ds.camera_models.end(), 3);
    loop.tracks.track_image.clear();
    loop.tracks.track_beg.assign(1, 0);
    for (int i = 0; i < 32; i++) {
        const int j = (i + 3) % 32;
        const double t = -4 + j % 8;
        const double x[] = {t, 4, -t, -4}, y[] = {-4, t, 4, -t};
        const auto q = rotate(x[j / 8], y[j / 8]);
        loop.ds.c2w[i * 12 + 3] = (float)q[0];
        loop.ds.c2w[i * 12 + 7] = (float)q[1];
        loop.ds.points.xyz[i * 3] = q[0] * 0.8;
        loop.ds.points.xyz[i * 3 + 1] = q[1] * 0.8;
        for (int d = -2; d <= 2; d++) loop.tracks.track_image.push_back((i + d + 32) % 32);
        loop.tracks.track_beg.push_back((int64_t)loop.tracks.track_image.size());
    }
    opt.panorama_route = PanoramaRoute::Loop;
    opt.parts = 4;
    const auto loop_cov = build_covisibility(loop.ds, &loop.tracks, opt);
    const auto loop_part = partition_scene(loop.ds, loop_cov, opt);
    const auto& labels = loop_part.frame_label;
    const bool one_arc = label_changes(labels) + (labels.front() != labels.back()) == 4;
    check(loop_part.num_parts == 4 && one_arc && !loop_part.axes->road_fallback &&
              loop_part.axes->up_source == "trajectory" &&
              std::abs(loop_part.axes->basis[0][0] * cosine + loop_part.axes->basis[0][1] * sine) > 0.99,
          "rotated building loop recovers axes and keeps each part on one cyclic arc");

    ParsedDataset repeat;
    repeat.num_cameras = 64;
    repeat.gauge_oriented = true;
    const auto drift = rotate(0.07, 0.07);
    for (int i = 31; i >= 0; i--) {
        for (int clip : {1, 0}) {
            const float* pose = &loop.ds.c2w[i * 12];
            const size_t offset = repeat.c2w.size();
            repeat.c2w.insert(repeat.c2w.end(), pose, pose + 12);
            repeat.c2w[offset + 3] += (float)(clip * drift[0]);
            repeat.c2w[offset + 7] += (float)(clip * drift[1]);
            repeat.camera_models.push_back(3);
            repeat.image_filenames.push_back("/loop/clip" + std::to_string(clip) + "/frame_" + std::to_string(i) + ".jpg");
        }
    }
    const auto repeated = partition_scene(repeat, build_covisibility(repeat, nullptr, opt), opt);
    const auto& axes = *repeated.axes;
    const auto along_wall = [&](int a) { return std::abs(axes.basis[a][0] * cosine + axes.basis[a][1] * sine); };
    check(!axes.heading_fallback && std::max(along_wall(0), along_wall(1)) > 0.999,
          "loop follows numbered frames within each clip despite closer stations in another lap");

    Corridor rig = make_corridor(12, 16, 9);
    std::fill(rig.ds.camera_models.begin(), rig.ds.camera_models.end(), 1);
    const fs::path model = tmp / "native_rig" / "sparse" / "0";
    fs::create_directories(model);
    write_text(model / "cameras.txt", "1 OPENCV_FISHEYE 500 500 250 250 250 250 0 0 0 0\n");
    write_text(model / "rigs.txt", "# Rig calibration from spirula sfm, cam_from_rig per member as\n"
               "rig cam0 cam0 1 0 0 0 0 0 0 0 0\nrig cam1 cam0 0 0 1 0 0 0 0 0 0\n");
    std::string image_rows;
    for (int i = 0; i < 12; i++) {
        const std::string name = "cam" + std::to_string(i % 2) + "/" + std::to_string(10000 + i / 2) + ".jpg";
        rig.ds.image_filenames[i] = "/nowhere/" + name;
        image_rows += std::to_string(i + 1) + " 1 0 0 0 0 0 0 1 " + name + "\n\n";
    }
    write_text(model / "images.txt", image_rows);
    {
        std::ofstream point_rows(model / "points3D.txt");
        for (int64_t i = 0; i < rig.ds.points.num(); i++) {
            point_rows << i + 1 << " 0 0 0 128 128 128 0.5";
            for (int64_t j = rig.tracks.track_beg[i]; j < rig.tracks.track_beg[i + 1]; j++)
                point_rows << ' ' << rig.tracks.track_image[j] + 1 << " 0";
            point_rows << '\n';
        }
    }
    const auto native_tracks = read_sparse_stats((tmp / "native_rig").string(), true);
    check(native_tracks.image_rig_frame == std::vector<int32_t>{0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5},
          "native rigs.txt resolves synchronized dual fisheye stations");
    opt.panorama_route = PanoramaRoute::Road;
    opt.parts = 0;
    opt.max_images = 6;
    const auto rig_cov = build_covisibility(rig.ds, &native_tracks, opt);
    const auto rig_part = partition_scene(rig.ds, rig_cov, opt);
    bool whole = rig_cov.cameras.n() == 6;
    for (int k = 0; k < rig_part.num_parts; k++) {
        const auto frames = rig_part.frames_of(k);
        whole &= frames.size() <= 6 && frames.size() % 2 == 0;
        for (size_t i = 0; i + 1 < frames.size(); i += 2)
            whole &= frames[i] % 2 == 0 && frames[i + 1] == frames[i] + 1;
    }
    check(whole, "split panoramas stay together in core and ring under the image cap");

    Corridor gain = make_corridor(3, 1, 3);
    std::fill(gain.ds.camera_models.begin(), gain.ds.camera_models.end(), 3);
    gain.ds.points.xyz = {0.1, 0, 0, 0.2, 0, 0, 2.2, 0, 0};
    gain.tracks.track_beg = {0, 2, 3, 4};
    gain.tracks.track_image = {0, 1, 2, 2};
    opt.max_images = 2;
    opt.ring_fraction = 0;
    const auto gain_part = partition_scene(gain.ds, build_covisibility(gain.ds, &gain.tracks, opt), opt);
    check(gain_part.ring[gain_part.frame_label[0]] == std::vector<int32_t>{2},
          "budgeted ring prefers a new owned point over repeated coverage");

    Corridor distant = make_corridor(2, 2, 5);
    std::fill(distant.ds.camera_models.begin(), distant.ds.camera_models.end(), 3);
    distant.ds.c2w[11] = distant.ds.c2w[23] = 0;
    distant.ds.c2w[15] = 20;
    distant.ds.points.xyz = {0.1, 0, 0, 1, 0, 0, 1.2, 0, 0, 19.9, 0, 0};
    distant.tracks.track_beg = {0, 2, 3, 4, 5};
    distant.tracks.track_image = {0, 1, 1, 1, 1};
    PartitionOptions distant_opt = opt;
    distant_opt.parts = 2;
    distant_opt.ring_fraction = 0.1f;
    distant_opt.ring_min_points = 2;
    const auto distant_cov = build_covisibility(distant.ds, &distant.tracks, distant_opt);
    const auto distant_part = partition_scene(distant.ds, distant_cov, distant_opt);
    check(distant_part.ring[distant_part.frame_label[0]] == std::vector<int32_t>{1},
          "a low-share station can cover previously unseen owned points");
    distant_opt.ring_min_points = 3;
    const auto below_gain = partition_scene(distant.ds, distant_cov, distant_opt);
    check(below_gain.ring[below_gain.frame_label[0]].empty(), "coverage threshold counts only newly seen points");
    distant_opt.ring_min_points = 2;
    distant_opt.max_images = 1;
    const auto full = partition_scene(distant.ds, distant_cov, distant_opt);
    check(full.ring[full.frame_label[0]].empty(), "coverage stations fit within the remaining image budget");

    Corridor coverage = make_corridor(6, 1, 1);
    std::fill(coverage.ds.camera_models.begin(), coverage.ds.camera_models.end(), 3);
    coverage.ds.points.xyz = {2.7, 1, 0};
    coverage.ds.points.rgb = {128, 128, 128};
    coverage.tracks.track_beg = {0, 1};
    coverage.tracks.track_image = {2};
    opt.parts = 2;
    const auto cover = partition_scene(coverage.ds, build_covisibility(coverage.ds, &coverage.tracks, opt), opt);
    check(cover.point_label[0] == cover.frame_label[2], "cut score keeps a surface with its only core observer");
    const auto proximity = build_covisibility(coverage.ds, nullptr, opt);
    check(proximity.source == CovisibilitySource::Proximity && !proximity.has_tracks() &&
              partition_scene(coverage.ds, proximity, opt).num_parts == 2,
          "pose-only panoramas use station proximity");

    ScenePartition::Axes axis;
    axis.basis = {{{1, 0, 0}, {0, 1, 0}, {0, 0, 1}}};
    axis.splits.push_back({0, 2, ~0, ~1});
    const double seam[] = {2, 1, 0};
    check(axis.owner(seam) == 1 && axis.region(1)->contains(seam) && !axis.region(0)->contains(seam),
          "shared plane belongs exactly to the high side");
}

}  // namespace

int main() {
    using namespace spirula;
    const Corridor corr = make_corridor(60, 40, 5);
    const ParsedDataset& ds = corr.ds;

    // ---- tracks -> two parts ----
    PartitionOptions opt;
    opt.parts = 2;
    Covisibility cov = build_covisibility(ds, &corr.tracks, opt);
    check(cov.source == CovisibilitySource::Tracks, "auto takes the tracks when they exist");
    check(cov.num_points() == ds.points.num() && cov.has_tracks(), "one track row per point");
    check(cov.cameras.n() == 60 && cov.cameras.adj.size() > 0, "camera graph built");

    ScenePartition p = partition_scene(ds, cov, opt);
    check(p.num_parts == 2, "two parts asked, two made");
    check(label_changes(p.frame_label) == 1, "the cut is one boundary along the corridor");
    check(p.core_count(0) >= p.core_count(1), "largest part is labelled 0");
    // Which label ended up on which end of the corridor.
    const int L = p.frame_label.front(), R = 1 - L;
    {
        // Where the cut is, and that the rings sit right at it.
        int cut = 0;
        for (size_t i = 1; i < p.frame_label.size(); i++)
            if (p.frame_label[i] != p.frame_label[i - 1]) cut = (int)i;
        check(cut >= 24 && cut <= 36, "cut near the middle (at camera " + std::to_string(cut) + ")");
        bool rings_ok = !p.ring[0].empty() && !p.ring[1].empty();
        for (int32_t f : p.ring[(size_t)L]) rings_ok = rings_ok && f >= cut && f < cut + 6;
        for (int32_t f : p.ring[(size_t)R]) rings_ok = rings_ok && f < cut && f >= cut - 6;
        check(rings_ok, "rings are the few cameras just across the cut (" +
                            std::to_string(p.ring[0].size()) + " / " +
                            std::to_string(p.ring[1].size()) + ")");
        // Points follow: one boundary along x, near the cut.
        int wrong = 0;
        for (int64_t i = 0; i < ds.points.num(); i++) {
            const double x = ds.points.xyz[(size_t)i * 3];
            const int want = x < cut - 1.5 ? L : (x > cut + 1.5 ? R : -1);
            if (want >= 0 && p.point_label[(size_t)i] != want) wrong++;
        }
        check(wrong == 0, "points away from the cut belong to the side they are on (" +
                              std::to_string(wrong) + " wrong)");
        // The field: one boundary along the corridor axis, and the nearer
        // part far away.
        std::vector<int32_t> along;
        for (double x = 0; x < 60; x += 0.5) along.push_back(p.field->label(x, 0.0, 0.0));
        check(label_changes(along) == 1, "ownership field has one boundary along the corridor");
        check(p.field->label(-1000.0, 0.0, 0.0) == L && p.field->label(1000.0, 0.0, 0.0) == R,
              "far away, the nearer part owns the point");
        check(p.frame_centers.size() == (size_t)ds.num_cameras * 3, "camera centres recorded");
        check(p.cut_fraction > 0 && p.cut_fraction < 0.2,
              "cut fraction is small (" + std::to_string(p.cut_fraction) + ")");
        // Seeds: part 0 seeds from everything it owns plus what its ring sees.
        const std::vector<int64_t>& s0 = p.part_points[0];
        bool has_own = false, has_borrowed = false;
        for (int64_t i : s0) {
            has_own = has_own || p.point_label[(size_t)i] == 0;
            has_borrowed = has_borrowed || p.point_label[(size_t)i] == 1;
        }
        check(has_own && has_borrowed && s0.size() < (size_t)ds.points.num(),
              "part 0 seeds from its own points and its ring's, not the whole cloud");
    }

    // ---- a surface both parts see: owners must not interleave ----
    {
        Corridor room = make_corridor(40, 60, 11, 30.0, 0.1);
        const Covisibility rc = build_covisibility(room.ds, &room.tracks, PartitionOptions{});
        PartitionOptions ro;
        ro.method = PartitionMethod::ViewGraph;
        ro.parts = 2;
        const ScenePartition rp = partition_scene(room.ds, rc, ro);
        // Interleaving is what blurs a merge: the share of each point's ten
        // nearest points owned by another part.
        const int64_t n = room.ds.points.num();
        const double* X = room.ds.points.xyz.data();
        double foreign = 0;
        for (int64_t i = 0; i < n; i++) {
            std::vector<std::pair<double, int64_t>> d;
            for (int64_t j = 0; j < n; j++) {
                if (j == i) continue;
                double e = 0;
                for (int r = 0; r < 3; r++) e += (X[i * 3 + r] - X[j * 3 + r]) * (X[i * 3 + r] - X[j * 3 + r]);
                d.push_back({e, j});
            }
            std::partial_sort(d.begin(), d.begin() + 10, d.end());
            for (int k = 0; k < 10; k++)
                foreign += rp.point_label[(size_t)d[(size_t)k].second] != rp.point_label[(size_t)i];
        }
        foreign /= 10.0 * (double)n;
        char buf[64];
        std::snprintf(buf, sizeof buf, "%.3f", foreign);
        check(rp.num_parts == 2 && foreign < 0.03,
              std::string("every camera sees the floor: owners do not interleave (") + buf +
                  " of neighbours foreign)");
    }

    // ---- region masks: a camera over the inside keeps its pixels ----
    {
        Corridor c = make_corridor(40, 60, 5);
        const int64_t n = c.ds.points.num();
        std::vector<uint8_t> inside((size_t)n);
        for (int64_t i = 0; i < n; i++) inside[(size_t)i] = c.ds.points.xyz[(size_t)i * 3] < 20.0;
        const fs::path dir = fs::temp_directory_path() / "spirula_region_masks_test";
        const std::vector<std::string> files =
            write_region_masks(c.ds, c.ds.points.xyz.data(), n, inside.data(), dir.string(), false);
        auto kept_in = [&](const std::vector<std::string>& set, int cam) {
            int w = 0, h = 0, ch = 0;
            stbi_uc* img = stbi_load(set[(size_t)cam].c_str(), &w, &h, &ch, 1);
            if (!img) return -1.0;
            double k = 0;
            for (int i = 0; i < w * h; i++) k += img[i] != 0;
            stbi_image_free(img);
            return k / (w * h);
        };
        auto kept = [&](int cam) { return kept_in(files, cam); };
        check(files.size() == 40 && kept(5) > 0.99, "region masks: a camera over the inside keeps everything");
        // Pixels no point falls near stay kept: here the strip's two sides.
        check(kept(35) < 0.35, "region masks: one over the outside keeps little but what nothing covers");
        check(kept(20) > 0.3 && kept(20) < 0.9, "region masks: one over the boundary keeps its inside half and a margin");

        // Given the region, the sides are rays that miss it.
        BoxRegion box;
        box.center[0] = 9.5;
        box.half[0] = 10.5;
        box.half[1] = 1.5;
        box.half[2] = 0.2;
        const std::vector<std::string> boxed = write_region_masks(
            c.ds, c.ds.points.xyz.data(), n, inside.data(), (dir / "boxed").string(), false, nullptr, &box);
        // The strip is 300 of the 500 px across, plus a 12 px margin a side.
        const double k5 = kept_in(boxed, 5);
        check(k5 > 0.6 && k5 < 0.75, "region masks: a camera over the inside keeps the region and a margin, not "
                                     "the sides (" + std::to_string(k5) + ")");
        check(kept_in(boxed, 35) == 0.0, "region masks: one wholly past the region keeps nothing");
        std::error_code ec;
        fs::remove_all(dir, ec);
    }

    // ---- max-images mode and the other sources ----
    {
        PartitionOptions o2;
        o2.max_images = 15;
        ScenePartition p2 = partition_scene(ds, cov, o2);
        bool sizes_ok = p2.num_parts >= 4;
        for (int k = 0; k < p2.num_parts; k++) sizes_ok = sizes_ok && p2.core_count(k) <= 15;
        check(sizes_ok, "max-images mode: " + std::to_string(p2.num_parts) + " parts, all <= 15");
        check(label_changes(p2.frame_label) == p2.num_parts - 1,
              "every part is one contiguous run of the corridor");

        PartitionOptions o3 = opt;
        o3.source = CovisibilitySource::Projection;
        Covisibility c3 = build_covisibility(ds, &corr.tracks, o3);
        check(c3.source == CovisibilitySource::Projection && c3.has_tracks(),
              "projection covisibility from the seed cloud");
        ScenePartition p3 = partition_scene(ds, c3, o3);
        check(p3.num_parts == 2 && label_changes(p3.frame_label) == 1,
              "projection source cuts the corridor once too");

        PartitionOptions o4 = opt;
        o4.source = CovisibilitySource::Proximity;
        Covisibility c4 = build_covisibility(ds, nullptr, o4);
        check(c4.source == CovisibilitySource::Proximity && !c4.has_tracks(),
              "proximity covisibility needs no points");
        ScenePartition p4 = partition_scene(ds, c4, o4);
        check(p4.num_parts == 2 && label_changes(p4.frame_label) == 1,
              "proximity source cuts the corridor once");
        check(!p4.ring[0].empty() && !p4.ring[1].empty(), "proximity rings from graph edges");

        ParsedDataset bare = ds;
        bare.points = ColmapPoints3D{};
        Covisibility c5 = build_covisibility(bare, nullptr, opt);
        check(c5.source == CovisibilitySource::Proximity, "auto falls through to proximity");
    }

    // ---- files, and applying a part ----
    const fs::path tmp = fs::temp_directory_path() / "spirula_scene_partition_test";
    std::error_code ec;
    fs::remove_all(tmp, ec);
    fs::create_directories(tmp / "dataset", ec);
    panorama_tests(tmp);
    const std::string json = (tmp / "dataset" / "partition.json").string();
    write_partition(p, json, (tmp / "dataset").string());
    {
        std::string dataset;
        ScenePartition back = read_partition(json, &dataset);
        check(dataset == (tmp / "dataset").string(), "dataset path recorded");
        check(back.num_parts == p.num_parts && back.frame_label == p.frame_label &&
                  back.frame_names == p.frame_names && back.ring == p.ring,
              "frames round-trip through json");
        check(back.point_label == p.point_label && back.part_points == p.part_points,
              "point tables round-trip through bin");
        auto same = [](const std::vector<float>& a, const std::vector<float>& b) {
            return a.size() == b.size() &&
                   std::memcmp(a.data(), b.data(), a.size() * sizeof(float)) == 0;
        };
        check(back.field && same(back.field->seeds, p.field->seeds) &&
                  same(back.field->nodes, p.field->nodes),
              "field round-trips through bin");
        check(back.frame_centers == p.frame_centers, "camera centres round-trip through bin");
        check(back.options.parts == 2 && back.source == CovisibilitySource::Tracks,
              "options and source round-trip");

        // The part holding camera 0, so a renamed camera 0 below is its loss.
        ParsedDataset part0 = ds;
        PartitionApplied a;
        check(apply_partition(part0, back, L, a), "apply the first camera's part");
        const std::vector<int32_t> want = back.frames_of(L);
        check(a.frames_after == (int64_t)want.size() && a.core == back.core_count(L) &&
                  a.ring == (int64_t)back.ring[(size_t)L].size() && a.missing == 0,
              "a part keeps exactly core + ring frames");
        bool names_ok = part0.num_cameras == (int64_t)want.size();
        for (size_t i = 0; names_ok && i < want.size(); i++)
            names_ok = fs::path(part0.image_filenames[i]).filename().string() ==
                       back.frame_names[(size_t)want[i]];
        check(names_ok, "kept frames are the part's, in order");
        check(part0.c2w.size() == (size_t)part0.num_cameras * 12 &&
                  part0.intrins.size() == (size_t)part0.num_cameras * 4 &&
                  part0.train_indices.size() == (size_t)part0.num_cameras,
              "per-frame arrays and train indices follow");
        check(part0.points.num() == (int64_t)back.part_points[(size_t)L].size() &&
                  part0.points.rgb.size() == part0.points.xyz.size(),
              "seed points are the part's");
        // A frame the dataset lacks is counted, not fatal.
        ParsedDataset fewer = ds;
        fewer.image_filenames[0] = "/nowhere/renamed.jpg";
        PartitionApplied b;
        apply_partition(fewer, back, L, b);
        check(b.missing == 1 && b.frames_after == a.frames_after - 1, "a renamed frame is reported missing");
        check(!apply_partition(fewer, back, 7, b), "out-of-range part is refused");
    }

    // ---- merge ----
    {
        // Part 0's model covers the whole corridor (as a ring-trained model
        // would), part 1's too; each keeps only its own side. Part 1 was
        // trained centred at x=30 (scene_transform.json), degree 1 vs 0.
        const fs::path r0 = tmp / "outputs" / "run_a" / "step-000000200.ckpt";
        const fs::path r1 = tmp / "outputs" / "run_b" / "step-000000300.ckpt";
        fs::create_directories(r0, ec);
        fs::create_directories(r1, ec);
        SplatCloud c0 = splats_along_x(-2, 62, 640, 0, 0.0f);
        SplatCloud c1 = splats_along_x(-2 - 30, 62 - 30, 640, 1, 0.5f);
        write_splat_ply(c0, (r0 / "splat.ply").string());
        write_splat_ply(c1, (r1 / "splat.ply").string());
        write_text(tmp / "outputs" / "run_a" / "config.json",
                   "{\"partition\": " + json_quote(json) + ", \"partition_part\": 0}");
        write_text(tmp / "outputs" / "run_b" / "config.json",
                   "{\"partition\": " + json_quote(json) + ", \"partition_part\": 1}");
        write_text(tmp / "outputs" / "run_stale" / "config.json",
                   "{\"partition\": " + json_quote(json) + ", \"partition_part\": 1}");
        SceneTransform T;
        T.t[0] = -30.0;
        const double centre[3] = {30, 0, 0};
        write_text(tmp / "outputs" / "run_b" / "scene_transform.json",
                   scene_transform_json(T, "median", centre));
        SceneTransform back_T;
        check(read_scene_transform_json((tmp / "outputs" / "run_b" / "scene_transform.json").string(), back_T) &&
                  std::fabs(back_T.t[0] + 30.0) < 1e-9 && std::fabs(back_T.scale - 1.0) < 1e-12,
              "scene_transform.json reads back");

        std::vector<std::string> runs = find_partition_runs((tmp / "outputs").string(), json, 2);
        check(runs.size() == 2 && fs::path(runs[0]).filename() == "run_a" &&
                  fs::path(runs[1]).filename() == "run_b",
              "runs found by their config.json; the one without a model is skipped");

        MergeStats st;
        SplatCloud merged = merge_partition_splats(p, runs, st);
        check(st.parts[0].read == 640 && st.parts[1].read == 640, "both models read");
        check(st.parts[0].kept + st.parts[1].kept == merged.num && merged.num > 0,
              "merged count is the sum of what each part kept");
        check(st.parts[0].kept > 200 && st.parts[1].kept > 200 && merged.num < 700,
              "each side kept roughly half (" + std::to_string(st.parts[0].kept) + " + " +
                  std::to_string(st.parts[1].kept) + ")");
        // The merge orients each splat toward the nearest camera of its part
        // (data/RegionProgram.h splat_normal); the same query here.
        bool sides_ok = true;
        int64_t from1 = 0;
        for (int64_t i = 0; i < merged.num; i++) {
            const float x = merged.means[(size_t)i * 3], tag = merged.means[(size_t)i * 3 + 1];
            const int from = tag > 0.25f ? 1 : 0;
            double best = 1e300, to[3] = {0, 0, 1};
            for (int32_t f : p.frames_of(from)) {
                const float* c = &p.frame_centers[(size_t)f * 3];
                const double d[3] = {c[0] - x, c[1] - tag, c[2]};
                const double d2 = d[0] * d[0] + d[1] * d[1] + d[2] * d[2];
                if (d2 < best) { best = d2; for (int k = 0; k < 3; k++) to[k] = d[k]; }
            }
            double n[3];
            splat_normal(&merged.quats[(size_t)i * 4], &merged.scales[(size_t)i * 3], to, n);
            const double q[3] = {x, tag, 0.0};
            const int owner = p.field->label(q, n);
            from1 += from;
            sides_ok = sides_ok && owner == from;
        }
        check(sides_ok && from1 == st.parts[1].kept,
              "every merged splat lies in the region of the part that made it, "
              "part 1 moved back by its scene transform");
        check(merged.sh_degree == 1 &&
                  merged.features_sh.size() == (size_t)merged.num * 3 * 3,
              "SH padded to the highest degree present");
        // Part 0's padded coefficients are zero, part 1's are its own.
        bool pad_ok = true;
        for (int64_t i = 0; i < merged.num; i++) {
            const bool from = merged.means[(size_t)i * 3 + 1] > 0.25f;
            const float v = merged.features_sh[(size_t)i * 9 + 4];
            pad_ok = pad_ok && (from ? std::fabs(v - 0.04f) < 1e-6f : v == 0.0f);
        }
        check(pad_ok, "padding is zeros, real coefficients survive");
        std::vector<std::string> one = {runs[0], ""};
        SplatCloud half = merge_partition_splats(p, one, st);
        check(half.num == st.parts[0].kept && st.parts[1].ply.empty(), "a missing part is skipped");
    }
    fs::remove_all(tmp, ec);

    if (g_failures == 0) std::printf("scene_partition_test: OK\n");
    return g_failures ? 1 : 0;
}
