// Copyright 2023 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "OIDNDenoiserImpl.h"

#include <scene_rdl2/common/rec_time/RecTime.h>
#include <scene_rdl2/render/logging/logging.h>

#include <iostream>
#include <string.h>

namespace moonray {
namespace denoiser {

OIDNDenoiserImpl::OIDNDenoiserImpl(int width,
                                   int height,
                                   bool useAlbedo,
                                   bool useNormals,
                                   std::string* errorMsg) :
    DenoiserImpl(width, height, useAlbedo, useNormals)
{
    scene_rdl2::logging::Logger::info("Creating Open Image Denoise denoiser");

    mDevice = oidnNewDevice(OIDN_DEVICE_TYPE_DEFAULT);
    if (!mDevice) {
        *errorMsg = "Unable to create OIDN Device";
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

    mInputBeauty3.resize(mWidth * mHeight * 3);
    mOutput3.resize(mWidth * mHeight * 3);

    oidnSetSharedFilterImage(mFilter, "color", (void*)mInputBeauty3.data(), OIDN_FORMAT_FLOAT3, mWidth, mHeight, 0, 0, 0);
    if (mUseAlbedo) {
        mInputAlbedo3.resize(mWidth * mHeight * 3);
        oidnSetSharedFilterImage(mFilter, "albedo", (void*)mInputAlbedo3.data(), OIDN_FORMAT_FLOAT3, mWidth, mHeight, 0, 0, 0);
    }
    if (mUseNormals) {
        mInputNormals3.resize(mWidth * mHeight * 3);
        oidnSetSharedFilterImage(mFilter, "normal", (void*)mInputNormals3.data(), OIDN_FORMAT_FLOAT3, mWidth, mHeight, 0, 0, 0);
    }
    oidnSetSharedFilterImage(mFilter, "output", (void*)mOutput3.data(), OIDN_FORMAT_FLOAT3, mWidth, mHeight, 0, 0, 0);
    oidnCommitFilter(mFilter);

    const char* oidnErrorMessage;
    if (oidnGetDeviceError(mDevice, &oidnErrorMessage) != OIDN_ERROR_NONE) {
        *errorMsg = oidnErrorMessage;
        return;
    }
}

OIDNDenoiserImpl::~OIDNDenoiserImpl()
{
    scene_rdl2::logging::Logger::info("Freeing Open Image Denoise denoiser");

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
    for (int i = 0; i < mWidth * mHeight; i++) {
        mInputBeauty3[i * 3] = inputBeauty[i * 4];
        mInputBeauty3[i * 3 + 1] = inputBeauty[i * 4 + 1];
        mInputBeauty3[i * 3 + 2] = inputBeauty[i * 4 + 2];
    }

    if (mUseAlbedo) {
        for (int i = 0; i < mWidth * mHeight; i++) {
            mInputAlbedo3[i * 3] = inputAlbedo[i * 4];
            mInputAlbedo3[i * 3 + 1] = inputAlbedo[i * 4 + 1];
            mInputAlbedo3[i * 3 + 2] = inputAlbedo[i * 4 + 2];
        }
    }

    if (mUseNormals) {
        for (int i = 0; i < mWidth * mHeight; i++) {
            mInputNormals3[i * 3] = inputNormals[i * 4];
            mInputNormals3[i * 3 + 1] = inputNormals[i * 4 + 1];
            mInputNormals3[i * 3 + 2] = inputNormals[i * 4 + 2];
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

    for (int i = 0; i < mWidth * mHeight; i++) {
        output[i * 4] = mOutput3[i * 3];
        output[i * 4 + 1] = mOutput3[i * 3 + 1];
        output[i * 4 + 2] = mOutput3[i * 3 + 2];
        output[i * 4 + 3] = 0.f;
    }
}

} // namespace denoiser
} // namespace moonray

