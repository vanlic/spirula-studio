#pragma once

#include "data/LccPartition.h"

namespace app {

spirula::LccProjectionMasks load_lcc_image_masks(const ParsedDataset& ds, bool flip_mask, bool keep_aperture,
                                                 const spirula::PartitionLog& log = {},
                                                 const std::atomic<bool>* cancel = nullptr);

}  // namespace app
