#include "sfm/map/PhotoCoverage.h"

#include <cstdio>

int main() {
    sfm::Reconstruction model;
    model.cameras[1] = sfm::Camera::defaultFor(1, 400, 400, 100, sfm::CamModel::ThinPrismFisheye);
    model.cameras[2] = sfm::Camera::defaultFor(2, 400, 400, 200, sfm::CamModel::OpenCV);
    for (uint32_t id = 1; id <= 5; ++id) {
        sfm::Image& image = model.images[id];
        image.id = id;
        image.camera_id = id == 2 || id == 3 ? 2 : 1;
        image.registered = true;
        for (int cell = 0; cell < 16; ++cell) for (int j = 0; j < 4; ++j) {
            image.points2D.push_back({cell % 4 * 100.0 + 20 + j, cell / 4 * 100.0 + 20 + j});
            image.point3D_ids.push_back(sfm::kInvalidPoint3D);
        }
    }
    for (uint32_t i = 0; i < 64; ++i) {
        std::vector<sfm::TrackElement> covered{{1, i}, {2, i}};
        if (i < 56) covered.push_back({3, i});
        model.addPoint3D({0, 0, 1}, covered);
        model.addPoint3D({0, 0, 2}, {{4, i}, {5, i}});
    }
    if (sfm::removeCoveredFisheyes(model, {}, .8) != 0 || model.images.size() != 5) return 1;
    if (sfm::removeCoveredFisheyes(model, {2, 3}, .8) != 1 || model.images.count(1) ||
        model.images.size() != 4 || model.points3D.size() != 120) return 1;
    for (const auto& [id, point] : model.points3D)
        for (const auto& observation : point.track)
            if (model.images.at(observation.image_id).point3D_ids.at(observation.point2D_idx) != id) return 1;
    for (uint32_t i = 56; i < 64; ++i)
        if (model.images.at(2).point3D_ids[i] != sfm::kInvalidPoint3D) return 1;
    std::puts("Mixed photo coverage: PASS");
    return 0;
}
