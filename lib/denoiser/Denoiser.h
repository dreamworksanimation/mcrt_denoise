// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <memory>
#include <string>

namespace moonray {
namespace denoiser {

// We hide the details of this class behind an impl because we don't want to expose
// all of the CUDA/Optix headers to the rest of Moonray.

class DenoiserImpl;

enum DenoiserMode
{
    OPTIX,
    OPEN_IMAGE_DENOISE,
    OPEN_IMAGE_DENOISE_CPU,
    OPEN_IMAGE_DENOISE_CUDA
};

class Denoiser
{
public:
    Denoiser(DenoiserMode mode,
             int width,
             int height,
             bool useAlbedo,
             bool useNormals,
             std::string* errorMsg);
    ~Denoiser();

    // Copy is disabled
    Denoiser(const Denoiser& other) = delete;
    Denoiser &operator=(const Denoiser& other) = delete;

    void denoise(const float *inputBeauty,  // RGBA
                 const float *inputAlbedo,  // RGBA
                 const float *inputNormals, // RGBA
                 float *output,       // RGBA
                 std::string* errorMsg);

    DenoiserMode mode() const { return mMode; }
    int imageWidth() const;
    int imageHeight() const;
    bool useAlbedo() const;
    bool useNormals() const;

private:
    DenoiserMode mMode;
    std::unique_ptr<DenoiserImpl> mImpl;
};

} // namespace denoiser
} // namespace moonray

