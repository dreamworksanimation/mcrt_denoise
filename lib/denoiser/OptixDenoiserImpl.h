// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "DenoiserImpl.h"

#include <cuda.h>
#include <cuda_runtime.h>
#include <optix.h>
#include <optix_stubs.h>

#include <string>

namespace moonray {
namespace denoiser {

class OptixDenoiserImpl : public DenoiserImpl
{
public:
    OptixDenoiserImpl(int width,
                      int height,
                      bool useAlbedo,
                      bool useNormals,
                      std::string* errorMsg);
    ~OptixDenoiserImpl();

    void denoise(const float *inputBeauty,  // RGBA
                 const float *inputAlbedo,  // RGBA
                 const float *inputNormals, // RGBA
                 float *output,       // RGBA
                 std::string* errorMsg) override;

private:
    bool createOptixContext(OptixLogCallback logCallback,
                            CUstream* cudaStream,
                            OptixDeviceContext* ctx,
                            std::string* deviceName,
                            std::string* errorMsg);

    CUstream mCudaStream;
    std::string mGPUDeviceName;
    OptixDeviceContext mContext;
    OptixDenoiser mDenoiser;
    OptixDenoiserSizes mDenoiserSizes;
    unsigned char* mDenoiserState;
    unsigned char* mScratch;
    OptixDenoiserParams mDenoiserParams;
    float* mDenoisedOutput;
    float* mInputBeauty;
    OptixDenoiserLayer mLayer;
    OptixDenoiserGuideLayer mGuideLayer;
    float* mInputAlbedo;
    float* mInputNormals;
};

} // namespace denoiser
} // namespace moonray

