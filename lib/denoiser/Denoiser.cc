// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#ifdef MOONRAY_USE_CUDA

#include "OIDNDenoiserImpl.h"
#include "OptixDenoiserImpl.h"
#include "Denoiser.h"

#include <scene_rdl2/render/logging/logging.h>

// This header must be included in exactly one .cc file for the link to succeed
// #include <optix_function_table_definition.h>

namespace moonray {
namespace denoiser {

Denoiser::Denoiser(DenoiserMode mode,
                   int width,
                   int height,
                   bool useAlbedo,
                   bool useNormals,
                   std::string* errorMsg) :
    mMode(mode)
{
    switch (mode) {
    case OPTIX:
        mImpl.reset(new OptixDenoiserImpl(width, height, useAlbedo, useNormals, errorMsg));
        if (!errorMsg->empty()) {
            // Something went wrong so free everything
            // Output the error to Logger::error so we are guaranteed to see it
            scene_rdl2::logging::Logger::error("Denoiser: " + *errorMsg);
            mImpl.reset();
        }
    break;
    case OPEN_IMAGE_DENOISE:
        mImpl.reset(new OIDNDenoiserImpl(OIDN_DEVICE_TYPE_DEFAULT, width, height, useAlbedo,
                                         useNormals, errorMsg));
        if (!errorMsg->empty()) {
            // Something went wrong so free everything
            // Output the error to Logger::error so we are guaranteed to see it
            scene_rdl2::logging::Logger::error("Denoiser: " + *errorMsg);
            mImpl.reset();
        }
    break;
    case OPEN_IMAGE_DENOISE_CPU:
        mImpl.reset(new OIDNDenoiserImpl(OIDN_DEVICE_TYPE_CPU, width, height, useAlbedo,
                                         useNormals, errorMsg));
        if (!errorMsg->empty()) {
            // Something went wrong so free everything
            // Output the error to Logger::error so we are guaranteed to see it
            scene_rdl2::logging::Logger::error("Denoiser: " + *errorMsg);
            mImpl.reset();
        }
    break;
    case OPEN_IMAGE_DENOISE_CUDA:
        mImpl.reset(new OIDNDenoiserImpl(OIDN_DEVICE_TYPE_CUDA, width, height, useAlbedo,
                                         useNormals, errorMsg));
        if (!errorMsg->empty()) {
            // Something went wrong so free everything
            // Output the error to Logger::error so we are guaranteed to see it
            scene_rdl2::logging::Logger::error("Denoiser: " + *errorMsg);
            mImpl.reset();
        }
    break;
    };
}

Denoiser::~Denoiser()
{
}

void 
Denoiser::denoise(const float *inputBeauty,
                  const float *inputAlbedo,
                  const float *inputNormals,
                  float *output,
                  std::string* errorMsg)
{
    mImpl->denoise(inputBeauty, inputAlbedo, inputNormals, output, errorMsg);
}

int 
Denoiser::imageWidth() const
{
    return mImpl->imageWidth();
}

int 
Denoiser::imageHeight() const
{
    return mImpl->imageHeight();
}

bool
Denoiser::useAlbedo() const
{
    return mImpl->useAlbedo();
}

bool
Denoiser::useNormals() const
{
    return mImpl->useNormals();
}

} // namespace denoiser
} // namespace moonray

#else // not MOONRAY_USE_CUDA

#include "OIDNDenoiserImpl.h"
#include "Denoiser.h"

#include <scene_rdl2/render/logging/logging.h>

namespace moonray {
namespace denoiser {

Denoiser::Denoiser(DenoiserMode mode,
                   int width,
                   int height,
                   bool useAlbedo,
                   bool useNormals,
                   std::string* errorMsg) :
    mMode(mode)
{
    switch (mode) {
    case OPTIX:
        *errorMsg = "Optix mode not supported in this build";
        scene_rdl2::logging::Logger::error("Denoiser: " + *errorMsg);
    break;
    case OPEN_IMAGE_DENOISE:
        mImpl.reset(new OIDNDenoiserImpl(OIDN_DEVICE_TYPE_DEFAULT, width, height, useAlbedo,
                                         useNormals, errorMsg));
        if (!errorMsg->empty()) {
            // Something went wrong so free everything
            // Output the error to Logger::error so we are guaranteed to see it
            scene_rdl2::logging::Logger::error("Denoiser: " + *errorMsg);
            mImpl.reset();
        }
    break;
    case OPEN_IMAGE_DENOISE_CPU:
        mImpl.reset(new OIDNDenoiserImpl(OIDN_DEVICE_TYPE_CPU, width, height, useAlbedo,
                                         useNormals, errorMsg));
        if (!errorMsg->empty()) {
            // Something went wrong so free everything
            // Output the error to Logger::error so we are guaranteed to see it
            scene_rdl2::logging::Logger::error("Denoiser: " + *errorMsg);
            mImpl.reset();
        }
    break;
    case OPEN_IMAGE_DENOISE_CUDA:
        *errorMsg = "Open Image Denoise CUDA mode not supported in this build";
        scene_rdl2::logging::Logger::error("Denoiser: " + *errorMsg);
    break;
    };
}

Denoiser::~Denoiser()
{
}

void 
Denoiser::denoise(const float *inputBeauty,
                  const float *inputAlbedo,
                  const float *inputNormals,
                  float *output,
                  std::string* errorMsg)
{
    mImpl->denoise(inputBeauty, inputAlbedo, inputNormals, output, errorMsg);
}

int 
Denoiser::imageWidth() const
{
    return mImpl->imageWidth();
}

int 
Denoiser::imageHeight() const
{
    return mImpl->imageHeight();
}

bool
Denoiser::useAlbedo() const
{
    return mImpl->useAlbedo();
}

bool
Denoiser::useNormals() const
{
    return mImpl->useNormals();
}

} // namespace denoiser
} // namespace moonray

#endif // not MOONRAY_USE_CUDA
