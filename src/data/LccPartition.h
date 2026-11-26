#pragma once

#include "data/ScenePartition.h"

#include <array>
#include <filesystem>

namespace spirula {

class LccLidarCache {
public:
    void prepare(const std::string& path);
    void retain_full(std::vector<double> xyz);
    const std::vector<double>& sample(const ParsedDataset& ds, const PartitionOptions& opt,
                                      const PartitionLog& log, const std::atomic<bool>* cancel);

private:
    std::string _path;
    std::filesystem::file_time_type _modified{};
    uintmax_t _size = 0;
    std::vector<double> _full;
    std::vector<double> _sample;
    std::array<double, 3> _center{};
    int _limit = 0;
    double _fraction = 0;
};

struct LccFrameProjection {
    int width = 0, height = 0;
    std::vector<int> pixel_voxel;  // Row-major, -1 for background.
    std::vector<int> part_pixels;
};

class LccProjectionDebug {
public:
    explicit operator bool() const { return bool(_data); }
    LccFrameProjection project(const ParsedDataset& ds, int64_t camera,
                               const std::atomic<bool>* cancel = nullptr) const;
    const std::vector<int>& voxel_parts(int voxel) const;

private:
    struct Data;
    std::shared_ptr<const Data> _data;
    friend void partition_lcc(const ParsedDataset&, const SparseStats*, ScenePartition&,
                              const PartitionLog&, const std::atomic<bool>*, LccLidarCache*, LccProjectionDebug*);
};

void partition_lcc(const ParsedDataset& ds, const SparseStats* tracks, ScenePartition& part,
                   const PartitionLog& log, const std::atomic<bool>* cancel,
                   LccLidarCache* lidar_cache, LccProjectionDebug* projection);

}  // namespace spirula
