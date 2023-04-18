#include <GLFW/glfw3.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <webgpu/webgpu.h>
#include <stdio.h>
#include <stdlib.h>
#include <glfw3webgpu.h>
#include "helper_v3.h"

typedef struct MyUniforms {
    float color[4];
    float time;
	float _pad[3];
} MyUniforms;
static_assert(sizeof(MyUniforms) % 16 == 0, "MyUniforms is multiple of 16");

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
	requiredLimits.limits.maxVertexBuffers = 1;
	// need these limits for it to run on my machine
    requiredLimits.limits.minUniformBufferOffsetAlignment = 64;
    requiredLimits.limits.minStorageBufferOffsetAlignment = 32;
	// We use at most 1 bind group for now
	requiredLimits.limits.maxBindGroups = 1;
	// We use at most 1 uniform buffer per stage
	requiredLimits.limits.maxUniformBuffersPerShaderStage = 1;
	// Uniform structs have a size of maximum 16 float (more than what we need)
	requiredLimits.limits.maxUniformBufferBindingSize = 16 * 4;
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
	wgpuDeviceSetDeviceLostCallback(device, onDeviceLost, NULL);

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

	WGPUShaderModule shaderModule = loadShaderModule(RESOURCE_DIR "/shader.wsl", device);
	printf( "Shader module: %p\n", shaderModule);

	printf( "Creating render pipeline...\n");

	// Vertex fetch
	// We now have 2 attributes
	WGPUVertexAttribute vertexAttribs[2];

	// Position attribute
	vertexAttribs[0] = (WGPUVertexAttribute){
		.shaderLocation = 0,
		.format = WGPUVertexFormat_Float32x3,
		.offset = 0
	};

	// Color attribute
	vertexAttribs[1] = (WGPUVertexAttribute){
		.shaderLocation = 1,
		.format = WGPUVertexFormat_Float32x3,
		.offset = 3 * sizeof(float)
	};

	WGPUVertexBufferLayout vertexBufferLayout = {
		.attributeCount = 2,
		.attributes = vertexAttribs,
		.arrayStride = 6 * sizeof(float),
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

	// Create binding layout
	WGPUBindGroupLayoutEntry bindingLayout = BIND_GROUP_DEFAULT;
	// The binding index as used in the @binding attribute in the shader
	bindingLayout.binding = 0;
	// The stage that needs to access this resource
	bindingLayout.visibility = WGPUShaderStage_Vertex | WGPUShaderStage_Fragment;
	bindingLayout.buffer.type = WGPUBufferBindingType_Uniform;
	bindingLayout.buffer.minBindingSize = sizeof(MyUniforms);

	// Create a bind group layout
	WGPUBindGroupLayoutDescriptor bindGroupLayoutDesc = {
		.entryCount = 1,
		.entries = &bindingLayout
	};
	WGPUBindGroupLayout bindGroupLayout = wgpuDeviceCreateBindGroupLayout(device, &bindGroupLayoutDesc);

	WGPUPipelineLayoutDescriptor layoutDesc = {
		.bindGroupLayoutCount = 1,
		.bindGroupLayouts = &bindGroupLayout
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
			.mask = ~0u,
			.alphaToCoverageEnabled = false
		},
		.layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc)
	};

	WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
	printf( "Render pipeline: %p\n", pipeline);

	struct GeometryData geometrydata = {malloc(sizeof(float)), 0, malloc(sizeof(size_t)), 0};
	bool success = loadGeometry(RESOURCE_DIR "/pyramid.txt", &geometrydata);
		if (!success) {
		fprintf(stderr, "Could not load geometry!\n");
		return 1;
	}

	float * pointData = geometrydata.pointData;
	size_t pointDataSize = geometrydata.pointDataSize;
	uint16_t * indexData = geometrydata.indexData;
	size_t indexDataSize = geometrydata.indexDataSize;

	// Create vertex buffer
	WGPUBufferDescriptor bufferDesc = {
		.size = pointDataSize,
		.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Vertex,
		.mappedAtCreation = false
	};
	WGPUBuffer vertexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
	wgpuQueueWriteBuffer(queue, vertexBuffer, 0, pointData, bufferDesc.size);

	int indexCount = indexDataSize/sizeof(indexData[0]);

	// Create index buffer
	// (we reuse the bufferDesc initialized for the vertexBuffer)
	// bufferDesc.size has to be indexCount × sizeof(float) or I get UnalignedBufferOffset(30) 
	// Not sure why. I'll have to look into it
	bufferDesc = (WGPUBufferDescriptor){
		.size = indexCount * sizeof(float),
		.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Index,
		.mappedAtCreation = false
	};
	WGPUBuffer indexBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);
	wgpuQueueWriteBuffer(queue, indexBuffer, 0, indexData, bufferDesc.size);

	//cleanup memory
	free(indexData);
	free(pointData);

	// Create uniform buffer
	// The buffer will only contain 1 float with the value of uTime
	bufferDesc = (WGPUBufferDescriptor){
		.size = sizeof(MyUniforms),
		.nextInChain = NULL,
		// Make sure to flag the buffer as BufferUsage::Uniform
		.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_Uniform,
		.mappedAtCreation = false
	};
	WGPUBuffer uniformBuffer = wgpuDeviceCreateBuffer(device, &bufferDesc);

	// Upload the initial value of the uniforms
	MyUniforms uniforms = {
		.color = { 0.0f, 1.0f, 0.4f, 1.0f },
		.time = 1.0f,
		};
	wgpuQueueWriteBuffer(queue, uniformBuffer, 0, &uniforms, sizeof(MyUniforms));

	// Create a binding
	WGPUBindGroupEntry binding = {
		.nextInChain = NULL,
		// The index of the binding (the entries in bindGroupDesc can be in any order)
		.binding = 0,
		// The buffer it is actually bound to
		.buffer = uniformBuffer,
		// We can specify an offset within the buffer, so that a single buffer can hold
		// multiple uniform blocks.
		.offset = 0,
		// And we specify again the size of the buffer.
		.size = sizeof(MyUniforms)
	};

	// A bind group contains one or multiple bindings
	WGPUBindGroupDescriptor bindGroupDesc = {
		.nextInChain = NULL,
		.layout = bindGroupLayout,
		// There must be as many bindings as declared in the layout!
		.entryCount = bindGroupLayoutDesc.entryCount,
		.entries = &binding
	};
	WGPUBindGroup bindGroup = wgpuDeviceCreateBindGroup(device, &bindGroupDesc);

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();
		uniforms.time = glfwGetTime();
		// wgpuQueueWriteBuffer(queue, uniformBuffer, 0, &uniforms, sizeof(MyUniforms));
		// wgpuQueueWriteBuffer(queue, uniformBuffer, offsetof(MyUniforms, time), &uniforms.time, sizeof(uniforms.time));
		// wgpuQueueWriteBuffer(queue, uniformBuffer, offsetof(MyUniforms, color), &uniforms.color, sizeof(uniforms.color));

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

		// Set both vertex and index buffers
		wgpuRenderPassEncoderSetVertexBuffer(renderPass, 0, vertexBuffer, 0, pointDataSize);

		// The second argument must correspond to the choice of uint16_t or uint32_t
		// we've done when creating the index buffer.
		wgpuRenderPassEncoderSetIndexBuffer(renderPass, indexBuffer, WGPUIndexFormat_Uint16, 0, indexDataSize);

		// Set binding group
		wgpuRenderPassEncoderSetBindGroup(renderPass, 0, bindGroup, 0, NULL);
		// Replace `draw()` with `drawIndexed()` and `vertexCount` with `indexCount`
		// The extra argument is an offset within the index buffer.
		wgpuRenderPassEncoderDrawIndexed(renderPass, indexCount, 1, 0, 0, 0);

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