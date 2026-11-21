#pragma once

#include "generated/slang.cuh"

inline __device__ float3  lidar_scale_bias(float3  log_scale_0)
{
    int thin_0 = int(0);
    int long_axis_0 = int(0);
    int axis_0 = int(1);
    for(;;)
    {
        if(axis_0 < int(3))
        {
        }
        else
        {
            break;
        }
        int _S1 = axis_0;
        if(_slang_vector_get_element(log_scale_0, axis_0) < _slang_vector_get_element(log_scale_0, thin_0))
        {
            thin_0 = axis_0;
        }
        if(_slang_vector_get_element(log_scale_0, _S1) > _slang_vector_get_element(log_scale_0, long_axis_0))
        {
            long_axis_0 = axis_0;
        }
        axis_0 = axis_0 + int(1);
    }
    float3  bias_0 = make_float3 (0.0f);
    if(thin_0 == long_axis_0)
    {
        return bias_0;
    }
    int middle_0 = int(3) - thin_0 - long_axis_0;
    float gap_0 = _slang_vector_get_element(log_scale_0, long_axis_0) - _slang_vector_get_element(log_scale_0, middle_0);
    float _S2;
    if(_slang_vector_get_element(log_scale_0, thin_0) < (_slang_vector_get_element(log_scale_0, long_axis_0) - 8.0f))
    {
        _S2 = -0.00999999977648258f;
    }
    else
    {
        _S2 = 0.00999999977648258f;
    }
    *_slang_vector_get_element_ptr(&bias_0, thin_0) = _S2;
    *_slang_vector_get_element_ptr(&bias_0, middle_0) = -0.00999999977648258f * gap_0;
    *_slang_vector_get_element_ptr(&bias_0, long_axis_0) = 0.00999999977648258f * gap_0;
    return bias_0;
}

inline __device__ float lidar_opacity_bias()
{
    return 0.00100000004749745f;
}

