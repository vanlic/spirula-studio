#pragma once

#include "data/CameraMath.h"
#include "data/DatasetParser.h"

#include <algorithm>
#include <cmath>

namespace spirula {

struct FrameView {
    double R[9];   // rows: the CV camera axes in world
    double t[3];
    camhost::Camera cam;
    double cos_max = -2.0;   // cone pre-test for pinhole; -2 disables
};

inline FrameView frame_view(const ParsedDataset& ds, int64_t i) {
    FrameView f;
    const float* M = &ds.c2w[(size_t)i * 12];
    // OpenGL columns X, Y, Z (camera looks down -Z); CV is (x, -y, -z).
    for (int k = 0; k < 3; k++) {
        f.R[0 + k] = M[k * 4 + 0];
        f.R[3 + k] = -M[k * 4 + 1];
        f.R[6 + k] = -M[k * 4 + 2];
        f.t[k] = M[k * 4 + 3];
    }
    f.cam.model = ds.camera_models.empty() ? 0 : ds.camera_models[(size_t)i];
    f.cam.tier = ds.camera_distortions.empty() ? 0 : ds.camera_distortions[(size_t)i];
    f.cam.width = ds.widths[(size_t)i];
    f.cam.height = ds.heights[(size_t)i];
    f.cam.fx = ds.intrins[(size_t)i * 4 + 0];
    f.cam.fy = ds.intrins[(size_t)i * 4 + 1];
    f.cam.cx = ds.intrins[(size_t)i * 4 + 2];
    f.cam.cy = ds.intrins[(size_t)i * 4 + 3];
    if (!ds.dist_coeffs.empty())
        for (int k = 0; k < 8; k++) f.cam.dist[k] = ds.dist_coeffs[(size_t)i * 8 + k];
    if (f.cam.model == 0 && f.cam.fx > 0 && f.cam.fy > 0) {
        // The widest normalized coordinate any corner reaches, with slack for
        // lens distortion pulling the border in or out.
        double r2 = 0;
        for (int c = 0; c < 4; c++) {
            const double u = ((c & 1 ? f.cam.width : 0) - f.cam.cx) / f.cam.fx;
            const double v = ((c & 2 ? f.cam.height : 0) - f.cam.cy) / f.cam.fy;
            r2 = std::max(r2, u * u + v * v);
        }
        const double r = std::sqrt(r2) * 1.15;
        f.cos_max = 1.0 / std::sqrt(1.0 + r * r);
    }
    return f;
}

inline bool frame_sees(const FrameView& f, const double p[3], double* pixel = nullptr, double* depth = nullptr) {
    const double d[3] = {p[0] - f.t[0], p[1] - f.t[1], p[2] - f.t[2]};
    double ray[3];
    for (int r = 0; r < 3; r++)
        ray[r] = f.R[r * 3] * d[0] + f.R[r * 3 + 1] * d[1] + f.R[r * 3 + 2] * d[2];
    const double len = std::sqrt(ray[0] * ray[0] + ray[1] * ray[1] + ray[2] * ray[2]);
    if (!(len > 1e-12)) return false;
    if (f.cos_max > -1.5 && ray[2] / len < f.cos_max) return false;
    double px[2];
    if (depth) *depth = len;
    return camhost::ray_in_frame(f.cam, ray, pixel ? pixel : px);
}

}  // namespace spirula
