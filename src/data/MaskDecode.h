#pragma once

#include <cstdint>
#include <string>

namespace spirula {

void decode_mask_into(const std::string& path, int dst_h, int dst_w, bool flip,
                      float boundary_offset_frac, uint8_t* dst, int turns_cw = 0);

}  // namespace spirula
