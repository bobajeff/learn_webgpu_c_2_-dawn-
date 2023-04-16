#include <GLFW/glfw3.h>
#include <cassert>
#include <glfw3webgpu.h>
#include <iostream>
#include <vector>
#include <webgpu/webgpu.h>

/**
 * Utility function to get a WebGPU device, so that
 *     WGPUAdapter device = requestDevice(adapter, options);
 * is roughly equivalent to
 *     const device = await adapter.requestDevice(descriptor);
 * It is very similar to requestAdapter
 */
WGPUDevice requestDevice(WGPUAdapter adapter,
                         WGPUDeviceDescriptor const *descriptor) {
  struct UserData {
    WGPUDevice device = nullptr;
    bool requestEnded = false;
  };
  UserData userData;

  auto onDeviceRequestEnded = [](WGPURequestDeviceStatus status,
                                 WGPUDevice device, char const *message,
                                 void *pUserData) {
    UserData &userData = *reinterpret_cast<UserData *>(pUserData);
    if (status == WGPURequestDeviceStatus_Success) {
      userData.device = device;
    } else {
      std::cout << "Could not get WebGPU adapter: " << message << std::endl;
    }
    userData.requestEnded = true;
  };

  wgpuAdapterRequestDevice(adapter, descriptor, onDeviceRequestEnded,
                           (void *)&userData);

  assert(userData.requestEnded);

  return userData.device;
}

/**
 * Utility function to get a WebGPU adapter, so that
 *     WGPUAdapter adapter = requestAdapter(options);
 * is roughly equivalent to
 *     const adapter = await navigator.gpu.requestAdapter(options);
 */
WGPUAdapter requestAdapter(WGPUInstance instance,
                           WGPURequestAdapterOptions const *options) {
  // A simple structure holding the local information shared with the
  // onAdapterRequestEnded callback.
  struct UserData {
    WGPUAdapter adapter = nullptr;
    bool requestEnded = false;
  };
  UserData userData;

  // Callback called by wgpuInstanceRequestAdapter when the request returns
  // This is a C++ lambda function, but could be any function defined in the
  // global scope. It must be non-capturing (the brackets [] are empty) so
  // that it behaves like a regular C function pointer, which is what
  // wgpuInstanceRequestAdapter expects (WebGPU being a C API). The workaround
  // is to convey what we want to capture through the pUserData pointer,
  // provided as the last argument of wgpuInstanceRequestAdapter and received
  // by the callback as its last argument.
  auto onAdapterRequestEnded = [](WGPURequestAdapterStatus status,
                                  WGPUAdapter adapter, char const *message,
                                  void *pUserData) {
    UserData &userData = *reinterpret_cast<UserData *>(pUserData);
    if (status == WGPURequestAdapterStatus_Success) {
      userData.adapter = adapter;
    } else {
      std::cout << "Could not get WebGPU adapter: " << message << std::endl;
    }
    userData.requestEnded = true;
  };

  // Call to the WebGPU request adapter procedure
  wgpuInstanceRequestAdapter(instance /* equivalent of navigator.gpu */,
                             options, onAdapterRequestEnded, (void *)&userData);

  // In theory we should wait until onAdapterReady has been called, which
  // could take some time (what the 'await' keyword does in the JavaScript
  // code). In practice, we know that when the wgpuInstanceRequestAdapter()
  // function returns its callback has been called.
  assert(userData.requestEnded);

  return userData.adapter;
}

int main (int, char**) {
	WGPUInstanceDescriptor instanceDesc{};
	instanceDesc.nextInChain = nullptr;
	WGPUInstance instance = wgpuCreateInstance(&instanceDesc);
	if (!instance) {
		std::cerr << "Could not initialize WebGPU!" << std::endl;
		return 1;
	}

	if (!glfwInit()) {
		std::cerr << "Could not initialize GLFW!" << std::endl;
		return 1;
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	GLFWwindow* window = glfwCreateWindow(640, 480, "Learn WebGPU", NULL, NULL);
	if (!window) {
		std::cerr << "Could not open window!" << std::endl;
		return 1;
	}

	std::cout << "Requesting adapter..." << std::endl;
	WGPUSurface surface = glfwGetWGPUSurface(instance, window);
	WGPURequestAdapterOptions adapterOpts{};
	adapterOpts.nextInChain = nullptr;
	adapterOpts.compatibleSurface = surface;
	WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);
	std::cout << "Got adapter: " << adapter << std::endl;

	std::cout << "Requesting device..." << std::endl;
	WGPUDeviceDescriptor deviceDesc{};
	deviceDesc.nextInChain = nullptr;
	deviceDesc.label = "My Device";
	deviceDesc.requiredFeaturesCount = 0;
	deviceDesc.requiredLimits = nullptr;
	deviceDesc.defaultQueue.label = "The default queue";
	WGPUDevice device = requestDevice(adapter, &deviceDesc);
	std::cout << "Got device: " << device << std::endl;

	// Add an error callback for more debug info
	static auto cCallback = [](WGPUErrorType type, char const* message, void*) -> void {
		std::cout << "Device error: type " << type;
		if (message) std::cout << " (message: " << message << ")";
		std::cout << std::endl;
	};
	wgpuDeviceSetUncapturedErrorCallback(device, cCallback, nullptr);

	WGPUQueue queue = wgpuDeviceGetQueue(device);

	std::cout << "Creating swapchain..." << std::endl;
	WGPUTextureFormat swapChainFormat = WGPUTextureFormat_BGRA8Unorm;
	WGPUSwapChainDescriptor swapChainDesc = {};
	swapChainDesc.nextInChain = nullptr;
	swapChainDesc.width = 640;
	swapChainDesc.height = 480;
	swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;
	swapChainDesc.format = swapChainFormat;
	swapChainDesc.presentMode = WGPUPresentMode_Fifo;
	WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDesc);
	std::cout << "Swapchain: " << swapChain << std::endl;

	std::cout << "Creating shader module..." << std::endl;
	const char* shaderSource = R"(
@vertex
fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {
	var p = vec2<f32>(0.0, 0.0);
	if (in_vertex_index == 0u) {
		p = vec2<f32>(-0.5, -0.5);
	} else if (in_vertex_index == 1u) {
		p = vec2<f32>(0.5, -0.5);
	} else {
		p = vec2<f32>(0.0, 0.5);
	}
	return vec4<f32>(p, 0.0, 1.0);
}
@fragment
fn fs_main() -> @location(0) vec4<f32> {
    return vec4<f32>(0.0, 0.4, 1.0, 1.0);
}
)";

	WGPUShaderModuleDescriptor shaderDesc{};
	// shaderDesc.hintCount = 0;
	// shaderDesc.hints = nullptr;

	// Use the extension mechanism to load a WGSL shader source code
	WGPUShaderModuleWGSLDescriptor shaderCodeDesc{};
	// Set the chained struct's header
	shaderCodeDesc.chain.next = nullptr;
	shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
	// Connect the chain
	shaderDesc.nextInChain = &shaderCodeDesc.chain;

	// Setup the actual payload of the shader code descriptor
	shaderCodeDesc.source = shaderSource;

	WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
	std::cout << "Shader module: " << shaderModule << std::endl;

	std::cout << "Creating render pipeline..." << std::endl;
	WGPURenderPipelineDescriptor pipelineDesc{};

	// Vertex fetch
	// (We don't use any input buffer so far)
	pipelineDesc.vertex.bufferCount = 0;
	pipelineDesc.vertex.buffers = nullptr;

	// Vertex shader
	pipelineDesc.vertex.module = shaderModule;
	pipelineDesc.vertex.entryPoint = "vs_main";
	pipelineDesc.vertex.constantCount = 0;
	pipelineDesc.vertex.constants = nullptr;

	// Primitive assembly and rasterization
	// Each sequence of 3 vertices is considered as a triangle
	pipelineDesc.primitive.topology = WGPUPrimitiveTopology_TriangleList;
	// We'll see later how to specify the order in which vertices should be
	// connected. When not specified, vertices are considered sequentially.
	pipelineDesc.primitive.stripIndexFormat = WGPUIndexFormat_Undefined;
	// The face orientation is defined by assuming that when looking
	// from the front of the face, its corner vertices are enumerated
	// in the counter-clockwise (CCW) order.
	pipelineDesc.primitive.frontFace = WGPUFrontFace_CCW;
	// But the face orientation does not matter much because we do not
	// cull (i.e. "hide") the faces pointing away from us (which is often
	// used for optimization).
	pipelineDesc.primitive.cullMode = WGPUCullMode_None;

	// Fragment shader
	WGPUFragmentState fragmentState{};
	fragmentState.nextInChain = nullptr;
	pipelineDesc.fragment = &fragmentState;
	fragmentState.module = shaderModule;
	fragmentState.entryPoint = "fs_main";
	fragmentState.constantCount = 0;
	fragmentState.constants = nullptr;

	// Configure blend state
	WGPUBlendState blendState{};
	// Usual alpha blending for the color:
	blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
	blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
	blendState.color.operation = WGPUBlendOperation_Add;
	// We leave the target alpha untouched:
	blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
	blendState.alpha.dstFactor = WGPUBlendFactor_One;
	blendState.alpha.operation = WGPUBlendOperation_Add;

	WGPUColorTargetState colorTarget{};
	colorTarget.nextInChain = nullptr;
	colorTarget.format = swapChainFormat;
	colorTarget.blend = &blendState;
	colorTarget.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.

	// We have only one target because our render pass has only one output color
	// attachment.
	fragmentState.targetCount = 1;
	fragmentState.targets = &colorTarget;
	
	// Depth and stencil tests are not used here
	pipelineDesc.depthStencil = nullptr;

	// Multi-sampling
	// Samples per pixel
	pipelineDesc.multisample.count = 1;
	// Default value for the mask, meaning "all bits on"
	pipelineDesc.multisample.mask = ~0u;
	// Default value as well (irrelevant for count = 1 anyways)
	pipelineDesc.multisample.alphaToCoverageEnabled = false;

	// Pipeline layout
	// (Our example does not use any resource)
	WGPUPipelineLayoutDescriptor layoutDesc{};
	layoutDesc.nextInChain = nullptr;
	layoutDesc.bindGroupLayoutCount = 0;
	layoutDesc.bindGroupLayouts = nullptr;
	WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);
	pipelineDesc.layout = layout;

	WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
	std::cout << "Render pipeline: " << pipeline << std::endl;

	while (!glfwWindowShouldClose(window)) {
		glfwPollEvents();

		WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(swapChain);
		if (!nextTexture) {
			std::cerr << "Cannot acquire next swap chain texture" << std::endl;
			return 1;
		}

		WGPUCommandEncoderDescriptor commandEncoderDesc{};
		commandEncoderDesc.nextInChain = nullptr;
		commandEncoderDesc.label = "Command Encoder";
		WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &commandEncoderDesc);
		
		WGPURenderPassDescriptor renderPassDesc{};
		renderPassDesc.nextInChain = nullptr;

		WGPURenderPassColorAttachment renderPassColorAttachment = {};
		renderPassColorAttachment.view = nextTexture;
		renderPassColorAttachment.resolveTarget = nullptr;
		renderPassColorAttachment.loadOp = WGPULoadOp_Clear;
		renderPassColorAttachment.storeOp = WGPUStoreOp_Store;
		renderPassColorAttachment.clearValue = WGPUColor{ 0.9, 0.1, 0.2, 1.0 };
		renderPassDesc.colorAttachmentCount = 1;
		renderPassDesc.colorAttachments = &renderPassColorAttachment;

		renderPassDesc.depthStencilAttachment = nullptr;
		renderPassDesc.timestampWriteCount = 0;
		renderPassDesc.timestampWrites = nullptr;
		WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);

		// In its overall outline, drawing a triangle is as simple as this:
		// Select which render pipeline to use
		wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);
		// Draw 1 instance of a 3-vertices shape
		wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);

		wgpuRenderPassEncoderEnd(renderPass);
		
		// wgpuTextureViewDrop(nextTexture);

		WGPUCommandBufferDescriptor cmdBufferDescriptor{};
		cmdBufferDescriptor.nextInChain = nullptr;
		cmdBufferDescriptor.label = "Command buffer";
		WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
		wgpuQueueSubmit(queue, 1, &command);

		wgpuSwapChainPresent(swapChain);
	}

	glfwDestroyWindow(window);
	glfwTerminate();

	return 0;
}