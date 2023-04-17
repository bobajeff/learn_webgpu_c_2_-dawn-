#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <webgpu/webgpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <glfw3webgpu.h>
#include <assert.h>
#include "helper.h"

int main(int argc, char *argv[]) {
	WGPUInstanceDescriptor desc = { .nextInChain = NULL};
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
	WGPURequestAdapterOptions adapterOpts = {
		.compatibleSurface = surface
	};
	WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);
	printf( "Got adapter: %p\n", adapter);

    //------------------DEVICE

    printf("Requesting device...\n");
    WGPURequiredLimits requiredLimits = {
		.limits = DEFAULT_WGPU_LIMITS
	};
    // We use at most 1 vertex attribute for now
    requiredLimits.limits.maxVertexAttributes = 1;
    // We should also tell that we use 1 vertex buffers
    requiredLimits.limits.maxVertexBuffers = 1;
    
    // need these limits for it to run on my machine
    requiredLimits.limits.minUniformBufferOffsetAlignment = 64;
    requiredLimits.limits.minStorageBufferOffsetAlignment = 32;
    requiredLimits.limits.maxVertexBufferArrayStride = 8;
    requiredLimits.limits.maxBufferSize = 48;

	WGPUDeviceDescriptor deviceDesc = {
		.nextInChain = NULL,
		.label = "My Device",
		.requiredLimits = &requiredLimits,
	};
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
	WGPUSwapChainDescriptor swapChainDesc = {
		.width = 640,
		.height = 480,
		.usage = WGPUTextureUsage_RenderAttachment,
		.format = swapChainFormat,
		.presentMode = WGPUPresentMode_Fifo
	};
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

	WGPUShaderModuleWGSLDescriptor shaderCodeDesc = {
		.chain = (WGPUChainedStruct){
			.next = NULL,
			.sType = WGPUSType_ShaderModuleWGSLDescriptor
		},
		.source = shaderSource
	};
	WGPUShaderModuleDescriptor shaderDesc = {
		.nextInChain = &shaderCodeDesc.chain
	};
	WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
	printf( "Shader module: %p\n", shaderModule);

	printf( "Creating render pipeline...\n");
	

	// Vertex fetch
	WGPUVertexAttribute vertexAttrib = {
		// == Per attribute ==
		// Corresponds to @location(...)
		.shaderLocation = 0,
		// Means vec2<f32> in the shader
		.format = WGPUVertexFormat_Float32x2,
		// Index of the first element
		.offset = 0
	};

	// [...] Build vertex buffer layout
	WGPUVertexBufferLayout vertexBufferLayout = {
		.attributeCount = 1,
		.attributes = &vertexAttrib,
		// == Common to attributes from the same buffer ==
		.arrayStride = 2 * sizeof(float),
		.stepMode = WGPUVertexStepMode_Vertex
	};

		WGPUBlendState blendState = {
		.color = (WGPUBlendComponent){
			.srcFactor = WGPUBlendFactor_SrcAlpha,
			.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
			.operation = WGPUBlendOperation_Add
		},
		.alpha = (WGPUBlendComponent){
			.srcFactor = WGPUBlendFactor_One,
			.dstFactor = WGPUBlendFactor_One,
			.operation = WGPUBlendOperation_Add
		}
	};

    WGPUColorTargetState colorTarget = {
		.format = swapChainFormat,
		.blend = &blendState,
		.writeMask = WGPUColorWriteMask_All
	};

	WGPUFragmentState fragmentState = {
		.module = shaderModule,
		.entryPoint = "fs_main",
		.constantCount = 0,
		.constants = NULL,
		.targetCount = 1,
		.targets = &colorTarget
	};

	WGPUPipelineLayoutDescriptor layoutDesc = {
		.bindGroupLayoutCount = 0,
		.bindGroupLayouts = NULL
	};

	WGPURenderPipelineDescriptor pipelineDesc = {
		.vertex = (WGPUVertexState){
			.bufferCount = 1,
			.buffers = &vertexBufferLayout,

			.module = shaderModule,
			.entryPoint = "vs_main",
			.constantCount = 0,
			.constants = NULL
			},
		.primitive = (WGPUPrimitiveState){
			.topology = WGPUPrimitiveTopology_TriangleList,
			.stripIndexFormat = WGPUIndexFormat_Undefined,
			.frontFace = WGPUFrontFace_CCW,
			.cullMode = WGPUCullMode_None
		},
		.fragment = &fragmentState,
		.depthStencil = NULL,
		.multisample = (WGPUMultisampleState){
			.count = 1,
			.mask = 0xFFFFFFFF,
			.alphaToCoverageEnabled = false
		},
		.layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc)
	};

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
	WGPUBufferDescriptor bufferDesc = {
		.size = sizeof vertexData,
		.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
		.mappedAtCreation = false
	};
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

		WGPUCommandEncoderDescriptor commandEncoderDesc = {
			.label = "Command Encoder"
		};
		WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDesc);
		
		WGPURenderPassColorAttachment renderPassColorAttachment = {
			.view = nextTexture,
			.resolveTarget = NULL,
			.loadOp = WGPULoadOp_Clear,
			.storeOp = WGPUStoreOp_Store,
			.clearValue = (WGPUColor){ 0.9, 0.1, 0.2, 1.0 }
		};
		WGPURenderPassDescriptor renderPassDesc = {
			.colorAttachmentCount = 1,
			.colorAttachments = &renderPassColorAttachment,

			.depthStencilAttachment = NULL,
			.timestampWriteCount = 0,
			.timestampWrites = NULL
		};

		WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

		wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);

		// Set vertex buffer while encoding the render pass
		wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, vertexBuffer, 0, sizeof vertexData);

		// We use the `vertexCount` variable instead of hard-coding the vertex count
		wgpuRenderPassEncoderDraw(renderPass, vertexCount, 1, 0, 0);

		wgpuRenderPassEncoderEnd(renderPass);
		
		wgpuTextureViewRelease(nextTexture);

		WGPUCommandBufferDescriptor cmdBufferDescriptor = {
			.label = "Command buffer"
		};
		WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
		wgpuQueueSubmit(queue, 1, &command);

		wgpuSwapChainPresent(swapChain);
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}