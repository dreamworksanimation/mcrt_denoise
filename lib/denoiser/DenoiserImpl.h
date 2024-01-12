// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include <string>

namespace moonray {
namespace denoiser {

class DenoiserImpl
{
public:
    DenoiserImpl(int width,
                 int height,
                 bool useAlbedo,
                 bool useNormals) :
    mWidth(width),
    mHeight(height),
    mUseAlbedo(useAlbedo),
    mUseNormals(useNormals) 
    {}

    virtual ~DenoiserImpl() {}

    virtual void denoise(const float *inputBeauty,  // RGBA
                         const float *inputAlbedo,  // RGBA
                         const float *inputNormals, // RGBA
                         float *output,       // RGBA
                         std::string* errorMsg) = 0;

    int imageWidth() const { return mWidth; }
    int imageHeight() const { return mHeight; }
    bool useAlbedo() const { return mUseAlbedo; }
    bool useNormals() const { return mUseNormals; }

protected:
    int mWidth;
    int mHeight;
    bool mUseAlbedo;
    bool mUseNormals;
};

} // namespace denoiser
} // namespace moonray

