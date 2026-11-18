#pragma once

#include "data/ScenePartition.h"

namespace spirula {

void refine_panorama_ownership(const ParsedDataset& ds, const Covisibility& cov,
                               const std::vector<std::array<double, 3>>& stations,
                               const std::vector<std::array<double, 2>>& xy, const std::vector<int>& owners,
                               int parts, const std::atomic<bool>* cancel, ScenePartition::Axes& axes);

}  // namespace spirula
