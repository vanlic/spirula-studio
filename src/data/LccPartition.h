#pragma once

#include "data/ScenePartition.h"

namespace spirula {

void partition_lcc(const ParsedDataset& ds, const SparseStats* tracks, ScenePartition& part,
                   const PartitionLog& log, const std::atomic<bool>* cancel);

}  // namespace spirula
