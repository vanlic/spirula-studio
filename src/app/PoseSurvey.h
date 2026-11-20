#pragma once

#include <array>
#include <string>
#include <vector>

namespace app::survey {

inline constexpr char kReportFile[] = "ba_report.json";
inline constexpr char kIntrinsicsExample[] =
    "# folder model fx fy cx cy k1 k2 k3 k4\n"
    "0 OPENCV_FISHEYE 785 785 1440 1440 0 0 0 0\n"
    "1 OPENCV_FISHEYE 785 785 1440 1440 0 0 0 0";

struct PoseTable {
    std::string file, camera;
    std::array<int, 8> columns{0, 1, 2, 3, 4, 5, 6, 7};
    bool header = true;
    bool world_to_camera = false;
};

std::vector<std::string> previewColumns(const std::string& path, bool header);
int runCommand(int argc, char** argv);

}  // namespace app::survey
