#pragma once

#include "data/DatasetParser.h"
#include "engine/Engine.h"
#include <array>
#include <string>
#include <vector>

namespace app::lidar {

class TrainingScan {
public:
    TrainingScan(const std::string& scan, const std::string& poses,
                 ColmapPoints3D* seeds, bool matched_seeds = false);
    size_t frame_count() const { return frames_.size(); }
    size_t point_count() const { return point_count_; }
    bool sample(int step, const std::array<double, 3>& center, LidarTrainingView& view) const;

private:
    struct Frame {
        std::array<double, 3> origin;
        bool valid = false;
        std::vector<std::array<float, 3>> points;
    };
    std::vector<Frame> frames_;
    size_t point_count_ = 0;
};

}  // namespace app::lidar
