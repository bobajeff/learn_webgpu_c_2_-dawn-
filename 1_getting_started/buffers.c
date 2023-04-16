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

struct Context {
    WGPUBuffer buffer;
};

void onBuffer2Mapped(WGPUBufferMapAsyncStatus status, void* pUserData) {
    struct Context* context = (struct Context *)(pUserData);
    printf( "Buffer 2 mapped with status %u\n", status);
    if (status != WGPUBufferMapAsyncStatus_Success) return;

    // Get a pointer to wherever the driver mapped the GPU memory to the RAM
    unsigned char* bufferData = (unsigned char*)wgpuBufferGetMappedRange(context->buffer, 0, 16);

    // [...] (Do stuff with bufferData)
    printf( "bufferData = [");
    for (unsigned char i = 0; i < 16; i++) {
        if (i > 0) 
        {
            printf( ",");
        }
         printf( "%hhu", bufferData[i]);    //probably what the c++ version meant to do 😊
    }
    printf(  "]\n");

    // Then do not forget to unmap the memory
    wgpuBufferUnmap(context->buffer);
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
    
    wgpuDeviceSetUncapturedErrorCallback(device, onDeviceError, NULL /* pUserData */);

    //------------------BUFFER

    WGPUBufferDescriptor bufferDesc1 = {};
    bufferDesc1.nextInChain = NULL;
    bufferDesc1.label = "Some GPU-side data buffer";
    bufferDesc1.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_CopySrc;
    bufferDesc1.size = 16;
    bufferDesc1.mappedAtCreation = false;
    WGPUBuffer buffer1 = wgpuDeviceCreateBuffer(device, &bufferDesc1);

    WGPUBufferDescriptor bufferDesc2 = {};
    bufferDesc2.nextInChain = NULL;
    bufferDesc2.label = "Some GPU-side data buffer";
    bufferDesc2.usage = WGPUBufferUsage_CopyDst | WGPUBufferUsage_MapRead;
    bufferDesc2.size = 16;
    bufferDesc2.mappedAtCreation = false;
    WGPUBuffer buffer2 = wgpuDeviceCreateBuffer(device, &bufferDesc2);


    //------------------COMMAND QUEUE
    WGPUQueue queue = wgpuDeviceGetQueue(device);

    // Create some CPU-side data buffer (of size 16 bytes)
    unsigned char numbers[16];
    for (unsigned char i = 0; i < 16; i++) numbers[i] = i;

    // Copy this from `numbers` (RAM) to `buffer1` (VRAM)
    wgpuQueueWriteBuffer(queue, buffer1, 0, numbers, 16);

    WGPUCommandEncoderDescriptor encoderDesc = {};
    encoderDesc.nextInChain = NULL;
    encoderDesc.label = "My command encoder";
    WGPUCommandEncoder encoder = wgpuDeviceCreateCommandEncoder(device, &encoderDesc);

    wgpuCommandEncoderCopyBufferToBuffer(encoder, buffer1, 0, buffer2, 0, 16);

    WGPUCommandBufferDescriptor cmdBufferDescriptor = {};
    cmdBufferDescriptor.nextInChain = NULL;
    cmdBufferDescriptor.label = "Command buffer";
    WGPUCommandBuffer command = wgpuCommandEncoderFinish(encoder, &cmdBufferDescriptor);
    // then submit queue
    wgpuQueueSubmit(queue, 1, &command);


    struct Context context = { buffer2 };
    wgpuBufferMapAsync(buffer2, WGPUMapMode_Read, 0, 16, onBuffer2Mapped, (void*)&context);

    while (!glfwWindowShouldClose(window)) {
        wgpuQueueSubmit(queue, 0, NULL);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    wgpuBufferDestroy(buffer1);
    wgpuBufferDestroy(buffer2);

}