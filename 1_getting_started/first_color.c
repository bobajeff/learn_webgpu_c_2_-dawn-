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
        renderPassDesc.nextInChain = NULL;
        
        WGPURenderPassEncoder renderPass = wgpuCommandEncoderBeginRenderPass(encoder, &renderPassDesc);
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