#include <webgpu/webgpu.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <float.h>
#include <limits.h>
#include "helper_v2.h"

//  ------------------------------- Adapter------------------------------------------------------------------

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

void setDefault(WGPULimits *limits) {
    *limits = (WGPULimits){
        .maxTextureDimension1D = 0,
        .maxTextureDimension2D = 0,
        .maxTextureDimension3D = 0,
        .maxTextureArrayLayers = 0,
        .maxBindGroups = 0,
        .maxBindingsPerBindGroup = 0,
        .maxDynamicUniformBuffersPerPipelineLayout = 0,
        .maxDynamicStorageBuffersPerPipelineLayout = 0,
        .maxSampledTexturesPerShaderStage = 0,
        .maxSamplersPerShaderStage = 0,
        .maxStorageBuffersPerShaderStage = 0,
        .maxStorageTexturesPerShaderStage = 0,
        .maxUniformBuffersPerShaderStage = 0,
        .maxUniformBufferBindingSize = 0,
        .maxStorageBufferBindingSize = 0,
        .minUniformBufferOffsetAlignment = 0,
        .minStorageBufferOffsetAlignment = 0,
        .maxVertexBuffers = 0,
        .maxBufferSize = 0,
        .maxVertexAttributes = 0,
        .maxVertexBufferArrayStride = 0,
        .maxInterStageShaderComponents = 0,
        .maxInterStageShaderVariables = 0,
        .maxColorAttachments = 0,
        .maxColorAttachmentBytesPerSample = 0,
        .maxComputeWorkgroupStorageSize = 0,
        .maxComputeInvocationsPerWorkgroup = 0,
        .maxComputeWorkgroupSizeX = 0,
        .maxComputeWorkgroupSizeY = 0,
        .maxComputeWorkgroupSizeZ = 0,
        .maxComputeWorkgroupsPerDimension = 0
    };

}

void cCallback(WGPUErrorType type, char const* message, void* userdata) {
	printf( "Device error: type  (%u)\n", type);
    if (message)
    printf( "message: (%s)\n", message);
};


void onDeviceLost(WGPUDeviceLostReason reason, char const* message, void* userdata) {
	printf( "Device lost: reason  (%u)\n", reason);
    if (message)
    printf( "message: (%s)\n", message);
};

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



