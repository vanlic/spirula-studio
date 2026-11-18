#pragma once

#include "data/ScenePartition.h"
#include "sfm/geometry/LinAlg.h"

#include <algorithm>

namespace spirula::panorama {

using Vec = std::array<double, 3>;

inline sfm::Vec3 as_vec(const Vec& v) { return {v[0], v[1], v[2]}; }

inline double project(const Vec& axis, const double* p) {
    return axis[0] * p[0] + axis[1] * p[1] + axis[2] * p[2];
}

inline double median(std::vector<double>& values) {
    if (values.empty()) return 0;
    const auto mid = values.begin() + values.size() / 2;
    std::nth_element(values.begin(), mid, values.end());
    return *mid;
}

inline void stop_if(const std::atomic<bool>* cancel) {
    if (graph::cancelled(cancel)) throw PartitionCancelled();
}

inline Vec source_point(const ParsedDataset& ds, size_t i) {
    return {ds.points.xyz[i * 3] + ds.center[0], ds.points.xyz[i * 3 + 1] + ds.center[1],
            ds.points.xyz[i * 3 + 2] + ds.center[2]};
}

}  // namespace spirula::panorama
