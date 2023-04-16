#include <webgpu/webgpu.h>
#include <assert.h>
#include <stdio.h>
#include "helper.h"

//  ------------------------------- Adapter------------------------------------------------------------------

void onAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData) {
    struct AdapterUserData * userData = (struct AdapterUserData *)pUserData;
    if (status == WGPURequestAdapterStatus_Success) {
        userData->adapter = adapter;
    } else {
            printf( "Could not get WebGPU adapter: %s\n", message);
    }
    userData->requestEnded = true;
};

WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const * options) {
    struct AdapterUserData userData =  {NULL, false};

    // Call to the WebGPU request adapter procedure
    wgpuInstanceRequestAdapter(
        instance /* equivalent of navigator.gpu */,
        options,
        onAdapterRequestEnded,
        (void*)&userData
    );

    assert(userData.requestEnded);

    return userData.adapter;
}

//  ------------------------------- Device------------------------------------------------------------------

void onDeviceRequestEnded(WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData) {
    struct DeviceUserData * userData = (struct DeviceUserData *)(pUserData);
    if (status == WGPURequestDeviceStatus_Success) {
        userData->device = device;
    } else {
        printf( "Could not get WebGPU adapter: %s\n", message);
    }
    userData->requestEnded = true;
};

WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor) {

    struct DeviceUserData userData = {NULL, false};

    wgpuAdapterRequestDevice(
        adapter,
        descriptor,
        onDeviceRequestEnded,
        (void*)&userData
    );

    assert(userData.requestEnded);

    return userData.device;
}

void onDeviceError (WGPUErrorType type, char const* message, void* pUserData) {
    printf( "Uncaptured device error: type (%u)\n", type);
    if (message)
    printf( "Could not get WebGPU adapter: (%s)\n", message);
};

void setDefault(WGPULimits *limits) {
    limits->maxTextureDimension1D = 0;
    limits->maxTextureDimension2D = 0;
    limits->maxTextureDimension3D = 0;
    limits->maxTextureArrayLayers = 0;
    limits->maxBindGroups = 0;
    limits->maxBindingsPerBindGroup = 0;
    limits->maxDynamicUniformBuffersPerPipelineLayout = 0;
    limits->maxDynamicStorageBuffersPerPipelineLayout = 0;
    limits->maxSampledTexturesPerShaderStage = 0;
    limits->maxSamplersPerShaderStage = 0;
    limits->maxStorageBuffersPerShaderStage = 0;
    limits->maxStorageTexturesPerShaderStage = 0;
    limits->maxUniformBuffersPerShaderStage = 0;
    limits->maxUniformBufferBindingSize = 0;
    limits->maxStorageBufferBindingSize = 0;
    limits->minUniformBufferOffsetAlignment = 0;
    limits->minStorageBufferOffsetAlignment = 0;
    limits->maxVertexBuffers = 0;
    limits->maxBufferSize = 0;
    limits->maxVertexAttributes = 0;
    limits->maxVertexBufferArrayStride = 0;
    limits->maxInterStageShaderComponents = 0;
    limits->maxInterStageShaderVariables = 0;
    limits->maxColorAttachments = 0;
    limits->maxColorAttachmentBytesPerSample = 0;
    limits->maxComputeWorkgroupStorageSize = 0;
    limits->maxComputeInvocationsPerWorkgroup = 0;
    limits->maxComputeWorkgroupSizeX = 0;
    limits->maxComputeWorkgroupSizeY = 0;
    limits->maxComputeWorkgroupSizeZ = 0;
    limits->maxComputeWorkgroupsPerDimension = 0;
}

void cCallback(WGPUErrorType type, char const* message, void* userdata) {
	printf( "Device error: type  (%u)\n", type);
    if (message)
    printf( "message: (%s)\n", message);
};


void onDeviceLost(WGPUDeviceLostReason reason, char const* message, void* userdata) {
	printf( "Device lost: reason  (%u)\n", reason);
    if (message)
    printf( "message: (%s)\n", message);
};
