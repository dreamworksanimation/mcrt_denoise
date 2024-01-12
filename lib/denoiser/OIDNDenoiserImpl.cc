// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "OIDNDenoiserImpl.h"

#include <scene_rdl2/common/rec_time/RecTime.h>
#include <scene_rdl2/render/logging/logging.h>

#include <iostream>
#include <string.h>

namespace moonray {
namespace denoiser {

OIDNDenoiserImpl::OIDNDenoiserImpl(OIDNDeviceType deviceType,
                                   int width,
                                   int height,
                                   bool useAlbedo,
                                   bool useNormals,
                                   std::string* errorMsg) :
    DenoiserImpl(width, height, useAlbedo, useNormals)
{
    mDeviceType = deviceType;

    switch (deviceType) {
    case OIDN_DEVICE_TYPE_DEFAULT:
        scene_rdl2::logging::Logger::info("Creating Open Image Denoise denoiser (default/best device)");
        break;
     case OIDN_DEVICE_TYPE_CPU:
        scene_rdl2::logging::Logger::info("Creating Open Image Denoise denoiser (CPU device)");
        break;
    case OIDN_DEVICE_TYPE_CUDA:
        scene_rdl2::logging::Logger::info("Creating Open Image Denoise denoiser (CUDA device)");
        break;
    default:
        scene_rdl2::logging::Logger::info("Creating Open Image Denoise denoiser (unknown device)");
    }

    const char* oidnErrorMessage;

    mFilter = nullptr;
    mDevice = oidnNewDevice(deviceType);
    if (!mDevice) {
        if (oidnGetDeviceError(mDevice, &oidnErrorMessage) != OIDN_ERROR_NONE) {
            *errorMsg = oidnErrorMessage;
        } else {
            *errorMsg = "Unable to create OIDN Device";
        }
        return;
    }
    oidnCommitDevice(mDevice);

    mFilter = oidnNewFilter(mDevice, "RT");
    if (!mFilter) {
        *errorMsg = "Unable to create OIDN Filter";
        oidnReleaseDevice(mDevice);
        return;
    } 
    oidnSetFilter1b(mFilter, "hdr", true);

    size_t bufferSize = mWidth * mHeight * 3 * sizeof(float);

    mInputBeauty3 = oidnNewBuffer(mDevice, bufferSize);
    oidnSetFilterImage(mFilter, "color", mInputBeauty3, OIDN_FORMAT_FLOAT3, mWidth, mHeight, 0, 0, 0);

    mOutput3 = oidnNewBuffer(mDevice, bufferSize);
    oidnSetFilterImage(mFilter, "output", mOutput3, OIDN_FORMAT_FLOAT3, mWidth, mHeight, 0, 0, 0);
    
    if (mUseAlbedo) {
        mInputAlbedo3 = oidnNewBuffer(mDevice, bufferSize);
        oidnSetFilterImage(mFilter, "albedo", mInputAlbedo3, OIDN_FORMAT_FLOAT3, mWidth, mHeight, 0, 0, 0);
    }

    if (mUseNormals) {
        mInputNormals3 = oidnNewBuffer(mDevice, bufferSize);
        oidnSetFilterImage(mFilter, "normal", mInputNormals3, OIDN_FORMAT_FLOAT3, mWidth, mHeight, 0, 0, 0);
    }
    
    oidnCommitFilter(mFilter);

    if (oidnGetDeviceError(mDevice, &oidnErrorMessage) != OIDN_ERROR_NONE) {
        *errorMsg = oidnErrorMessage;
        return;
    }
}

OIDNDenoiserImpl::~OIDNDenoiserImpl()
{
    switch (mDeviceType) {
    case OIDN_DEVICE_TYPE_DEFAULT:
        scene_rdl2::logging::Logger::info("Freeing Open Image Denoise denoiser (default/best device)");
        break;
     case OIDN_DEVICE_TYPE_CPU:
        scene_rdl2::logging::Logger::info("Freeing Open Image Denoise denoiser (CPU device)");
        break;
    case OIDN_DEVICE_TYPE_CUDA:
        scene_rdl2::logging::Logger::info("Freeing Open Image Denoise denoiser (CUDA device)");
        break;
    default:
        scene_rdl2::logging::Logger::info("Freeing Open Image Denoise denoiser (unknown device)");
    }

    if (mInputBeauty3) oidnReleaseBuffer(mInputBeauty3);
    if (mUseAlbedo && mInputAlbedo3) oidnReleaseBuffer(mInputAlbedo3);
    if (mUseNormals && mInputNormals3) oidnReleaseBuffer(mInputNormals3);
    if (mOutput3) oidnReleaseBuffer(mOutput3);

    if (mFilter) oidnReleaseFilter(mFilter);
    if (mDevice) oidnReleaseDevice(mDevice);
}

void 
OIDNDenoiserImpl::denoise(const float *inputBeauty,
                          const float *inputAlbedo,
                          const float *inputNormals,
                          float *output,
                          std::string* errorMsg)
{
    float* mInputBeauty3Ptr = (float*)oidnGetBufferData(mInputBeauty3);
    for (int i = 0; i < mWidth * mHeight; i++) {
        mInputBeauty3Ptr[i * 3] = inputBeauty[i * 4];
        mInputBeauty3Ptr[i * 3 + 1] = inputBeauty[i * 4 + 1];
        mInputBeauty3Ptr[i * 3 + 2] = inputBeauty[i * 4 + 2];
    }


    if (mUseAlbedo) {
        float* mInputAlbedo3Ptr = (float*)oidnGetBufferData(mInputAlbedo3);
        for (int i = 0; i < mWidth * mHeight; i++) {
            mInputAlbedo3Ptr[i * 3] = inputAlbedo[i * 4];
            mInputAlbedo3Ptr[i * 3 + 1] = inputAlbedo[i * 4 + 1];
            mInputAlbedo3Ptr[i * 3 + 2] = inputAlbedo[i * 4 + 2];
        }
    }

    if (mUseNormals) {
        float* mInputNormals3Ptr = (float*)oidnGetBufferData(mInputNormals3);
        for (int i = 0; i < mWidth * mHeight; i++) {
            mInputNormals3Ptr[i * 3] = inputNormals[i * 4];
            mInputNormals3Ptr[i * 3 + 1] = inputNormals[i * 4 + 1];
            mInputNormals3Ptr[i * 3 + 2] = inputNormals[i * 4 + 2];
        }
    }

    // scene_rdl2::rec_time::RecTime denoiseTimer;
    // denoiseTimer.start();

    oidnExecuteFilter(mFilter);

    const char* oidnErrorMessage;
    if (oidnGetDeviceError(mDevice, &oidnErrorMessage) != OIDN_ERROR_NONE) {
        *errorMsg = oidnErrorMessage;
        return;
    }

    // std::cerr << "OIDN denoise() elapsed time (s): " << denoiseTimer.end() << std::endl;

    float* mOutput3Ptr = (float*)oidnGetBufferData(mOutput3);
    for (int i = 0; i < mWidth * mHeight; i++) {
        output[i * 4] = mOutput3Ptr[i * 3];
        output[i * 4 + 1] = mOutput3Ptr[i * 3 + 1];
        output[i * 4 + 2] = mOutput3Ptr[i * 3 + 2];
        output[i * 4 + 3] = inputBeauty[i * 4 + 3];
    }
}

} // namespace denoiser
} // namespace moonray

