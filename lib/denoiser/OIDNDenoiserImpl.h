// Copyright 2023 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#pragma once

#include "DenoiserImpl.h"

#include <OpenImageDenoise/oidn.h>
#include <string>
#include <vector>

namespace moonray {
namespace denoiser {

class OIDNDenoiserImpl : public DenoiserImpl
{
public:
    OIDNDenoiserImpl(int width,
                     int height,
                     bool useAlbedo,
                     bool useNormals,
                     std::string* errorMsg);
    ~OIDNDenoiserImpl();

    void denoise(const float *inputBeauty,  // RGBA
                 const float *inputAlbedo,  // RGBA
                 const float *inputNormals, // RGBA
                 float *output,       // RGBA
                 std::string* errorMsg) override;

private:
    OIDNDevice mDevice;
    OIDNFilter mFilter;

    std::vector<float> mInputBeauty3;
    std::vector<float> mInputAlbedo3;
    std::vector<float> mInputNormals3;
    std::vector<float> mOutput3;
};

} // namespace denoiser
} // namespace moonray

