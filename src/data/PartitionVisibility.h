#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace spirula {

template <class T>
void normalize_observation_weights(T* weights, size_t count, std::vector<T>& scratch) {
    if (count == 0) return;
    scratch.assign(weights, weights + count);
    const auto cap = scratch.begin() + count * 95 / 100;
    std::nth_element(scratch.begin(), cap, scratch.end());
    double sum = 0;
    for (size_t i = 0; i < count; i++) sum += (weights[i] = std::min(weights[i], *cap));
    for (size_t i = 0; i < count; i++) weights[i] = (T)(weights[i] / sum);
}

inline bool keep_outside_seed(uint64_t index, uint32_t keep) {
    return ((uint32_t)(index * 2654435761u) >> 16) < keep;
}

}  // namespace spirula
