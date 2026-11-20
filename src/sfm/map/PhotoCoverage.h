#pragma once

#include "sfm/core/Model.h"

#include <algorithm>
#include <array>
#include <set>

namespace sfm {

inline size_t removeCoveredFisheyes(Reconstruction& model, const std::set<uint32_t>& detail, double overlap) {
    std::set<uint64_t> shared;
    for (const auto& [id, image] : model.images) {
        if (!image.registered || !detail.count(id)) continue;
        for (uint64_t p : image.point3D_ids) if (p != kInvalidPoint3D) shared.insert(p);
    }
    std::set<uint32_t> drop;
    for (const auto& [id, image] : model.images) {
        if (!image.registered || detail.count(id) || !model.cameras.at(image.camera_id).isFisheye()) continue;
        const auto& camera = model.cameras.at(image.camera_id);
        std::array<int, 16> total{}, covered{};
        for (size_t i = 0; i < image.point3D_ids.size(); ++i) {
            const uint64_t p = image.point3D_ids[i];
            if (p == kInvalidPoint3D) continue;
            const Vec2 xy = image.points2D[i];
            const int x = std::clamp(int(4 * xy.x / camera.width), 0, 3);
            const int y = std::clamp(int(4 * xy.y / camera.height), 0, 3);
            ++total[y * 4 + x];
            if (shared.count(p)) ++covered[y * 4 + x];
        }
        int count = 0, hit = 0, cells = 0, good = 0;
        for (size_t i = 0; i < total.size(); ++i) {
            count += total[i]; hit += covered[i];
            if (total[i] < 4) continue;
            ++cells;
            good += covered[i] >= overlap * total[i];
        }
        if (count >= 40 && hit >= overlap * count && cells >= 4 && good >= overlap * cells)
            drop.insert(id);
    }
    if (drop.empty()) return 0;
    for (uint32_t id : drop) model.images.erase(id);
    for (auto it = model.points3D.begin(); it != model.points3D.end();) {
        auto& track = it->second.track;
        track.erase(std::remove_if(track.begin(), track.end(), [&](const TrackElement& p) { return drop.count(p.image_id) != 0; }), track.end());
        if (track.size() >= 2) { ++it; continue; }
        for (const auto& p : track) model.images.at(p.image_id).point3D_ids[p.point2D_idx] = kInvalidPoint3D;
        it = model.points3D.erase(it);
    }
    return drop.size();
}

}  // namespace sfm
