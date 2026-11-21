#include "app/LidarTraining.h"

#include "data/PointCloudFile.h"
#include "sfm/geometry/LinAlg.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <stdexcept>

namespace app::lidar {
namespace {

struct Pose {
    double time;
    std::array<double, 3> origin;
};

std::vector<std::string> columns(std::string line) {
    if (!line.empty() && line.back() == '\r') line.pop_back();
    std::istringstream stream(line);
    std::vector<std::string> fields;
    for (std::string field; std::getline(stream, field, ',');) fields.push_back(std::move(field));
    return fields;
}

std::vector<Pose> read_poses(const std::string& path) {
    std::ifstream file(std::filesystem::u8path(path));
    std::string line;
    if (!std::getline(file, line)) throw std::runtime_error("Cannot read LiDAR trajectory: " + path);
    if (line.compare(0, 3, "\xef\xbb\xbf") == 0) line.erase(0, 3);
    const auto header = columns(line);
    std::array<size_t, 4> indices{};
    const char* names[] = {"time_sec", "x", "y", "z"};
    for (size_t i = 0; i < indices.size(); ++i) {
        const auto it = std::find(header.begin(), header.end(), names[i]);
        if (it == header.end()) throw std::runtime_error("Missing LiDAR trajectory column: " + std::string(names[i]));
        indices[i] = size_t(it - header.begin());
    }
    std::vector<Pose> poses;
    while (std::getline(file, line)) {
        if (line.empty() || line == "\r") continue;
        const auto fields = columns(line);
        double value[4];
        for (size_t i = 0; i < indices.size(); ++i) {
            size_t end = 0;
            const auto& field = fields.at(indices[i]);
            value[i] = std::stod(field, &end);
            if (end != field.size() || !std::isfinite(value[i]))
                throw std::runtime_error("Invalid number in LiDAR trajectory: " + path);
        }
        if (!poses.empty() && value[0] <= poses.back().time)
            throw std::runtime_error("LiDAR trajectory times must be strictly increasing");
        poses.push_back({value[0], {value[1], value[2], value[3]}});
    }
    if (poses.size() < 2) throw std::runtime_error("LiDAR trajectory needs at least two positions");
    return poses;
}

}  // namespace

TrainingScan::TrainingScan(const std::string& scan, const std::string& trajectory,
                           std::vector<double>* seed_xyz) {
    const std::vector<Pose> poses = read_poses(trajectory);
    spirula::cloud::Reader reader(scan);
    if (reader.info().format != spirula::cloud::Format::Las)
        throw std::runtime_error("LiDAR training requires a LAS or LAZ file with per-point time");
    constexpr double frame_seconds = 0.1;
    const double first = poses.front().time, last = poses.back().time;
    const double bins = std::ceil((last - first) / frame_seconds);
    if (!std::isfinite(bins) || bins > 1000000)
        throw std::runtime_error("LiDAR trajectory has an invalid duration");
    frames_.resize(std::max<size_t>(1, size_t(bins)));
    size_t after = 1;
    for (size_t i = 0; i < frames_.size(); ++i) {
        const double begin = first + double(i) * frame_seconds;
        const double time = 0.5 * (begin + std::min(last, begin + frame_seconds));
        while (after + 1 < poses.size() && poses[after].time < time) ++after;
        const Pose& a = poses[after - 1];
        const Pose& b = poses[after];
        if (b.time - a.time > 2.0) continue;
        const double w = (time - a.time) / (b.time - a.time);
        for (size_t k = 0; k < 3; ++k)
            frames_[i].origin[k] = (1 - w) * a.origin[k] + w * b.origin[k];
        frames_[i].valid = true;
    }
    std::mt19937 rng(42);
    std::bernoulli_distribution seed(0.03);
    size_t index = 0;
    reader.read([&](const spirula::e57::Point* points, size_t count) {
        for (size_t i = 0; i < count; ++i, ++index) {
            const auto& p = points[i];
            if (!std::isfinite(p.xyz[0]) || !std::isfinite(p.xyz[1]) || !std::isfinite(p.xyz[2])) continue;
            if (seed_xyz && seed(rng)) seed_xyz->insert(seed_xyz->end(), p.xyz, p.xyz + 3);
            if (index % 4 || !std::isfinite(p.gps_time) || p.gps_time < first || p.gps_time > last) continue;
            const size_t frame = std::min(frames_.size() - 1, size_t((p.gps_time - first) / frame_seconds));
            auto& target = frames_[frame];
            if (!target.valid) continue;
            target.points.push_back({float(p.xyz[0] - target.origin[0]),
                                     float(p.xyz[1] - target.origin[1]),
                                     float(p.xyz[2] - target.origin[2])});
            ++point_count_;
        }
    });
    frames_.erase(std::remove_if(frames_.begin(), frames_.end(),
                                [](const Frame& f) { return f.points.empty(); }), frames_.end());
    if (frames_.empty()) throw std::runtime_error("No scan point times overlap the LiDAR trajectory");
}

bool TrainingScan::sample(int step, const std::array<double, 3>& center, LidarTrainingView& view) const {
    std::mt19937 rng(static_cast<uint32_t>(step));
    const Frame& frame = frames_[std::uniform_int_distribution<size_t>(0, frames_.size() - 1)(rng)];
    const auto& p = frame.points[std::uniform_int_distribution<size_t>(0, frame.points.size() - 1)(rng)];
    const sfm::Vec3 forward = sfm::Vec3{p[0], p[1], p[2]}.normalized();
    if (forward.dot(forward) < 0.5) return false;
    const sfm::Vec3 up = std::abs(forward.z) < 0.98 ? sfm::Vec3{0,0,1} : sfm::Vec3{0,1,0};
    const sfm::Vec3 right = forward.cross(up).normalized();
    const sfm::Vec3 down = forward.cross(right);
    const sfm::Vec3 origin{frame.origin[0] - center[0], frame.origin[1] - center[1],
                           frame.origin[2] - center[2]};
    const sfm::Vec3 axes[] = {right, down, forward};
    for (int i = 0; i < 3; ++i)
        view.viewmat[i] = {float(axes[i].x), float(axes[i].y), float(axes[i].z), float(-axes[i].dot(origin))};
    view.viewmat[3] = {0, 0, 0, 1};
    view.frame_points = frame.points.size();
    view.samples.clear();
    view.samples.reserve(frame.points.size());
    for (const auto& pt : frame.points) {
        const sfm::Vec3 delta{pt[0], pt[1], pt[2]};
        const double z = forward.dot(delta);
        if (z < 0.1) continue;
        const double x = view.focal * right.dot(delta) / z + view.size * 0.5;
        const double y = view.focal * down.dot(delta) / z + view.size * 0.5;
        if (x >= 0 && y >= 0 && x < view.size && y < view.size)
            view.samples.push_back({float(x), float(y), float(z)});
    }
    return !view.samples.empty();
}

}  // namespace app::lidar
