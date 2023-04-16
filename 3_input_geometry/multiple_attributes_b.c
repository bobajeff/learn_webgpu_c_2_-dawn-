#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <webgpu/webgpu.h>
// #include <webgpu/wgpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <glfw3webgpu.h>
#include <assert.h>
#include "helper.h"

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

	printf("Requesting adapter...\n");
	WGPUSurface surface = glfwGetWGPUSurface(instance, window);
	WGPURequestAdapterOptions adapterOpts = {};
	adapterOpts.compatibleSurface = surface;
	WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);
	printf( "Got adapter: %p\n", adapter);

	printf("Requesting device...\n");
	WGPURequiredLimits requiredLimits = {};
	setDefault(&requiredLimits.limits);
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
	WGPUDeviceDescriptor deviceDesc;
	deviceDesc.nextInChain = NULL;
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeaturesCount = 0;
	deviceDesc.requiredLimits = &requiredLimits;
	deviceDesc.defaultQueue.label = "The default queue";
	WGPUDevice device = requestDevice(adapter, &deviceDesc);
	printf( "Got device: %p\n", device);

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

	// We now have 2 attributes
	WGPUVertexBufferLayout vertexBufferLayouts[2];

	// Position attribute remains untouched
	WGPUVertexAttribute positionAttrib = {};
	positionAttrib.shaderLocation = 0;
	positionAttrib.format = WGPUVertexFormat_Float32x2; // size of position
	positionAttrib.offset = 0;

	vertexBufferLayouts[0].attributeCount = 1;
	vertexBufferLayouts[0].attributes = &positionAttrib;
	vertexBufferLayouts[0].arrayStride = 2 * sizeof(float); // stride = size of position
	vertexBufferLayouts[0].stepMode = WGPUVertexStepMode_Vertex;

	// Position attribute
	WGPUVertexAttribute colorAttrib = {};
	colorAttrib.shaderLocation = 1;
	colorAttrib.format = WGPUVertexFormat_Float32x3; // size of color
	colorAttrib.offset = 0;

	vertexBufferLayouts[1].attributeCount = 1;
	vertexBufferLayouts[1].attributes = &colorAttrib;
	vertexBufferLayouts[1].arrayStride = 3 * sizeof(float); // stride = size of color
	vertexBufferLayouts[1].stepMode = WGPUVertexStepMode_Vertex;

	pipelineDesc.vertex.bufferCount = (uint32_t)(sizeof vertexBufferLayouts/ sizeof vertexBufferLayouts[0]);
	pipelineDesc.vertex.buffers = vertexBufferLayouts;

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
	blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
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
	pipelineDesc.multisample.mask = ~0u;
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	WGPUPipelineLayoutDescriptor layoutDesc = {};
	layoutDesc.bindGroupLayoutCount = 0;
	layoutDesc.bindGroupLayouts = NULL;
	WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);
	pipelineDesc.layout = layout;

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
	WGPUBufferDescriptor bufferDesc = {};
	bufferDesc.nextInChain = NULL;
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
	bufferDesc.mappedAtCreation = false;

	bufferDesc.size = sizeof positionData;
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

		WGPUCommandEncoderDescriptor commandEncoderDesc = {};
		commandEncoderDesc.label = "Command Encoder";
		WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDesc);
		
		WGPURenderPassDescriptor renderPassDesc = {};

		WGPURenderPassColorAttachment renderPassColorAttachment = {};
		renderPassColorAttachment.view = nextTexture;
		renderPassColorAttachment.resolveTarget = NULL;
		renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
		renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
		renderPassColorAttachment.clearValue = (WGPUColor){ 0.05, 0.05, 0.05, 1.0 };
		renderPassDesc.colorAttachmentCount = 1;
		renderPassDesc.colorAttachments = &renderPassColorAttachment;

		renderPassDesc.depthStencilAttachment = NULL;
		renderPassDesc.timestampWriteCount = 0;
		renderPassDesc.timestampWrites = NULL;
		WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

		wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);

		// Set vertex buffers while encoding the render pass
		wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, positionBuffer, 0, sizeof positionData);
		wgpuRenderPassEncoderSetVertexBuffer(renderPass, 1, colorBuffer, 0, sizeof colorData);

		// We use the `vertexCount` variable instead of hard-coding the vertex count
		wgpuRenderPassEncoderDraw(renderPass, vertexCount, 1, 0, 0);

		wgpuRenderPassEncoderEnd(renderPass);
		
		// wgpuTextureViewDrop(nextTexture);

		WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
		cmdBufferDescriptor.label = "Command buffer";
		WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
		wgpuQueueSubmit(queue, 1, &command);

		wgpuSwapChainPresent(swapChain);
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}