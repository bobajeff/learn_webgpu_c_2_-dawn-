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

//--------------------------------------------main
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

    //------------------ADAPTER
    printf("Requesting adapter...\n");

    WGPUSurface surface = glfwGetWGPUSurface(instance, window);

    WGPURequestAdapterOptions adapterOpts = {};
    adapterOpts.nextInChain = NULL;
    adapterOpts.compatibleSurface = surface;

    WGPUAdapter adapter = requestAdapter(instance, &adapterOpts);

    printf( "Got adapter: %p\n", adapter);

    //------------------DEVICE
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
    
    //------------------COMMAND BUFFER
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, NULL /* pUserData */);

    WGPUQueue queue = wgpuDeviceGetQueue(device);

    WGPUSwapChainDescriptor swapChainDesc = {};
    swapChainDesc.width = 640;
    swapChainDesc.height = 480;

    swapChainDesc.usage = WGPUTextureUsage_RenderAttachment;

    WGPUTextureFormat swapChainFormat = WGPUTextureFormat_BGRA8Unorm;
    swapChainDesc.format = swapChainFormat;

    swapChainDesc.presentMode = WGPUPresentMode_Fifo;

    WGPUSwapChain swapChain = wgpuDeviceCreateSwapChain(device, surface, &swapChainDesc);
    printf( "Swapchain: %p\n", swapChain);

    const char* shaderSource = "                                                                \
    @vertex                                                                                     \
    fn vs_main(@builtin(vertex_index) in_vertex_index: u32) -> @builtin(position) vec4<f32> {   \
        var p = vec2<f32>(0.0, 0.0);                                                            \
        if (in_vertex_index == 0u) {                                                            \
            p = vec2<f32>(-0.5, -0.5);                                                          \
        } else if (in_vertex_index == 1u) {                                                     \
            p = vec2<f32>(0.5, -0.5);                                                           \
        } else {                                                                                \
            p = vec2<f32>(0.0, 0.5);                                                            \
        }                                                                                       \
        return vec4<f32>(p, 0.0, 1.0);                                                          \
    }                                                                                           \
                                                                                                \
    @fragment                                                                                   \
    fn fs_main() -> @location(0) vec4<f32> {                                                    \
        return vec4<f32>(0.0, 0.4, 1.0, 1.0);                                                   \
    }                                                                                           \
    ";

    WGPUShaderModuleDescriptor shaderDesc = {};
    // shaderDesc.hintCount = 0;
    // shaderDesc.hints = NULL;

    WGPUShaderModuleWGSLDescriptor shaderCodeDesc = {};
    // Set the chained struct's header
    shaderCodeDesc.chain.next = NULL;
    shaderCodeDesc.chain.sType = WGPUSType_ShaderModuleWGSLDescriptor;
    // Connect the chain
    shaderDesc.nextInChain = &shaderCodeDesc.chain;

    shaderCodeDesc.source = shaderSource;

    WGPUShaderModule shaderModule = wgpuDeviceCreateShaderModule(device, &shaderDesc);

    WGPURenderPipelineDescriptor pipelineDesc = {};
    // pipelineDesc.nextInChain = NULL;
    
    pipelineDesc.vertex.bufferCount = 0;
    pipelineDesc.vertex.buffers = NULL;

    pipelineDesc.vertex.module = shaderModule;
    pipelineDesc.vertex.entryPoint = "vs_main";
    pipelineDesc.vertex.constantCount = 0;
    pipelineDesc.vertex.constants = NULL;

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

    WGPUFragmentState fragmentState = {};
    fragmentState.nextInChain = NULL;
    pipelineDesc.fragment = &fragmentState;
    fragmentState.module = shaderModule;
    fragmentState.entryPoint = "fs_main";
    fragmentState.constantCount = 0;
    fragmentState.constants = NULL;

    WGPUBlendState blendState = {};
    // [...] Configure blend state
    blendState.color.srcFactor = WGPUBlendFactor_SrcAlpha;
    blendState.color.dstFactor = WGPUBlendFactor_OneMinusSrcAlpha;
    blendState.color.operation = WGPUBlendOperation_Add;

    blendState.alpha.srcFactor = WGPUBlendFactor_Zero;
    blendState.alpha.dstFactor = WGPUBlendFactor_One;
    blendState.alpha.operation = WGPUBlendOperation_Add;

    WGPUColorTargetState colorTarget = {};
    colorTarget.format = swapChainFormat;
    colorTarget.blend = &blendState;
    colorTarget.writeMask = WGPUColorWriteMask_All; // We could write to only some of the color channels.

    // We have only one target because our render pass has only one output color
    // attachment.
    fragmentState.targetCount = 1;
    fragmentState.targets = &colorTarget;

    pipelineDesc.depthStencil = NULL;

    // Samples per pixel
    pipelineDesc.multisample.count = 1;
    // Default value for the mask, meaning "all bits on"
    pipelineDesc.multisample.mask = ~0u;
    // Default value as well (irrelevant for count = 1 anyways)
    pipelineDesc.multisample.alphaToCoverageEnabled = false;


    WGPUPipelineLayoutDescriptor layoutDesc = {};
    layoutDesc.nextInChain = NULL;
    layoutDesc.bindGroupLayoutCount = 0;
    layoutDesc.bindGroupLayouts = NULL;
    WGPUPipelineLayout layout = wgpuDeviceCreatePipelineLayout(device, &layoutDesc);

    pipelineDesc.layout = layout;
    WGPURenderPipeline pipeline = wgpuDeviceCreateRenderPipeline(device, &pipelineDesc);
    while (!glfwWindowShouldClose(window)) {
        // In the main loop
        glfwPollEvents();
        WGPUTextureView nextTexture = wgpuSwapChainGetCurrentTextureView(swapChain);

        if (!nextTexture) {
            fprintf(stderr, "Cannot acquire next swap chain texture\n");
            return 1;
        }

        printf( "nextTexture: %p\n", nextTexture);

        WGPUCommandEncoderDescriptor encoderDesc = {};
        encoderDesc.nextInChain = NULL;
        encoderDesc.label = "My command encoder";
        WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

        WGPURenderPassDescriptor renderPassDesc = {};
        renderPassDesc.nextInChain = NULL;
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
        
        // Select which render pipeline to use
        wgpuRenderPassEncoderSetPipeline(renderPass, pipeline);
        // Draw 1 instance of a 3-vertices shape
        wgpuRenderPassEncoderDraw(renderPass, 3, 1, 0, 0);

        wgpuRenderPassEncoderEnd(renderPass);

        // wgpuTextureViewDrop(nextTexture);

        WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
        cmdBufferDescriptor.nextInChain = NULL;
        cmdBufferDescriptor.label = "Command buffer";
        WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
        // then submit queue
        wgpuQueueSubmit(queue, 1, &command);

        wgpuSwapChainPresent(swapChain);
    }

    glfwDestroyWindow(window);
    glfwTerminate();
}