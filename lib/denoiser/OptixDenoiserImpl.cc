// Copyright 2023-2024 DreamWorks Animation LLC
// SPDX-License-Identifier: Apache-2.0

#include "OptixDenoiserImpl.h"

#include <scene_rdl2/common/rec_time/RecTime.h>
#include <scene_rdl2/render/logging/logging.h>

#include <iostream>
#include <string.h>

namespace moonray {
namespace denoiser {

// Optix will call this callback for information / error messages.
static void denoiserMessageCallback(unsigned int /*level*/,
                                    const char * /*tag*/,
                                    const char *message,
                                    void *)
{
    scene_rdl2::logging::Logger::info("Denoiser: ", message);
}

bool
getNVIDIADriverVersion(int* major, int* minor)
{
    *major = 0;
    *minor = 0;
    bool success = false;
    FILE *fp = fopen("/sys/module/nvidia/version", "r");
    if (fp != NULL) {
        if (fscanf(fp, "%d.%d", major, minor) == 2) {
            success = true;
        }
        fclose(fp);
    }
    return success;
}


OptixDenoiserImpl::OptixDenoiserImpl(int width,
                                     int height,
                                     bool useAlbedo,
                                     bool useNormals,
                                     std::string* errorMsg) :
    DenoiserImpl(width, height, useAlbedo, useNormals),
    mCudaStream {0},
    mContext {nullptr},
    mDenoiser {nullptr},
    mDenoiserState {nullptr},
    mScratch {nullptr},
    mDenoisedOutput {nullptr},
    mInputBeauty {nullptr},
    mInputAlbedo {nullptr},
    mInputNormals {nullptr}
{
    scene_rdl2::logging::Logger::info("Creating Optix denoiser");

    if (!createOptixContext(denoiserMessageCallback,
                            &mCudaStream,
                            &mContext,
                            &mGPUDeviceName,
                            errorMsg)) {
        return;
    }

    OptixDenoiserOptions optionsDenoiser = {};
    if (useAlbedo) optionsDenoiser.guideAlbedo = 1;
    if (useNormals) optionsDenoiser.guideNormal = 1;

    if (optixDenoiserCreate(mContext,
                            OPTIX_DENOISER_MODEL_KIND_HDR,
                            &optionsDenoiser,
                            &mDenoiser) != OPTIX_SUCCESS) {
        *errorMsg = "Unable to create the Optix denoiser";
        return;
    }

    mDenoiserSizes = {}; // zero initialize
    if (optixDenoiserComputeMemoryResources(mDenoiser,
                                            mWidth,
                                            mHeight,
                                            &mDenoiserSizes) != OPTIX_SUCCESS) {
        *errorMsg = "Unable to compute denoiser memory resources";
        return;
    }

    if (cudaMalloc(&mDenoiserState, mDenoiserSizes.stateSizeInBytes) != cudaSuccess) {
         *errorMsg = "Unable to allocate denoiser state";
        return;
    }

    if (cudaMalloc(&mScratch, mDenoiserSizes.withoutOverlapScratchSizeInBytes) != cudaSuccess) {
         *errorMsg = "Unable to allocate denoiser scratch buffer";
        return;       
    }

    if (optixDenoiserSetup(mDenoiser,
                           mCudaStream, 
                           mWidth, mHeight, 
                           reinterpret_cast<CUdeviceptr>(mDenoiserState),
                           mDenoiserSizes.stateSizeInBytes,
                           reinterpret_cast<CUdeviceptr>(mScratch),
                           mDenoiserSizes.withoutOverlapScratchSizeInBytes) != OPTIX_SUCCESS) {
        *errorMsg = "Unable to setup denoiser";
        return;       
    }

    mDenoiserParams = {};                 // zero initialize
    mDenoiserParams.denoiseAlpha = OPTIX_DENOISER_ALPHA_MODE_COPY; // don't denoise alpha
    mDenoiserParams.hdrIntensity = 0;     // optional average log intensity image of input image, 
                                          //  helps with very dark/bright images
    mDenoiserParams.blendFactor = 0.f;    // show the denoised image only
    mDenoiserParams.hdrAverageColor = 0;  // used with OPTIX_DENOISER_MODEL_KIND_AOV

    if (cudaMalloc(&mDenoisedOutput, sizeof(float4) * mWidth * mHeight) != cudaSuccess) {
         *errorMsg = "Unable to allocate denoiser output buffer";
        return;
    }

    if (cudaMalloc(&mInputBeauty, sizeof(float4) * mWidth * mHeight) != cudaSuccess) {
         *errorMsg = "Unable to allocate denoiser input beauty buffer";
        return;
    }

    // The layer specifies the input/output buffers and their formats
    mLayer = {};
    mLayer.input.data                = reinterpret_cast<CUdeviceptr>(mInputBeauty);
    mLayer.input.width               = mWidth;
    mLayer.input.height              = mHeight;
    mLayer.input.rowStrideInBytes    = mWidth * sizeof(float4);
    mLayer.input.pixelStrideInBytes  = sizeof(float4);
    mLayer.input.format              = OPTIX_PIXEL_FORMAT_FLOAT4;
    mLayer.output.data               = reinterpret_cast<CUdeviceptr>(mDenoisedOutput);
    mLayer.output.width              = mWidth;
    mLayer.output.height             = mHeight;
    mLayer.output.rowStrideInBytes   = mWidth * sizeof(float4);
    mLayer.output.pixelStrideInBytes = sizeof(float4);
    mLayer.output.format             = OPTIX_PIXEL_FORMAT_FLOAT4;

    // The guide layer specifies the albedo/normal buffers and their formats
    mGuideLayer = {};

    if (useAlbedo) {
        if (cudaMalloc(&mInputAlbedo, sizeof(float4) * mWidth * mHeight) != cudaSuccess) {
            *errorMsg = "Unable to allocate denoiser input albedo buffer";
            return;
        }
        mGuideLayer.albedo.data               = reinterpret_cast<CUdeviceptr>(mInputAlbedo);
        mGuideLayer.albedo.width              = mWidth;
        mGuideLayer.albedo.height             = mHeight;
        mGuideLayer.albedo.rowStrideInBytes   = mWidth * sizeof(float4);
        mGuideLayer.albedo.pixelStrideInBytes = sizeof(float4);
        mGuideLayer.albedo.format             = OPTIX_PIXEL_FORMAT_FLOAT4;
    }
    if (useNormals) {
        if (cudaMalloc(&mInputNormals, sizeof(float4) * mWidth * mHeight) != cudaSuccess) {
            *errorMsg = "Unable to allocate denoiser input normals buffer";
            return;
        }
        mGuideLayer.normal.data               = reinterpret_cast<CUdeviceptr>(mInputNormals);
        mGuideLayer.normal.width              = mWidth;
        mGuideLayer.normal.height             = mHeight;
        mGuideLayer.normal.rowStrideInBytes   = mWidth * sizeof(float4);
        mGuideLayer.normal.pixelStrideInBytes = sizeof(float4);
        mGuideLayer.normal.format             = OPTIX_PIXEL_FORMAT_FLOAT4;
    }
}

OptixDenoiserImpl::~OptixDenoiserImpl()
{
    scene_rdl2::logging::Logger::info("Freeing Optix denoiser");

    if (mInputNormals != 0) {
        cudaFree(mInputNormals);
    }

    if (mInputAlbedo != 0) {
        cudaFree(mInputAlbedo);
    }

    if (mInputBeauty != 0) {
        cudaFree(mInputBeauty);
    }

    if (mDenoisedOutput != 0) {
        cudaFree(mDenoisedOutput);
    }

    if (mScratch != 0) {
        cudaFree(mScratch);
    }

    if (mDenoiserState != 0) {
        cudaFree(mDenoiserState);
    }

    if (mDenoiser != nullptr) {
        optixDenoiserDestroy(mDenoiser);
    }

    if (mContext != nullptr) {
        optixDeviceContextDestroy(mContext);
    }

    if (mCudaStream != 0) {
        cudaStreamDestroy(mCudaStream);
    }
}

void 
OptixDenoiserImpl::denoise(const float *inputBeauty,
                           const float *inputAlbedo,
                           const float *inputNormals,
                           float *output,
                           std::string* errorMsg)
{
    // scene_rdl2::rec_time::RecTime denoiseTimer;
    // denoiseTimer.start();

    // Copy the noisy input beauty to the GPU
    if (cudaMemcpy((void*)mInputBeauty,
                   inputBeauty,
                   mWidth * mHeight * sizeof(float4),
                   cudaMemcpyHostToDevice) != cudaSuccess) {
        *errorMsg = "Denoiser failure copying input beauty";
        return;
    }
 
    if (mInputAlbedo) {
        // Copy the input albedo to the GPU
        if (cudaMemcpy((void*)mInputAlbedo,
                       inputAlbedo,
                       mWidth * mHeight * sizeof(float4),
                       cudaMemcpyHostToDevice) != cudaSuccess) {
            *errorMsg = "Denoiser failure copying input albedo";
            return;
        }
    }

    if (mInputNormals) {
        // Copy the noisy input normals to the GPU
        if (cudaMemcpy((void*)mInputNormals,
                       inputNormals,
                       mWidth * mHeight * sizeof(float4),
                       cudaMemcpyHostToDevice) != cudaSuccess) {
            *errorMsg = "Denoiser failure copying input normals";
            return;            
        }
    }

    if (optixDenoiserInvoke(mDenoiser, mCudaStream, &mDenoiserParams,
                            reinterpret_cast<CUdeviceptr>(mDenoiserState),
                            mDenoiserSizes.stateSizeInBytes,
                            &mGuideLayer,
                            &mLayer,
                            1,  // numLayers 
                            0,  // inputOffsetX
                            0,  // inputOffsetY
                            reinterpret_cast<CUdeviceptr>(mScratch),
                            mDenoiserSizes.withoutOverlapScratchSizeInBytes) != OPTIX_SUCCESS) {
        *errorMsg = "Denoiser failure in optixDenoiserInvoke()";
        return;
    }

    // Copy the denoised output from the GPU to *output
    if (cudaMemcpy(output,
                   (void*)mDenoisedOutput,
                   mWidth * mHeight * sizeof(float4),
                   cudaMemcpyDeviceToHost) != cudaSuccess) { 
        *errorMsg = "Denoiser failure copying output";
        return;         
    }

    // std::cerr << "Optix denoise() elapsed time (s): " << denoiseTimer.end() << std::endl;
}

bool
OptixDenoiserImpl::createOptixContext(OptixLogCallback logCallback,
                                      CUstream *cudaStream,
                                      OptixDeviceContext *ctx,
                                      std::string *deviceName,
                                      std::string *errorMsg)
{
    *errorMsg = "";

    int major, minor;
    if (!getNVIDIADriverVersion(&major, &minor)) {
        *errorMsg = "Unable to query NVIDIA driver version";
        return false;
    }
    if (major < 525) {
        *errorMsg = "NVIDIA driver too old, must be >= 525";
        return false;
    }

    cudaFree(0);    // init CUDA

    int numDevices = 0;
    cudaGetDeviceCount(&numDevices);
    if (numDevices == 0) {
        *errorMsg = "No CUDA capable devices found";
        return false;
    }

    const int deviceID = 0;
    if (cudaSetDevice(deviceID) != cudaSuccess) {
        *errorMsg = "Unable to set the CUDA device";
        return false;
    }

    cudaDeviceProp cudaDeviceProps;
    if (cudaGetDeviceProperties(&cudaDeviceProps, deviceID) != cudaSuccess) {
        *errorMsg = "Unable to get the CUDA device properties";
        return false;
    }
    *deviceName = cudaDeviceProps.name;

    if (cudaDeviceProps.major < 6) {
        *errorMsg = "GPU too old, must be compute capability 6 or greater";
        return false;
    }

    if (cudaStreamCreate(cudaStream) != cudaSuccess) {
        *errorMsg = "Unable to create the CUDA stream";
        return false;
    }

    if (optixInit() != OPTIX_SUCCESS) {
        *errorMsg = "Unable to initialize the Optix API";
        return false;
    }

    CUcontext cudaContext = 0;  // zero means take the current context
    OptixDeviceContextOptions options = {};
    if (optixDeviceContextCreate(cudaContext, &options, ctx) != OPTIX_SUCCESS) {
        cudaStreamDestroy(*cudaStream);
        *errorMsg = "Unable to create the Optix device context";
        return false;
    }

    // Log all messages, they can be filtered by level in the log callback function
    if (optixDeviceContextSetLogCallback(*ctx, logCallback, nullptr, 4) != OPTIX_SUCCESS) {
        optixDeviceContextDestroy(*ctx);
        cudaStreamDestroy(*cudaStream);
        *errorMsg = "Unable to set the Optix logging callback";
        return false;
    }

    return true;
}

} // namespace denoiser
} // namespace moonray

