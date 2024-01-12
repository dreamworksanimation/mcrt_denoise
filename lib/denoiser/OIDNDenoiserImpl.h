// Copyright 2023-2024 DreamWorks Animation LLC
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
    OIDNDenoiserImpl(OIDNDeviceType deviceType,
                     int width,
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
    OIDNDeviceType mDeviceType;
    OIDNDevice mDevice;
    OIDNFilter mFilter;
    OIDNBuffer mInputBeauty3;
    OIDNBuffer mInputAlbedo3;
    OIDNBuffer mInputNormals3;
    OIDNBuffer mOutput3; 
};

} // namespace denoiser
} // namespace moonray

