#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
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

	printf("Requesting adapter...\n");
	WGPUSurface surface = glfwGetWGPUSurface(instance, window);
	WGPURequestAdapterOptions adapterOpts = {
		.compatibleSurface = surface
	};
	WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);
	printf( "Got adapter: %p\n", adapter);

	printf("Requesting device...\n");
	WGPURequiredLimits requiredLimits = {
		.limits = DEFAULT_WGPU_LIMITS
	};
	requiredLimits.limits.maxVertexAttributes = 2;
	requiredLimits.limits.maxVertexBuffers = 2;
	// need these limits for it to run on my machine
    requiredLimits.limits.minUniformBufferOffsetAlignment = 64;
    requiredLimits.limits.minStorageBufferOffsetAlignment = 32;
    requiredLimits.limits.maxVertexBufferArrayStride = 12;
    requiredLimits.limits.maxBufferSize = 72;
	// this has to be set to 3 or I get
	// message: (Stage { stage: VERTEX, error: TooManyVaryings { used: 3, limit: 0 }
	requiredLimits.limits.maxInterStageShaderComponents = 3;
	WGPUDeviceDescriptor deviceDesc = {
		.nextInChain = NULL,
		.label = "My Device",
		.requiredFeaturesCount = 0,
		.requiredLimits = &requiredLimits,
		.defaultQueue.label = "The default queue"
	};
	WGPUDevice device = requestDevice(adapter, &deviceDesc);
	printf( "Got device: %p\n", device);

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
	const char* shaderSource = 
	"struct VertexInput {\n"
	"	@location(0) position: vec2<f32>,\n"
	"	@location(1) color: vec3<f32>,\n"
	"};\n"
	"\n"
	"struct VertexOutput {\n"
	"	@builtin(position) position: vec4<f32>,\n"
	"	@location(0) color: vec3<f32>,\n"
	"};\n"
	"\n"
	"@vertex\n"
	"fn vs_main(in: VertexInput) -> VertexOutput {\n"
	"	var out: VertexOutput;\n"
	"	out.position = vec4<f32>(in.position, 0.0, 1.0);\n"
	"	out.color = in.color;\n"
	"	return out;\n"
	"}\n"
	"\n"
	"@fragment\n"
	"fn fs_main(in: VertexOutput) -> @location(0) vec4<f32> {\n"
	"	return vec4<f32>(in.color, 1.0);\n"
	"}\n"
	;

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
	
	// We now have 2 attributes
	WGPUVertexBufferLayout vertexBufferLayouts[2];

	// Position attribute remains untouched
	WGPUVertexAttribute positionAttrib = {
		.shaderLocation = 0,
		.format = WGPUVertexFormat_Float32x2,
		.offset = 0
	};

	vertexBufferLayouts[0] = (WGPUVertexBufferLayout){
		.attributeCount = 1,
		.attributes = &positionAttrib,
		.arrayStride = 2 * sizeof(float),
		.stepMode = WGPUVertexStepMode_Vertex
	};
	
	// Position attribute
	WGPUVertexAttribute colorAttrib = {
		.shaderLocation = 1,
		.format = WGPUVertexFormat_Float32x3,
		.offset = 0
	};

	vertexBufferLayouts[1] = (WGPUVertexBufferLayout){
		.attributeCount = 1,
		.attributes = &colorAttrib,
		.arrayStride = 3 * sizeof(float),
		.stepMode = WGPUVertexStepMode_Vertex
	};

	WGPUBlendState blendState = {
		.color = (WGPUBlendComponent){
			.srcFactor = WGPUBlendFactor_SrcAlpha,
			.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha,
			.operation = WGPUBlendOperation_Add
		},
		.alpha = (WGPUBlendComponent){
			.srcFactor = WGPUBlendFactor_Zero,
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
			.bufferCount = (uint32_t)(sizeof vertexBufferLayouts/ sizeof vertexBufferLayouts[0]),
			.buffers = vertexBufferLayouts,

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
			.mask = ~0u,
			.alphaToCoverageEnabled = false
		},
		.layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc)
	};

	WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
	printf( "Render pipeline: %p\n", pipeline);

	// Vertex buffers
	// x0, y0, x1, y1, ...
	float positionData[] = {
		-0.5, -0.5,
		+0.5, -0.5,
		+0.0, +0.5,
		-0.55f, -0.5,
		-0.05f, +0.5,
		-0.55f, +0.5
	};

	// r0,  g0,  b0, r1,  g1,  b1, ...
	float colorData[] = {
		1.0, 0.0, 0.0,
		0.0, 1.0, 0.0,
		0.0, 0.0, 1.0,
		1.0, 1.0, 0.0,
		1.0, 0.0, 1.0,
		0.0, 1.0, 1.0
	};
	// We now divide the vector size by 5 fields.
	int vertexCount = (int)((sizeof positionData / sizeof positionData[0]) / 2);
	assert((int)((sizeof colorData / sizeof colorData[0])/3));

	// Create vertex buffers
	WGPUBufferDescriptor bufferDesc = {
		.nextInChain = NULL,
		.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
		.mappedAtCreation = false,
		.size = sizeof positionData
	};
	WGPUBuffer positionBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
	wgpuQueueWriteBuffer(queue, positionBuffer, 0, positionData, bufferDesc.size);

	bufferDesc.size = sizeof colorData;
	WGPUBuffer colorBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
	wgpuQueueWriteBuffer(queue, colorBuffer, 0, colorData, bufferDesc.size);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(swapChain);
		if (!nextTexture) {
			fprintf(stderr, "Cannot acquire next swap chain texture\n");
			return 1;
		}

		WGPUCommandEncoderDescriptor commandEncoderDesc = {.label = "Command Encoder"};
		WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDesc);

		WGPURenderPassColorAttachment renderPassColorAttachment = {
			.view = nextTexture,
			.resolveTarget = NULL,
			.loadOp = WGPULoadOp_Clear,
			.storeOp = WGPUStoreOp_Store,
			.clearValue = (WGPUColor){ 0.05, 0.05, 0.05, 1.0 }
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

		// Set vertex buffers while encoding the render pass
		wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, positionBuffer, 0, sizeof positionData);
		wgpuRenderPassEncoderSetVertexBuffer(renderPass, 1, colorBuffer, 0, sizeof colorData);

		// We use the `vertexCount` variable instead of hard-coding the vertex count
		wgpuRenderPassEncoderDraw(renderPass, vertexCount, 1, 0, 0);

		wgpuRenderPassEncoderEnd(renderPass);
		
		WGPUCommandBufferDescriptor cmdBufferDescriptor = {.label = "Command buffer"};
		WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
		wgpuQueueSubmit(queue, 1, &command);

		wgpuSwapChainPresent(swapChain);
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}