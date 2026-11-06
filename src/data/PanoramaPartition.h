#pragma once

#include "data/ScenePartition.h"

namespace spirula {

void panorama_proximity(const ParsedDataset& ds, Covisibility& cov, int neighbours,
                        const std::atomic<bool>* cancel);

void partition_panorama(const ParsedDataset& ds, const Covisibility& cov, ScenePartition& p,
                        const PartitionLog& log, const std::atomic<bool>* cancel);

}  // namespace spirula
