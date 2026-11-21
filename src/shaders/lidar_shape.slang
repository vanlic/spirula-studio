[CudaDeviceExport]
float3 lidar_scale_bias(float3 log_scale) {
    int thin = 0, long_axis = 0;
    for (int axis = 1; axis < 3; ++axis) {
        if (log_scale[axis] < log_scale[thin]) thin = axis;
        if (log_scale[axis] > log_scale[long_axis]) long_axis = axis;
    }
    float3 bias = float3(0.0f);
    if (thin == long_axis) return bias;
    const int middle = 3 - thin - long_axis;
    const float gap = log_scale[long_axis] - log_scale[middle];
    bias[thin] = log_scale[thin] < log_scale[long_axis] - 8.0f ? -0.01f : 0.01f;
    bias[middle] = -0.01f * gap;
    bias[long_axis] = 0.01f * gap;
    return bias;
}

[CudaDeviceExport]
float lidar_opacity_bias() {
    return 0.001f;
}
