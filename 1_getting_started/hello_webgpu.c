#include <GLFW/glfw3.h>
#include <webgpu/webgpu.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    // We create the equivalent of the navigator.gpu if this were web code

    // 1. We create a descriptor
    WGPUInstanceDescriptor desc = {};
    desc.nextInChain = NULL;

    // 2. We create the instance using this descriptor
    WGPUInstance instance = wgpuCreateInstance(&desc);

    // 3. We can check whether there is actually an instance created
    if (!instance) {
        fprintf(stderr, "Could not initialize WebGPU!\n");
        return 1;
    }

    // 4. Display the object (WGPUInstance is a simple pointer, it may be
    // copied around without worrying about its size).
        printf( "WGPU instance: %p\n", instance);
}