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

void partition_lcc(const ParsedDataset& ds, const SparseStats* tracks, ScenePartition& part,
                   const PartitionLog& log, const std::atomic<bool>* cancel, LccLidarCache* lidar_cache);

}  // namespace spirula
