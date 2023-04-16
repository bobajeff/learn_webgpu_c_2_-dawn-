#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <webgpu/webgpu.h>
// #include <webgpu/wgpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <glfw3webgpu.h>
// #include "helper.h"
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

//----------------------------------main

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
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    GLFWwindow *window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
    if (!window) {
        printf("Could not open window!\n");
        glfwTerminate();
        return 1;
    }

    //------------------ADAPTER
	printf("Requesting adapter...\n");
	WGPUSurface surface = glfwGetWGPUSurface(instance, window);
	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.compatibleSurface = surface;
	WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);
	printf( "Got adapter: %p\n", adapter);

    //------------------DEVICE

    printf("Requesting device...\n");
    // If you do not use webgpu.hpp, I suggest you create a function to init the
    // WGPULimits structure somewhere.

    WGPURequiredLimits requiredLimits = {};
    setDefault(&requiredLimits.limits);
    // We use at most 1 vertex attribute for now
    requiredLimits.limits.maxVertexAttributes = 1;
    // We should also tell that we use 1 vertex buffers
    requiredLimits.limits.maxVertexBuffers = 1;
    
    // need these limits for it to run on my machine
    requiredLimits.limits.minUniformBufferOffsetAlignment = 64;
    requiredLimits.limits.minStorageBufferOffsetAlignment = 32;
    requiredLimits.limits.maxVertexBufferArrayStride = 8;
    requiredLimits.limits.maxBufferSize = 48;

    WGPUDeviceDescriptor deviceDesc = {};
    deviceDesc.nextInChain = NULL;
    deviceDesc.label = "My Device"; // anything works here, that's your call
    deviceDesc.requiredLimits = &requiredLimits;
    WGPUDevice device = requestDevice(adapter, &deviceDesc);


    WGPUSupportedLimits supportedLimits;

    wgpuAdapterGetLimits(adapter, &supportedLimits);
    printf( "adapter.maxVertexAttributes: %i\n", supportedLimits.limits.maxVertexAttributes);
    printf( "adapter.minStorageBufferOffsetAlignment: %i\n", supportedLimits.limits.minStorageBufferOffsetAlignment);
    printf( "adapter.minUniformBufferOffsetAlignment: %i\n", supportedLimits.limits.minUniformBufferOffsetAlignment);

    wgpuDeviceGetLimits(device, &supportedLimits);
    printf( "device.maxVertexAttributes: %i\n", supportedLimits.limits.maxVertexAttributes);
    printf( "device.maxVertexBufferArrayStride: %u\n", supportedLimits.limits.maxVertexBufferArrayStride);
    printf( "device.maxBufferSize: %lu\n", supportedLimits.limits.maxBufferSize);

    // Personally I get:
    //   adapter.maxVertexAttributes: 32
    //   device.maxVertexAttributes: 16

	// Add an error callback for more debug info


	wgpuDeviceSetUncapturedErrorCallback(device, cCallback, NULL);

	WGPUQueue queue = wgpuDeviceGetQueue(device);

    printf( "Creating swapchain...\n");
	WGPUTextureFormat swapChainFormat = WGPUTextureFormat_BGRA8Unorm;
	WGPUSwapChainDescriptor swapChainDesc = {};
	swapChainDesc.width = 640;
	swapChainDesc.height = 480;
	swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
	swapChainDesc.format = swapChainFormat;
	swapChainDesc.presentMode = WGPUPresentMode_Fifo;
	WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDesc);
	printf( "Swapchain: %p\n", swapChain);

    printf( "Creating shader module...\n");
	const char* shaderSource = "                                                                \
    @vertex                                                                                     \
    fn vs_main(@location(0) in_vertex_position: vec2<f32>) -> @builtin(position) vec4<f32> {    \
        return vec4<f32>(in_vertex_position, 0.0, 1.0);                                         \
    }                                                                                           \
    @fragment                                                                                   \
    fn fs_main() -> @location(0) vec4<f32> {                                                    \
        return vec4<f32>(0.0, 0.4, 1.0, 1.0);                                                   \
    }                                                                                           \
    ";

	WGPUShaderModuleWGSLDescriptor shaderCodeDesc = {};
	shaderCodeDesc.chain.next = NULL;
	shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
	shaderCodeDesc.source = shaderSource;
	WGPUShaderModuleDescriptor shaderDesc = {};
	// shaderDesc.hintCount = 0;
	// shaderDesc.hints = NULL;
	shaderDesc.nextInChain = &shaderCodeDesc.chain;
	WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
	printf( "Shader module: %p\n", shaderModule);

	printf( "Creating render pipeline...\n");
	WGPURenderPipelineDescriptor pipelineDesc = {};

	// Vertex fetch
	WGPUVertexAttribute vertexAttrib;
	// == Per attribute ==
	// Corresponds to @location(...)
	vertexAttrib.shaderLocation = 0;
	// Means vec2<f32> in the shader
	vertexAttrib.format = WGPUVertexFormat_Float32x2;
	// Index of the first element
	vertexAttrib.offset = 0;

	WGPUVertexBufferLayout vertexBufferLayout = {};
	// [...] Build vertex buffer layout
	vertexBufferLayout.attributeCount = 1;
	vertexBufferLayout.attributes = &vertexAttrib;
	// == Common to attributes from the same buffer ==
	vertexBufferLayout.arrayStride = 2 * sizeof(float);
	vertexBufferLayout.stepMode = WGPUVertexStepMode_Vertex;

	pipelineDesc.vertex.bufferCount = 1;
	pipelineDesc.vertex.buffers = &vertexBufferLayout;

	pipelineDesc.vertex.module = shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = NULL;

	pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
	pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
	pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
	pipelineDesc.primitive.cullMode = WGPUCullMode_None;

	WGPUFragmentState fragmentState = {};
	pipelineDesc.fragment = &fragmentState;
	fragmentState.module = shaderModule;
	fragmentState.entryPoint = "fs_main";
	fragmentState.constantCount = 0;
	fragmentState.constants = NULL;

	WGPUBlendState blendState = {};
	blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
	blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
	blendState.color.operation = WGPUBlendOperation_Add;
	blendState.alpha.srcFactor = WGPUBlendFactor_One;
	blendState.alpha.dstFactor = WGPUBlendFactor_One;
	blendState.alpha.operation = WGPUBlendOperation_Add;

    WGPUColorTargetState colorTarget = {};
    colorTarget.format = swapChainFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = WGPUColorWriteMask_All;

	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;
	
	pipelineDesc.depthStencil = NULL;

	pipelineDesc.multisample.count = 1;
	pipelineDesc.multisample.mask = 0xFFFFFFFF;
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	WGPUPipelineLayoutDescriptor layoutDesc = {};
	layoutDesc.bindGroupLayoutCount = 0;
	layoutDesc.bindGroupLayouts = NULL;
	WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);
	pipelineDesc.layout = layout;

	WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
	printf( "Render pipeline: %p\n", pipeline);

	// Vertex buffer
	// There are 2 floats per vertex, one for x and one for y.
	// But in the end this is just a bunch of floats to the eyes of the GPU,
	// the *layout* will tell how to interpret this.
	float vertexData[] = {
		-0.5, -0.5,
		+0.5, -0.5,
		+0.0, +0.5,

		-0.55f, -0.5,
		-0.05f, +0.5,
		-0.55f, +0.5
	};
	int vertexCount = (int)((sizeof vertexData / sizeof vertexData[0]) / 2);

	// Create vertex buffer
	WGPUBufferDescriptor bufferDesc = {};
	bufferDesc.size = sizeof vertexData;
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
	bufferDesc.mappedAtCreation = false;
	WGPUBuffer vertexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

	// Upload geometry data to the buffer
	wgpuQueueWriteBuffer(queue, vertexBuffer, 0, vertexData, bufferDesc.size);


	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(swapChain);
		if (!nextTexture) {
			fprintf(stderr, "Cannot acquire next swap chain texture\n");
			return 1;
		}

		WGPUCommandEncoderDescriptor commandEncoderDesc = {};
		commandEncoderDesc.label = "Command Encoder";
		WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDesc);
		
		WGPURenderPassDescriptor renderPassDesc = {};

		WGPURenderPassColorAttachment renderPassColorAttachment = {};
		renderPassColorAttachment.view = nextTexture;
		renderPassColorAttachment.resolveTarget = NULL;
		renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
		renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
		renderPassColorAttachment.clearValue = (WGPUColor){ 0.9, 0.1, 0.2, 1.0 };
		renderPassDesc.colorAttachmentCount = 1;
		renderPassDesc.colorAttachments = &renderPassColorAttachment;

		renderPassDesc.depthStencilAttachment = NULL;
		renderPassDesc.timestampWriteCount = 0;
		renderPassDesc.timestampWrites = NULL;
		WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

		wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);

		// Set vertex buffer while encoding the render pass
		wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, vertexBuffer, 0, sizeof vertexData);

		// We use the `vertexCount` variable instead of hard-coding the vertex count
		wgpuRenderPassEncoderDraw(renderPass, vertexCount, 1, 0, 0);

		wgpuRenderPassEncoderEnd(renderPass);
		
		// wgpuTextureViewDrop(nextTexture);
		wgpuTextureViewRelease(nextTexture);

		WGPUCommandBufferDescriptor cmdBufferDescriptor;
		cmdBufferDescriptor.label = "Command buffer";
		WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
		wgpuQueueSubmit(queue, 1, &command);

		wgpuSwapChainPresent(swapChain);
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}