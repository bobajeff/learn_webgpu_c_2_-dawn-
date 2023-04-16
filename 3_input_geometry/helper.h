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

void setDefault(WGPULimits *limits);

void cCallback(WGPUErrorType type, char const* message, void* userdata);
void onDeviceLost(WGPUDeviceLostReason reason, char const* message, void* userdata);
#endif