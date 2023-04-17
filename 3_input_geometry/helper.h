#ifndef HELPER_HEADER_FILE
#define HELPER_HEADER_FILE

#include <webgpu/webgpu.h>

//  ------------------------------- Adapter------------------------------------------------------------------
struct AdapterUserData {
    WGPUAdapter adapter;
    bool requestEnded;
};

void onAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData);

WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const * options);

//  ------------------------------- Device------------------------------------------------------------------
struct DeviceUserData {
    WGPUDevice device;
    bool requestEnded;
};

void onDeviceRequestEnded(WGPURequestDeviceStatus status, WGPUDevice device, char const * message, void * pUserData);

WGPUDevice requestDevice(WGPUAdapter adapter, WGPUDeviceDescriptor const * descriptor);

void onDeviceError (WGPUErrorType type, char const* message, void* pUserData);

void setDefault(WGPULimits *limits); //replace setDefault with DEFAULT_WGPU_LIMITS
static const WGPULimits DEFAULT_WGPU_LIMITS = {
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

void cCallback(WGPUErrorType type, char const* message, void* userdata);
void onDeviceLost(WGPUDeviceLostReason reason, char const* message, void* userdata);
#endif