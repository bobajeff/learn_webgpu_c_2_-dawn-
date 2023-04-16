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
	requiredLimits.limits.maxVertexBuffers = 1;
	// need these limits for it to run on my machine
    requiredLimits.limits.minUniformBufferOffsetAlignment = 64;
    requiredLimits.limits.minStorageBufferOffsetAlignment = 32;
    requiredLimits.limits.maxVertexBufferArrayStride = 20;
    requiredLimits.limits.maxBufferSize = 120;
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
	"	let ratio = 640.0 / 480.0;\n"
	"	out.position = vec4<f32>(in.position.x, in.position.y * ratio, 0.0, 1.0);\n"
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

	// Vertex fetch
	// We now have 2 attributes
	WGPUVertexAttribute vertexAttribs[2];

	// Position attribute
	vertexAttribs[0].shaderLocation = 0;
	vertexAttribs[0].format = WGPUVertexFormat_Float32x2;
	vertexAttribs[0].offset = 0;

	// Color attribute
	vertexAttribs[1].shaderLocation = 1;
	vertexAttribs[1].format = WGPUVertexFormat_Float32x3; // different type!
	vertexAttribs[1].offset = 2 * sizeof(float); // non null offset!

	WGPUVertexBufferLayout vertexBufferLayout = {};
	vertexBufferLayout.attributeCount = 2;
	vertexBufferLayout.attributes = vertexAttribs;
	// The new stride
	vertexBufferLayout.arrayStride = 5 * sizeof(float);
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

	// Vertex buffer
	// The de-duplicated list of point positions
	float pointData[] = {
		// x,   y,     r,   g,   b
		-0.5, -0.5,   1.0, 0.0, 0.0,
		+0.5, -0.5,   0.0, 1.0, 0.0,
		+0.5, +0.5,   0.0, 0.0, 1.0,
		-0.5, +0.5,   1.0, 1.0, 0.0
	};

	// Create vertex buffer
	WGPUBufferDescriptor bufferDesc = {};
	bufferDesc.size = sizeof pointData;
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex;
	bufferDesc.mappedAtCreation = false;
	WGPUBuffer vertexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
	wgpuQueueWriteBuffer(queue, vertexBuffer, 0, pointData, bufferDesc.size);

	// Index Buffer
	// This is a list of indices referencing positions in the pointData
	uint16_t indexData[] = {
		0, 1, 2, // Triangle #0
		0, 2, 3  // Triangle #1
	};

	int indexCount = (int)(sizeof(indexData)/sizeof(indexData[0]));

	// Create index buffer
	// (we reuse the bufferDesc initialized for the vertexBuffer)
	bufferDesc.size = sizeof indexData;
	bufferDesc.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index;
	bufferDesc.mappedAtCreation = false;
	WGPUBuffer indexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
	wgpuQueueWriteBuffer(queue, indexBuffer, 0, indexData, bufferDesc.size);

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

		// Set both vertex and index buffers
		wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, vertexBuffer, 0, sizeof pointData);

		// The second argument must correspond to the choice of uint16_t or uint32_t
		// we've done when creating the index buffer.
		wgpuRenderPassEncoderSetIndexBuffer(renderPass, indexBuffer, WGPUIndexFormat_Uint16, 0, sizeof(indexData));

		// Replace `draw()` with `drawIndexed()` and `vertexCount` with `indexCount`
		// The extra argument is an offset within the index buffer.
		wgpuRenderPassEncoderDrawIndexed(renderPass, indexCount, 1, 0, 0, 0);

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