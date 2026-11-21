#include "kernels/optim/Optimizer.cuh"

#include <array>
#include <cmath>
#include <cstdio>

namespace {

constexpr int count = 5;

template<class T>
struct Buffer {
    explicit Buffer(const std::array<T, count>& input = {}) {
        backend::memcpy_sync(ptr, input.data(), bytes, backend::MemcpyKind::HostToDevice);
    }
    ~Buffer() { backend::device_free(ptr); }
    Buffer(const Buffer&) = delete;
    Buffer& operator=(const Buffer&) = delete;
    DeviceVector<T> view() const { return DeviceVector<T>({(uint64_t)ptr, sizeof(T), {count, 1}}); }
    std::array<T, count> read() const {
        std::array<T, count> output{};
        backend::memcpy_sync(output.data(), ptr, bytes, backend::MemcpyKind::DeviceToHost);
        return output;
    }
    static constexpr size_t bytes = count * sizeof(T);
    T* ptr = static_cast<T*>(backend::device_malloc(bytes));
};

bool check(bool enable) {
    const std::array<float3, count> initial{{
        {-4, -2, -1}, {-10, -2, -0.5f}, {-2, -2, -2}, {-2, -1, -4}, {-4, -2, -1}}};
    const std::array<float3, count> expected{{
        {-4.00005f, -1.99995f, -1.00005f}, {-9.99995f, -1.999925f, -0.500075f},
        {-2, -2, -2}, {-1.99995f, -1.00005f, -4.00005f}, {-4, -2, -1}}};
    Buffer<float3> means, mean_grad, mean_m1, mean_m2;
    std::array<float4, count> identity;
    identity.fill({1, 0, 0, 0});
    Buffer<float4> quats(identity);
    Buffer<float4> quat_grad, quat_m1, quat_m2;
    Buffer<float3> scales(initial), scale_grad, scale_m1, scale_m2, dc, dc_grad;
    Buffer<float> opacs, opac_grad, opac_m1, opac_m2;
    Buffer<float> radii(std::array<float, count>{1, 1, 1, 1, 0});
    fused_optim_3dgs_geometry(count,
        means.view(), mean_grad.view(), mean_m1.view(), mean_m2.view(),
        quats.view(), quat_grad.view(), quat_m1.view(), quat_m2.view(),
        scales.view(), scale_grad.view(), scale_m1.view(), scale_m2.view(),
        opacs.view(), opac_grad.view(), opac_m1.view(), opac_m2.view(),
        dc.view(), dc_grad.view(), radii.view(), {},
        0.0f, 0.0f, 0.005f, 0.05f, 0.0f, 10.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 0.0f, false, 0.0f, 0.0f, 0.0f, 0.0f, false,
        {}, {}, {}, {}, {}, 7, {}, 1.0f, false, {}, enable);

    const auto out = scales.read(), first = scale_m1.read(), second = scale_m2.read();
    const auto logits = opacs.read(), opacity_first = opac_m1.read(), opacity_second = opac_m2.read();
    const auto close = [](float a, float b) { return std::abs(a - b) <= 1e-6f; };
    const auto zero = [](float3 v) { return v.x == 0 && v.y == 0 && v.z == 0; };
    for (int i = 0; i < count; ++i) {
        const float3 ref = enable ? expected[i] : initial[i];
        const float logit = enable && i != count - 1 ? -0.00005f : 0.0f;
        if (!close(out[i].x, ref.x) || !close(out[i].y, ref.y) || !close(out[i].z, ref.z) ||
            !close(logits[i], logit) || !zero(first[i]) || !zero(second[i]) ||
            opacity_first[i] != 0 || opacity_second[i] != 0) {
            std::printf("LiDAR shape bias %s failed at splat %d\n", enable ? "on" : "off", i);
            return false;
        }
    }
    return true;
}

}  // namespace

int main() {
    if (!check(false) || !check(true)) return 1;
    std::puts("LiDAR shape bias: PASS");
    return 0;
}
