#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <webgpu/webgpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <glfw3webgpu.h>
#include <assert.h>

//  ------------------------------- Adapter------------------------------------------------------------------
struct AdapterUserData {
    WGPUAdapter adapter;
    bool requestEnded;
};

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
struct DeviceUserData {
    WGPUDevice device;
    bool requestEnded;
};

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


void inspectDevice(WGPUDevice device) {
    WGPUFeatureName * features;
	size_t featureCount = wgpuDeviceEnumerateFeatures(device, NULL);
    printf("featureCount:%zu\n", featureCount);
    
    features = malloc(featureCount * sizeof(WGPUFeatureName));
	wgpuDeviceEnumerateFeatures(device, features);

    printf("Device features:\n");
    for (int i=0; i < featureCount; i++){
        printf(" - %i\n", features[i]);
    }
     free(features);

	WGPUSupportedLimits limits = {};
	limits.nextInChain = NULL;
	bool success = wgpuDeviceGetLimits(device, &limits);
	if (success) {
		printf("Device limits:\n");
		printf(" - maxTextureDimension1D: %i\n", limits.limits.maxTextureDimension1D);
		printf(" - maxTextureDimension2D: %i\n", limits.limits.maxTextureDimension2D);
		printf(" - maxTextureDimension3D: %i\n", limits.limits.maxTextureDimension3D);
		printf(" - maxTextureArrayLayers: %i\n", limits.limits.maxTextureArrayLayers);
		printf(" - maxBindGroups: %i\n", limits.limits.maxBindGroups);
		printf(" - maxDynamicUniformBuffersPerPipelineLayout: %i\n", limits.limits.maxDynamicUniformBuffersPerPipelineLayout);
		printf(" - maxDynamicStorageBuffersPerPipelineLayout: %i\n", limits.limits.maxDynamicStorageBuffersPerPipelineLayout);
		printf(" - maxSampledTexturesPerShaderStage: %i\n", limits.limits.maxSampledTexturesPerShaderStage);
		printf(" - maxSamplersPerShaderStage: %i\n", limits.limits.maxSamplersPerShaderStage);
		printf(" - maxStorageBuffersPerShaderStage: %i\n", limits.limits.maxStorageBuffersPerShaderStage);
		printf(" - maxStorageTexturesPerShaderStage: %i\n", limits.limits.maxStorageTexturesPerShaderStage);
		printf(" - maxUniformBuffersPerShaderStage: %i\n", limits.limits.maxUniformBuffersPerShaderStage);
		printf(" - maxUniformBufferBindingSize: %lu\n", limits.limits.maxUniformBufferBindingSize);
		printf(" - maxStorageBufferBindingSize: %lu\n", limits.limits.maxStorageBufferBindingSize);
		printf(" - minUniformBufferOffsetAlignment: %i\n", limits.limits.minUniformBufferOffsetAlignment);
		printf(" - minStorageBufferOffsetAlignment: %i\n", limits.limits.minStorageBufferOffsetAlignment);
		printf(" - maxVertexBuffers: %i\n", limits.limits.maxVertexBuffers);
		printf(" - maxVertexAttributes: %i\n", limits.limits.maxVertexAttributes);
		printf(" - maxVertexBufferArrayStride: %i\n", limits.limits.maxVertexBufferArrayStride);
		printf(" - maxInterStageShaderComponents: %i\n", limits.limits.maxInterStageShaderComponents);
		printf(" - maxComputeWorkgroupStorageSize: %i\n", limits.limits.maxComputeWorkgroupStorageSize);
		printf(" - maxComputeInvocationsPerWorkgroup: %i\n", limits.limits.maxComputeInvocationsPerWorkgroup);
		printf(" - maxComputeWorkgroupSizeX: %i\n", limits.limits.maxComputeWorkgroupSizeX);
		printf(" - maxComputeWorkgroupSizeY: %i\n", limits.limits.maxComputeWorkgroupSizeY);
		printf(" - maxComputeWorkgroupSizeZ: %i\n", limits.limits.maxComputeWorkgroupSizeZ);
		printf(" - maxComputeWorkgroupsPerDimension: %i\n", limits.limits.maxComputeWorkgroupsPerDimension);
	}
}

int main(int argc, char *argv[]) {
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = NULL;
    WGPUInstance instance = wgpuCreateInstance(&desc);
    if (!instance) {
        fprintf(stderr, "Could not initialize WebGPU!\n");
        return 1;
    }
    if (!glfwInit()) {
        printf("Could not initialize GLFW!\n");
        return 1;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); 
    GLFWwindow *window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
    if (!window) {
        printf("Could not open window!\n");
        glfwTerminate();
        return 1;
    }

    printf("Requesting adapter...\n");

    WGPUSurface surface = glfwGetWGPUSurface(instance, window);

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = NULL;
    adapterOpts.compatibleSurface = surface;

    WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);

    printf( "Got adapter: %p\n", adapter);

    printf("Requesting device...\n");

    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = NULL;
    deviceDesc.label = "My Device"; // anything works here, that's your call
    deviceDesc.requiredFeaturesCount = 0; // we do not require any specific feature
    deviceDesc.requiredLimits = NULL; // we do not require any specific limit
    deviceDesc.defaultQueue.nextInChain = NULL;
    deviceDesc.defaultQueue.label = "The default queue";
    WGPUDevice device = requestDevice(adapter, &deviceDesc);

    printf( "Got device: %p\n", device);
    
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, NULL /* pUserData */);

    inspectDevice(device);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}