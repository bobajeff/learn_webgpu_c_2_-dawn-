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
#include <errno.h>
#include <float.h>
#include <limits.h>
#include <string.h>

WGPUShaderModule loadShaderModule(const char * path, WGPUDevice device) {
    FILE *f = fopen(path, "rt");
    assert(f);
    fseek(f, 0, SEEK_END);
    long length = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = (char *) malloc(length + 1);
    buffer[length] = '\0';
    fread(buffer, 1, length, f);
    fclose(f);

	WGPUShaderModuleWGSLDescriptor shaderCodeDesc = {
		.chain = (WGPUChainedStruct){
			.next = NULL,
			.sType = WGPUSType_ShaderModuleWGSLDescriptor
		},
		.source = buffer
	};
	WGPUShaderModuleDescriptor shaderDesc = {
		.nextInChain = &shaderCodeDesc.chain
	};
	WGPUShaderModule shadermodule = wgpuDeviceCreateShaderModule(device, &shaderDesc);
    free(buffer);
	return shadermodule;
}

typedef struct GeometryData {
    float *pointData;
    size_t pointDataSize;
    uint16_t * indexData;
    size_t indexDataSize;
} t_geometry_data;


bool loadGeometry(const char * path, t_geometry_data * geometry_data) {
    int pointcount = 0;
    int indexcount = 0;
    FILE *f = fopen(path, "rt");
	if (!f){
		printf("can't open file:\n %s\n", path);
		return false;
	}

	enum Section {
		None,
		Points,
		Indices,
	};
	enum Section currentSection = None;

	float value;
	uint16_t index;
    size_t line_buf_size = 0;
    char *line;

    ssize_t line_size = getline(&line, &line_buf_size, f);

	while (line_size >= 0) {
		if (strcmp(line, "[points]\n") == 0) {
			currentSection = Points;
		}
		else if (strcmp(line, "[indices]\n") == 0) {
			currentSection = Indices;
		}
		else if (line[0] == '#' || line[0] == '\n') {
		}
		else if (currentSection == Points) {
            char *start = line;
            char *eon;
            float value;
            errno = 0;
            
            float * tmp;
            // extract floating next point number after the current 'start' place in the the line
            while ((value = strtof(start, &eon)),
                eon != start &&
                !((errno == EINVAL && value == 0) ||
                    (errno == ERANGE && (value == FLT_MIN || value == FLT_MAX))))
            {
                tmp = realloc(geometry_data->pointData, geometry_data->pointDataSize + (sizeof(float)));
                if (!tmp) {
                    printf("Memory Re-allocation failed.\n");
					return false;
                } else {
                    geometry_data->pointData = tmp;
                    geometry_data->pointData[pointcount] = value;
                    pointcount++;
                    geometry_data->pointDataSize += (sizeof(float));

                    start = eon;
                    errno = 0;
                }
            }
		}
		else if (currentSection == Indices) {
            char *start = line;
            char *eon;
            long value;
            errno = 0;
            
            uint16_t * tmp;
            // extract next long integer after the current 'start' point in the the line.
            while ((value = strtol(start, &eon, 0)),
                eon != start &&
                !((errno == EINVAL && value == 0) ||
                    (errno == ERANGE && (value == LONG_MIN || value == LONG_MAX))))
            {
                tmp = realloc(geometry_data->indexData, geometry_data->pointDataSize + sizeof(uint16_t));
                if (!tmp) {
                    printf("Memory Re-allocation failed.");
					return false;
                } else {
                    geometry_data->indexData = tmp;
                }
                geometry_data->indexData[indexcount] = value;
                indexcount++;
                geometry_data->indexDataSize += (sizeof(uint16_t));

                start = eon;
                errno = 0;
            }
		}
        line_size = getline(&line, &line_buf_size, f);
	}

    free(line);
    fclose(f);
	return true;
}

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
    requiredLimits.limits.maxVertexBufferArrayStride = 20;
    requiredLimits.limits.maxBufferSize = 300;
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

	WGPUShaderModule shaderModule = loadShaderModule(RESOURCE_DIR "/shader.wsl", device);
	printf( "Shader module: %p\n", shaderModule);

	printf( "Creating render pipeline...\n");
	

	// Vertex fetch
	// We now have 2 attributes
	WGPUVertexAttribute vertexAttribs[2];

	// Position attribute
	vertexAttribs[0] = (WGPUVertexAttribute){
		.shaderLocation = 0,
		.format = WGPUVertexFormat_Float32x2,
		.offset = 0
	};

	// Color attribute
	vertexAttribs[1] = (WGPUVertexAttribute){
		.shaderLocation = 1,
		.format = WGPUVertexFormat_Float32x3,
		.offset = 2 * sizeof(float)
	};

	WGPUVertexBufferLayout vertexBufferLayout = {
		.attributeCount = 2,
		.attributes = vertexAttribs,
		. arrayStride = 5 * sizeof(float),
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
	bool success = loadGeometry(RESOURCE_DIR "/webgpu.txt", &geometrydata);
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

		// Replace `draw()` with `drawIndexed()` and `vertexCount` with `indexCount`
		// The extra argument is an offset within the index buffer.
		wgpuRenderPassEncoderDrawIndexed(renderPass, indexCount, 1, 0, 0, 0);

		wgpuRenderPassEncoderEnd(renderPass);
		
		// wgpuTextureViewDrop(nextTexture);

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