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
    *limits = (WGPULimits){
        .maxTextureDimension1D = 0,
        .maxTextureDimension2D = 0,
        .maxTextureDimension3D = 0,
        .maxTextureArrayLayers = 0,
        .maxBindGroups = 0,
        .maxBindingsPerBindGroup = 0,
        .maxDynamicUniformBuffersPerPipelineLayout = 0,
        .maxDynamicStorageBuffersPerPipelineLayout = 0,
        .maxSampledTexturesPerShaderStage = 0,
        .maxSamplersPerShaderStage = 0,
        .maxStorageBuffersPerShaderStage = 0,
        .maxStorageTexturesPerShaderStage = 0,
        .maxUniformBuffersPerShaderStage = 0,
        .maxUniformBufferBindingSize = 0,
        .maxStorageBufferBindingSize = 0,
        .minUniformBufferOffsetAlignment = 0,
        .minStorageBufferOffsetAlignment = 0,
        .maxVertexBuffers = 0,
        .maxBufferSize = 0,
        .maxVertexAttributes = 0,
        .maxVertexBufferArrayStride = 0,
        .maxInterStageShaderComponents = 0,
        .maxInterStageShaderVariables = 0,
        .maxColorAttachments = 0,
        .maxColorAttachmentBytesPerSample = 0,
        .maxComputeWorkgroupStorageSize = 0,
        .maxComputeInvocationsPerWorkgroup = 0,
        .maxComputeWorkgroupSizeX = 0,
        .maxComputeWorkgroupSizeY = 0,
        .maxComputeWorkgroupSizeZ = 0,
        .maxComputeWorkgroupsPerDimension = 0
    };

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
