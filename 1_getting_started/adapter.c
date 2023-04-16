#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <glfw3webgpu.h>
#include <assert.h>

// A simple structure holding the local information shared with the
// onAdapterRequestEnded callback.
struct UserData {
    WGPUAdapter adapter;
    bool requestEnded;
};

void onAdapterRequestEnded(WGPURequestAdapterStatus status, WGPUAdapter adapter, char const * message, void * pUserData) {
    struct UserData * userData = (struct UserData *)pUserData;
    if (status == WGPURequestAdapterStatus_Success) {
        userData->adapter = adapter;
    } else {
            printf( "Could not get WebGPU adapter: %s\n", message);
    }
    userData->requestEnded = true;
};


WGPUAdapter requestAdapter(WGPUInstance instance, WGPURequestAdapterOptions const * options) {
    struct UserData userData =  {NULL, false};

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

void inspectAdapter(WGPUAdapter adapter) {
	// std::vector<WGPUFeatureName> features;

  WGPUFeatureName* features;
	size_t featureCount = wgpuAdapterEnumerateFeatures(adapter, NULL);

	// features.resize(featureCount);
    features = malloc(featureCount * sizeof(WGPUFeatureName));

	wgpuAdapterEnumerateFeatures(adapter, features);

	printf("Adapter features:\n");
	for (int i=0; i < featureCount; i++){
        printf(" - %i\n", features[i]);
		// printf(" - %i\n", f ) ;
	}
     free(features);

	WGPUSupportedLimits limits = {};
	limits.nextInChain = NULL;
	bool success = wgpuAdapterGetLimits(adapter, &limits);
	if (success) {
		printf("Adapter limits:\n");
		printf(" - maxTextureDimension1D: %i\n",limits.limits.maxTextureDimension1D);
		printf(" - maxTextureDimension2D: %i\n",limits.limits.maxTextureDimension2D);
		printf(" - maxTextureDimension3D: %i\n",limits.limits.maxTextureDimension3D);
		printf(" - maxTextureArrayLayers: %i\n",limits.limits.maxTextureArrayLayers);
		printf(" - maxBindGroups: %i\n",limits.limits.maxBindGroups);
		printf(" - maxDynamicUniformBuffersPerPipelineLayout: %i\n",limits.limits.maxDynamicUniformBuffersPerPipelineLayout);
		printf(" - maxDynamicStorageBuffersPerPipelineLayout: %i\n",limits.limits.maxDynamicStorageBuffersPerPipelineLayout);
		printf(" - maxSampledTexturesPerShaderStage: %i\n",limits.limits.maxSampledTexturesPerShaderStage);
		printf(" - maxSamplersPerShaderStage: %i\n",limits.limits.maxSamplersPerShaderStage);
		printf(" - maxStorageBuffersPerShaderStage: %i\n",limits.limits.maxStorageBuffersPerShaderStage);
		printf(" - maxStorageTexturesPerShaderStage: %i\n",limits.limits.maxStorageTexturesPerShaderStage);
		printf(" - maxUniformBuffersPerShaderStage: %i\n",limits.limits.maxUniformBuffersPerShaderStage);
		printf(" - maxUniformBufferBindingSize: %lu\n",limits.limits.maxUniformBufferBindingSize);
		printf(" - maxStorageBufferBindingSize: %lu\n",limits.limits.maxStorageBufferBindingSize);
		printf(" - minUniformBufferOffsetAlignment: %i\n",limits.limits.minUniformBufferOffsetAlignment);
		printf(" - minStorageBufferOffsetAlignment: %i\n",limits.limits.minStorageBufferOffsetAlignment);
		printf(" - maxVertexBuffers: %i\n",limits.limits.maxVertexBuffers);
		printf(" - maxVertexAttributes: %i\n",limits.limits.maxVertexAttributes);
		printf(" - maxVertexBufferArrayStride: %i\n",limits.limits.maxVertexBufferArrayStride);
		printf(" - maxInterStageShaderComponents: %i\n",limits.limits.maxInterStageShaderComponents);
		printf(" - maxComputeWorkgroupStorageSize: %i\n",limits.limits.maxComputeWorkgroupStorageSize);
		printf(" - maxComputeInvocationsPerWorkgroup: %i\n",limits.limits.maxComputeInvocationsPerWorkgroup);
		printf(" - maxComputeWorkgroupSizeX: %i\n",limits.limits.maxComputeWorkgroupSizeX);
		printf(" - maxComputeWorkgroupSizeY: %i\n",limits.limits.maxComputeWorkgroupSizeY);
		printf(" - maxComputeWorkgroupSizeZ: %i\n",limits.limits.maxComputeWorkgroupSizeZ);
		printf(" - maxComputeWorkgroupsPerDimension: %i\n",limits.limits.maxComputeWorkgroupsPerDimension);
	}

	WGPUAdapterProperties properties = {};
	properties.nextInChain = NULL;
	wgpuAdapterGetProperties(adapter, &properties);
	printf("Adapter properties:\n");
	printf(" - vendorID: %i\n", properties.vendorID);
	printf(" - deviceID: %i\n", properties.deviceID);
	printf(" - name: %s\n", properties.name);
	if (properties.driverDescription) {
		printf(" - driverDescription: %p\n", properties.driverDescription);
	}
	printf(" - adapterType: %u\n", properties.adapterType);
	printf(" - backendType: %u\n", properties.backendType);
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

    inspectAdapter(adapter);

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}