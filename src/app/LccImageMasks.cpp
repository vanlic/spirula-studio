#include "app/LccImageMasks.h"

#include "app/FrameMask.h"
#include "core/CameraModel.h"
#include "core/ImageOrient.h"
#include "data/MaskDecode.h"

#include <algorithm>
#include <exception>
#include <filesystem>
#include <map>
#include <stdexcept>
#include <tuple>

namespace app {
namespace {

bool cancelled(const std::atomic<bool>* cancel) {
    return cancel && cancel->load(std::memory_order_relaxed);
}

int quarter_turns(const ParsedDataset& ds, size_t frame) {
    return ds.exif_quarter_turns.empty() ? 0 : ds.exif_quarter_turns[frame];
}

std::shared_ptr<const spirula::LccRasterMask> pack_mask(const std::vector<uint8_t>& pixels) {
    if (std::all_of(pixels.begin(), pixels.end(), [](uint8_t p) { return p != 0; })) return {};
    auto mask = std::make_shared<spirula::LccRasterMask>();
    for (size_t i = 0; i < pixels.size(); ++i) (*mask)[i] = pixels[i] != 0;
    return mask;
}

}  // namespace

spirula::LccProjectionMasks load_lcc_image_masks(const ParsedDataset& ds, bool flip_mask,
                                                 const spirula::PartitionLog& log,
                                                 const std::atomic<bool>* cancel) {
    namespace fs = std::filesystem;
    using spirula::lcc_raster_dimensions;
    const size_t frames = (size_t)ds.num_cameras;
    spirula::LccProjectionMasks result;
    result.frames.resize(frames);
    std::exception_ptr decode_error;
    size_t file_masks = 0;
    if (!ds.mask_filenames.empty()) {
#pragma omp parallel for schedule(dynamic, 4) num_threads(8) reduction(+:file_masks)
        for (int64_t i = 0; i < ds.num_cameras; ++i) {
            const size_t frame = (size_t)i;
            const std::string& path = ds.mask_filenames[frame];
            if (path.empty() || cancelled(cancel)) continue;
            const auto [cols, rows] = lcc_raster_dimensions(ds.widths[frame], ds.heights[frame]);
            if (cols == 0) continue;
            try {
                std::vector<uint8_t> pixels((size_t)cols * rows);
                spirula::decode_mask_into(path, rows, cols, flip_mask, 0.0f,
                                          pixels.data(), quarter_turns(ds, frame));
                result.frames[frame] = pack_mask(pixels);
                ++file_masks;
            } catch (...) {
#pragma omp critical(ss_lcc_mask_error)
                if (!decode_error) decode_error = std::current_exception();
            }
        }
    }
    if (cancelled(cancel)) throw spirula::PartitionCancelled();
    if (decode_error) std::rethrow_exception(decode_error);
    if (log && file_masks) log("LCC: loaded " + std::to_string(file_masks) + " dataset masks");

    using Lens = std::tuple<fs::path, int, int>;
    std::map<Lens, std::vector<size_t>> lenses;
    for (size_t frame = 0; frame < frames && !ds.camera_models.empty(); ++frame) {
        const auto model = (CameraModelType)ds.camera_models[frame];
        if (model != CameraModelType::FISHEYE && model != CameraModelType::EQUISOLID) continue;
        int w = ds.widths[frame], h = ds.heights[frame];
        spirula::oriented_size(quarter_turns(ds, frame), w, h);
        if (w <= 0 || h <= 0) continue;
        lenses[{fs::u8path(ds.image_filenames[frame]).parent_path(), w, h}].push_back(frame);
    }
    const auto has_file = [&](size_t frame) {
        return !ds.mask_filenames.empty() && !ds.mask_filenames[frame].empty();
    };
    for (const auto& [lens, indices] : lenses) {
        if (cancelled(cancel)) throw spirula::PartitionCancelled();
        if (std::all_of(indices.begin(), indices.end(), has_file)) continue;
        std::vector<std::string> photos;
        photos.reserve(indices.size());
        for (size_t frame : indices) photos.push_back(ds.image_filenames[frame]);
        const BorderDetect border = detect_fisheye_border(photos, {}, false, cancel);
        if (cancelled(cancel)) throw spirula::PartitionCancelled();
        const std::string folder = std::get<0>(lens).u8string();
        if (log) log(std::string(border.found ? "LCC: fitted fisheye border in " : "LCC: no fisheye border found in ") + folder);
        if (!border.found) continue;
        const FrameMask stencil{{border.shape}, {}};
        std::map<std::tuple<int, int, int>, std::shared_ptr<const spirula::LccRasterMask>> raster_masks;
        for (size_t frame : indices) {
            if (has_file(frame)) continue;
            const auto [cols, rows] = lcc_raster_dimensions(ds.widths[frame], ds.heights[frame]);
            const int turns = quarter_turns(ds, frame);
            auto [it, fresh] = raster_masks.try_emplace({cols, rows, turns});
            if (fresh) {
                int w = cols, h = rows;
                spirula::oriented_size(turns, w, h);
                std::vector<uint8_t> pixels;
                std::string error;
                if (!rasterize_frame_mask(stencil, w, h, pixels, error))
                    throw std::runtime_error("LCC: cannot rasterize the fisheye border: " + error);
                if (turns != 0) {
                    std::vector<uint8_t> turned(pixels.size());
                    spirula::orient_pixels(pixels.data(), w, h, 1, turns, false, turned.data());
                    pixels.swap(turned);
                }
                it->second = pack_mask(pixels);
            }
            result.frames[frame] = it->second;
        }
    }
    if (std::none_of(result.frames.begin(), result.frames.end(), [](const auto& mask) { return bool(mask); }))
        result.frames.clear();
    return result;
}

}  // namespace app
