#include <iostream>
#include <fstream>
#include <sstream>
#include <time.h>

#include <stdlib.h>
#include <tchar.h>
#include <memory.h>
#include <vector>

#include <CL\cl.h>

#include "raytracing.h"
#include "define.h"

#pragma warning( push )
#pragma warning( disable : 4996 )

using namespace RAYTRACING;

// Macros for OpenCL versions
#define OPENCL_VERSION_1_2  1.2f
#define OPENCL_VERSION_2_0  2.0f

// TODO: these should probably be read in as commandline parameters.
const size_t kWidth = WIDTH_SIZE;
const size_t kHeight = HEIGHT_SIZE;
const size_t kNumPixelSamples = NUM_SAMPLE;
const size_t workAmount = WORK_AMOUNT;

// Upload the OpenCL C source code to output argument source
// The memory resource is implicitly allocated in the function
// and should be deallocated by the caller
int ReadSourceFromFile(const char* fileName, char** source, size_t* sourceSize)
{
	int errorCode = CL_SUCCESS;

	FILE* fp = NULL;
	fopen_s(&fp, fileName, "rb");
	if (fp == NULL)
	{
		printf("Error: Couldn't find program source file '%s'.\n", fileName);
		errorCode = CL_INVALID_VALUE;
	}
	else {
		fseek(fp, 0, SEEK_END);
		*sourceSize = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		*source = new char[*sourceSize];
		if (*source == NULL)
		{
			printf("Error: Couldn't allocate %d bytes for program source from file '%s'.\n", *sourceSize, fileName);
			errorCode = CL_OUT_OF_HOST_MEMORY;
		}
		else {
			fread(*source, 1, *sourceSize, fp);
		}
	}
	return errorCode;
}

/* This function helps to create informative messages in
* case when OpenCL errors occur. It returns a string
* representation for an OpenCL error code.
* (E.g. "CL_DEVICE_NOT_FOUND" instead of just -1.)
*/
const char* TranslateOpenCLError(cl_int errorCode)
{
	switch (errorCode)
	{
	case CL_SUCCESS:                            return "CL_SUCCESS";
	case CL_DEVICE_NOT_FOUND:                   return "CL_DEVICE_NOT_FOUND";
	case CL_DEVICE_NOT_AVAILABLE:               return "CL_DEVICE_NOT_AVAILABLE";
	case CL_COMPILER_NOT_AVAILABLE:             return "CL_COMPILER_NOT_AVAILABLE";
	case CL_MEM_OBJECT_ALLOCATION_FAILURE:      return "CL_MEM_OBJECT_ALLOCATION_FAILURE";
	case CL_OUT_OF_RESOURCES:                   return "CL_OUT_OF_RESOURCES";
	case CL_OUT_OF_HOST_MEMORY:                 return "CL_OUT_OF_HOST_MEMORY";
	case CL_PROFILING_INFO_NOT_AVAILABLE:       return "CL_PROFILING_INFO_NOT_AVAILABLE";
	case CL_MEM_COPY_OVERLAP:                   return "CL_MEM_COPY_OVERLAP";
	case CL_IMAGE_FORMAT_MISMATCH:              return "CL_IMAGE_FORMAT_MISMATCH";
	case CL_IMAGE_FORMAT_NOT_SUPPORTED:         return "CL_IMAGE_FORMAT_NOT_SUPPORTED";
	case CL_BUILD_PROGRAM_FAILURE:              return "CL_BUILD_PROGRAM_FAILURE";
	case CL_MAP_FAILURE:                        return "CL_MAP_FAILURE";
	case CL_MISALIGNED_SUB_BUFFER_OFFSET:       return "CL_MISALIGNED_SUB_BUFFER_OFFSET";                          //-13
	case CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST:    return "CL_EXEC_STATUS_ERROR_FOR_EVENTS_IN_WAIT_LIST";   //-14
	case CL_COMPILE_PROGRAM_FAILURE:            return "CL_COMPILE_PROGRAM_FAILURE";                               //-15
	case CL_LINKER_NOT_AVAILABLE:               return "CL_LINKER_NOT_AVAILABLE";                                  //-16
	case CL_LINK_PROGRAM_FAILURE:               return "CL_LINK_PROGRAM_FAILURE";                                  //-17
	case CL_DEVICE_PARTITION_FAILED:            return "CL_DEVICE_PARTITION_FAILED";                               //-18
	case CL_KERNEL_ARG_INFO_NOT_AVAILABLE:      return "CL_KERNEL_ARG_INFO_NOT_AVAILABLE";                         //-19
	case CL_INVALID_VALUE:                      return "CL_INVALID_VALUE";
	case CL_INVALID_DEVICE_TYPE:                return "CL_INVALID_DEVICE_TYPE";
	case CL_INVALID_PLATFORM:                   return "CL_INVALID_PLATFORM";
	case CL_INVALID_DEVICE:                     return "CL_INVALID_DEVICE";
	case CL_INVALID_CONTEXT:                    return "CL_INVALID_CONTEXT";
	case CL_INVALID_QUEUE_PROPERTIES:           return "CL_INVALID_QUEUE_PROPERTIES";
	case CL_INVALID_COMMAND_QUEUE:              return "CL_INVALID_COMMAND_QUEUE";
	case CL_INVALID_HOST_PTR:                   return "CL_INVALID_HOST_PTR";
	case CL_INVALID_MEM_OBJECT:                 return "CL_INVALID_MEM_OBJECT";
	case CL_INVALID_IMAGE_FORMAT_DESCRIPTOR:    return "CL_INVALID_IMAGE_FORMAT_DESCRIPTOR";
	case CL_INVALID_IMAGE_SIZE:                 return "CL_INVALID_IMAGE_SIZE";
	case CL_INVALID_SAMPLER:                    return "CL_INVALID_SAMPLER";
	case CL_INVALID_BINARY:                     return "CL_INVALID_BINARY";
	case CL_INVALID_BUILD_OPTIONS:              return "CL_INVALID_BUILD_OPTIONS";
	case CL_INVALID_PROGRAM:                    return "CL_INVALID_PROGRAM";
	case CL_INVALID_PROGRAM_EXECUTABLE:         return "CL_INVALID_PROGRAM_EXECUTABLE";
	case CL_INVALID_KERNEL_NAME:                return "CL_INVALID_KERNEL_NAME";
	case CL_INVALID_KERNEL_DEFINITION:          return "CL_INVALID_KERNEL_DEFINITION";
	case CL_INVALID_KERNEL:                     return "CL_INVALID_KERNEL";
	case CL_INVALID_ARG_INDEX:                  return "CL_INVALID_ARG_INDEX";
	case CL_INVALID_ARG_VALUE:                  return "CL_INVALID_ARG_VALUE";
	case CL_INVALID_ARG_SIZE:                   return "CL_INVALID_ARG_SIZE";
	case CL_INVALID_KERNEL_ARGS:                return "CL_INVALID_KERNEL_ARGS";
	case CL_INVALID_WORK_DIMENSION:             return "CL_INVALID_WORK_DIMENSION";
	case CL_INVALID_WORK_GROUP_SIZE:            return "CL_INVALID_WORK_GROUP_SIZE";
	case CL_INVALID_WORK_ITEM_SIZE:             return "CL_INVALID_WORK_ITEM_SIZE";
	case CL_INVALID_GLOBAL_OFFSET:              return "CL_INVALID_GLOBAL_OFFSET";
	case CL_INVALID_EVENT_WAIT_LIST:            return "CL_INVALID_EVENT_WAIT_LIST";
	case CL_INVALID_EVENT:                      return "CL_INVALID_EVENT";
	case CL_INVALID_OPERATION:                  return "CL_INVALID_OPERATION";
	case CL_INVALID_GL_OBJECT:                  return "CL_INVALID_GL_OBJECT";
	case CL_INVALID_BUFFER_SIZE:                return "CL_INVALID_BUFFER_SIZE";
	case CL_INVALID_MIP_LEVEL:                  return "CL_INVALID_MIP_LEVEL";
	case CL_INVALID_GLOBAL_WORK_SIZE:           return "CL_INVALID_GLOBAL_WORK_SIZE";                           //-63
	case CL_INVALID_PROPERTY:                   return "CL_INVALID_PROPERTY";                                   //-64
	case CL_INVALID_IMAGE_DESCRIPTOR:           return "CL_INVALID_IMAGE_DESCRIPTOR";                           //-65
	case CL_INVALID_COMPILER_OPTIONS:           return "CL_INVALID_COMPILER_OPTIONS";                           //-66
	case CL_INVALID_LINKER_OPTIONS:             return "CL_INVALID_LINKER_OPTIONS";                             //-67
	case CL_INVALID_DEVICE_PARTITION_COUNT:     return "CL_INVALID_DEVICE_PARTITION_COUNT";                     //-68
		//    case CL_INVALID_PIPE_SIZE:                  return "CL_INVALID_PIPE_SIZE";                                  //-69
		//    case CL_INVALID_DEVICE_QUEUE:               return "CL_INVALID_DEVICE_QUEUE";                               //-70    

	default:
		return "UNKNOWN ERROR CODE";
	}
}

/* Convenient container for all OpenCL specific objects used in the sample
*
* It consists of two parts:
*   - regular OpenCL objects which are used in almost each normal OpenCL applications
*   - several OpenCL objects that are specific for this particular sample
*
* You collect all these objects in one structure for utility purposes
* only, there is no OpenCL specific here: just to avoid global variables
* and make passing all these arguments in functions easier.
*/
struct ocl_args_d_t
{
	ocl_args_d_t();
	~ocl_args_d_t();

	// Regular OpenCL objects:
	cl_context       context;           // hold the context handler
	cl_device_id     device;            // hold the selected device handler
	cl_command_queue commandQueue;      // hold the commands-queue handler
	cl_program       program;           // hold the program handler
	cl_kernel        kernel;            // hold the kernel handler
	float            platformVersion;   // hold the OpenCL platform version (default 1.2)
	float            deviceVersion;     // hold the OpenCL device version (default. 1.2)
	float            compilerVersion;   // hold the device OpenCL C version (default. 1.2)

	// Objects that are specific for algorithm implemented in this sample
	cl_mem           Lights;              // hold first source buffer
	cl_uint			 LightCount;
	cl_mem			 Shapes;
	cl_uint			 ShapeCount;
	cl_uint			 sampleCount;
	cl_uint			 width;
	cl_uint			 height;
	cl_mem			 cam;
	cl_mem           Pixels;            // hold destination buffer
	cl_mem			 Seeds;
};

ocl_args_d_t::ocl_args_d_t() :
		context(NULL),
		device(NULL),
		commandQueue(NULL),
		program(NULL),
		kernel(NULL),
		platformVersion(OPENCL_VERSION_1_2),
		deviceVersion(OPENCL_VERSION_1_2),
		compilerVersion(OPENCL_VERSION_1_2),
		Lights(NULL),
		LightCount(0),
		Shapes(NULL),
		ShapeCount(0),
		sampleCount(0),
		width(0),
		height(0),
		cam(NULL),
		Pixels(NULL),
		Seeds(NULL)
{
}

/*
* destructor - called only once
* Release all OpenCL objects
* This is a regular sequence of calls to deallocate all created OpenCL resources in bootstrapOpenCL.
*
* You may want to call these deallocation procedures in the middle of your application execution
* (not at the end) if you don't further need OpenCL runtime.
* You may want to do that in order to free some memory, for example,
* or recreate OpenCL objects with different parameters.
*
*/
ocl_args_d_t::~ocl_args_d_t()
{
	cl_int err = CL_SUCCESS;

	if (kernel)
	{
		err = clReleaseKernel(kernel);
		if (CL_SUCCESS != err)
		{
			printf("Error: clReleaseKernel returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (program)
	{
		err = clReleaseProgram(program);
		if (CL_SUCCESS != err)
		{
			printf("Error: clReleaseProgram returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (Lights)
	{
		err = clReleaseMemObject(Lights);
		if (CL_SUCCESS != err)
		{
			printf("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (Shapes)
	{
		err = clReleaseMemObject(Shapes);
		if (CL_SUCCESS != err)
		{
			printf("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (cam)
	{
		err = clReleaseMemObject(cam);
		if (CL_SUCCESS != err)
		{
			printf("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (Pixels)
	{
		err = clReleaseMemObject(Pixels);
		if (CL_SUCCESS != err)
		{
			printf("Error: clReleaseMemObject returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (commandQueue)
	{
		err = clReleaseCommandQueue(commandQueue);
		if (CL_SUCCESS != err)
		{
			printf("Error: clReleaseCommandQueue returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (device)
	{
		err = clReleaseDevice(device);
		if (CL_SUCCESS != err)
		{
			printf("Error: clReleaseDevice returned '%s'.\n", TranslateOpenCLError(err));
		}
	}
	if (context)
	{
		err = clReleaseContext(context);
		if (CL_SUCCESS != err)
		{
			printf("Error: clReleaseContext returned '%s'.\n", TranslateOpenCLError(err));
		}
	}

	/*
	* Note there is no procedure to deallocate platform
	* because it was not created at the startup,
	* but just queried from OpenCL runtime.
	*/
}

/*
* Check whether an OpenCL platform is the required platform
* (based on the platform's name)
*/
bool CheckPreferredPlatformMatch(cl_platform_id platform, const char* preferredPlatform)
{
	size_t stringLength = 0;
	cl_int err = CL_SUCCESS;
	bool match = false;

	// In order to read the platform's name, we first read the platform's name string length (param_value is NULL).
	// The value returned in stringLength
	err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, NULL, &stringLength);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetPlatformInfo() to get CL_PLATFORM_NAME length returned '%s'.\n", TranslateOpenCLError(err));
		return false;
	}

	// Now, that we know the platform's name string length, we can allocate enough space before read it
	std::vector<char> platformName(stringLength);

	// Read the platform's name string
	// The read value returned in platformName
	err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, stringLength, &platformName[0], NULL);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetplatform_ids() to get CL_PLATFORM_NAME returned %s.\n", TranslateOpenCLError(err));
		return false;
	}

	// Now check if the platform's name is the required one
	if (strstr(&platformName[0], preferredPlatform) != 0)
	{
		// The checked platform is the one we're looking for
		match = true;
	}

	return match;
}

/*
* Find and return the preferred OpenCL platform
* In case that preferredPlatform is NULL, the ID of the first discovered platform will be returned
*/
cl_platform_id FindOpenCLPlatform(const char* preferredPlatform, cl_device_type deviceType)
{
	cl_uint numPlatforms = 0;
	cl_int err = CL_SUCCESS;

	// Get (in numPlatforms) the number of OpenCL platforms available
	// No platform ID will be return, since platforms is NULL
	err = clGetPlatformIDs(0, NULL, &numPlatforms);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetplatform_ids() to get num platforms returned %s.\n", TranslateOpenCLError(err));
		return NULL;
	}

	printf("Number of available platforms: %u\n", numPlatforms);

	if (0 == numPlatforms)
	{
		printf("Error: No platforms found!\n");
		return NULL;
	}

	std::vector<cl_platform_id> platforms(numPlatforms);

	// Now, obtains a list of numPlatforms OpenCL platforms available
	// The list of platforms available will be returned in platforms
	err = clGetPlatformIDs(numPlatforms, &platforms[0], NULL);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetplatform_ids() to get platforms returned %s.\n", TranslateOpenCLError(err));
		return NULL;
	}

	// Check if one of the available platform matches the preferred requirements
	for (cl_uint i = 0; i < numPlatforms; i++)
	{
		bool match = true;
		cl_uint numDevices = 0;

		// If the preferredPlatform is not NULL then check if platforms[i] is the required one
		// Otherwise, continue the check with platforms[i]
		if ((NULL != preferredPlatform) && (strlen(preferredPlatform) > 0))
		{
			// In case we're looking for a specific platform
			match = CheckPreferredPlatformMatch(platforms[i], preferredPlatform);
		}

		// match is true if the platform's name is the required one or don't care (NULL)
		if (match)
		{
			// Obtains the number of deviceType devices available on platform
			// When the function failed we expect numDevices to be zero.
			// We ignore the function return value since a non-zero error code
			// could happen if this platform doesn't support the specified device type.
			err = clGetDeviceIDs(platforms[i], deviceType, 0, NULL, &numDevices);
			if (CL_SUCCESS != err)
			{
				printf("clGetDeviceIDs() returned %s.\n", TranslateOpenCLError(err));
			}

			if (0 != numDevices)
			{
				// There is at list one device that answer the requirements
				return platforms[i];
			}
		}
	}

	return NULL;
}

/*
* This function read the OpenCL platdorm and device versions
* (using clGetxxxInfo API) and stores it in the ocl structure.
* Later it will enable us to support both OpenCL 1.2 and 2.0 platforms and devices
* in the same program.
*/
int GetPlatformAndDeviceVersion(cl_platform_id platformId, ocl_args_d_t *ocl)
{
	cl_int err = CL_SUCCESS;

	// Read the platform's version string length (param_value is NULL).
	// The value returned in stringLength
	size_t stringLength = 0;
	err = clGetPlatformInfo(platformId, CL_PLATFORM_VERSION, 0, NULL, &stringLength);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetPlatformInfo() to get CL_PLATFORM_VERSION length returned '%s'.\n", TranslateOpenCLError(err));
		return err;
	}

	// Now, that we know the platform's version string length, we can allocate enough space before read it
	std::vector<char> platformVersion(stringLength);

	// Read the platform's version string
	// The read value returned in platformVersion
	err = clGetPlatformInfo(platformId, CL_PLATFORM_VERSION, stringLength, &platformVersion[0], NULL);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetplatform_ids() to get CL_PLATFORM_VERSION returned %s.\n", TranslateOpenCLError(err));
		return err;
	}

	if (strstr(&platformVersion[0], "OpenCL 2.0") != NULL)
	{
		ocl->platformVersion = OPENCL_VERSION_2_0;
	}

	// Read the device's version string length (param_value is NULL).
	err = clGetDeviceInfo(ocl->device, CL_DEVICE_VERSION, 0, NULL, &stringLength);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetDeviceInfo() to get CL_DEVICE_VERSION length returned '%s'.\n", TranslateOpenCLError(err));
		return err;
	}

	// Now, that we know the device's version string length, we can allocate enough space before read it
	std::vector<char> deviceVersion(stringLength);

	// Read the device's version string
	// The read value returned in deviceVersion
	err = clGetDeviceInfo(ocl->device, CL_DEVICE_VERSION, stringLength, &deviceVersion[0], NULL);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetDeviceInfo() to get CL_DEVICE_VERSION returned %s.\n", TranslateOpenCLError(err));
		return err;
	}

	if (strstr(&deviceVersion[0], "OpenCL 2.0") != NULL)
	{
		ocl->deviceVersion = OPENCL_VERSION_2_0;
	}

	// Read the device's OpenCL C version string length (param_value is NULL).
	err = clGetDeviceInfo(ocl->device, CL_DEVICE_OPENCL_C_VERSION, 0, NULL, &stringLength);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetDeviceInfo() to get CL_DEVICE_OPENCL_C_VERSION length returned '%s'.\n", TranslateOpenCLError(err));
		return err;
	}

	// Now, that we know the device's OpenCL C version string length, we can allocate enough space before read it
	std::vector<char> compilerVersion(stringLength);

	// Read the device's OpenCL C version string
	// The read value returned in compilerVersion
	err = clGetDeviceInfo(ocl->device, CL_DEVICE_OPENCL_C_VERSION, stringLength, &compilerVersion[0], NULL);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetDeviceInfo() to get CL_DEVICE_OPENCL_C_VERSION returned %s.\n", TranslateOpenCLError(err));
		return err;
	}

	else if (strstr(&compilerVersion[0], "OpenCL C 2.0") != NULL)
	{
		ocl->compilerVersion = OPENCL_VERSION_2_0;
	}

	return err;
}

int GetWorkGroupInfo(ocl_args_d_t *ocl, size_t globalSize, size_t* localSize)
{
	cl_int err = CL_SUCCESS;
	size_t tmpSize;
	err = clGetKernelWorkGroupInfo(ocl->kernel, ocl->device, CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE, sizeof(size_t), &tmpSize, NULL);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetKernelWorkGroupInfo() to get CL_DEVICE_OPENCL_C_VERSION returned %s.\n", TranslateOpenCLError(err));
		return err;
	}

	*localSize = globalSize / tmpSize;
	err = clGetKernelWorkGroupInfo(ocl->kernel, ocl->device, CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &tmpSize, NULL);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetKernelWorkGroupInfo() to get CL_DEVICE_OPENCL_C_VERSION returned %s.\n", TranslateOpenCLError(err));
		return err;
	}

	if (*localSize > tmpSize)
		*localSize = tmpSize;

	return err;
}

/*
* This function picks/creates necessary OpenCL objects which are needed.
* The objects are:
* OpenCL platform, device, context, and command queue.
*
* All these steps are needed to be performed once in a regular OpenCL application.
* This happens before actual compute kernels calls are performed.
*
* For convenience, in this application you store all those basic OpenCL objects in structure ocl_args_d_t,
* so this function populates fields of this structure, which is passed as parameter ocl.
* Please, consider reviewing the fields before going further.
* The structure definition is right in the beginning of this file.
*/
int SetupOpenCL(ocl_args_d_t *ocl, cl_device_type deviceType)
{
	// The following variable stores return codes for all OpenCL calls.
	cl_int err = CL_SUCCESS;

	// Query for all available OpenCL platforms on the system
	// Here you enumerate all platforms and pick one which name has preferredPlatform as a sub-string
	cl_platform_id platformId = FindOpenCLPlatform("Intel", deviceType);
	if (NULL == platformId)
	{
		printf("Error: Failed to find OpenCL platform.\n");
		return CL_INVALID_VALUE;
	}

	// Create context with device of specified type.
	// Required device type is passed as function argument deviceType.
	// So you may use this function to create context for any CPU or GPU OpenCL device.
	// The creation is synchronized (pfn_notify is NULL) and NULL user_data
	cl_context_properties contextProperties[] = { CL_CONTEXT_PLATFORM, (cl_context_properties)platformId, 0 };
	ocl->context = clCreateContextFromType(contextProperties, deviceType, NULL, NULL, &err);
	if ((CL_SUCCESS != err) || (NULL == ocl->context))
	{
		printf("Couldn't create a context, clCreateContextFromType() returned '%s'.\n", TranslateOpenCLError(err));
		return err;
	}

	// Query for OpenCL device which was used for context creation
	err = clGetContextInfo(ocl->context, CL_CONTEXT_DEVICES, sizeof(cl_device_id), &ocl->device, NULL);
	if (CL_SUCCESS != err)
	{
		printf("Error: clGetContextInfo() to get list of devices returned %s.\n", TranslateOpenCLError(err));
		return err;
	}

	// Read the OpenCL platform's version and the device OpenCL and OpenCL C versions
	GetPlatformAndDeviceVersion(platformId, ocl);

	// Create command queue.
	// OpenCL kernels are enqueued for execution to a particular device through special objects called command queues.
	// Command queue guarantees some ordering between calls and other OpenCL commands.
	// Here you create a simple in-order OpenCL command queue that doesn't allow execution of two kernels in parallel on a target device.
#ifdef CL_VERSION_2_0
	if (OPENCL_VERSION_2_0 == ocl->deviceVersion)
	{
		const cl_command_queue_properties properties[] = { CL_QUEUE_PROPERTIES, CL_QUEUE_PROFILING_ENABLE, 0 };
		ocl->commandQueue = clCreateCommandQueueWithProperties(ocl->context, ocl->device, properties, &err);
	}
	else {
		// default behavior: OpenCL 1.2
		cl_command_queue_properties properties = CL_QUEUE_PROFILING_ENABLE;
		ocl->commandQueue = clCreateCommandQueue(ocl->context, ocl->device, properties, &err);
	}
#else
	// default behavior: OpenCL 1.2
	cl_command_queue_properties properties = CL_QUEUE_PROFILING_ENABLE;
	ocl->commandQueue = clCreateCommandQueue(ocl->context, ocl->device, properties, &err);
#endif
	if (CL_SUCCESS != err)
	{
		printf("Error: clCreateCommandQueue() returned %s.\n", TranslateOpenCLError(err));
		return err;
	}

	return CL_SUCCESS;
}

/*
* Create and build OpenCL program from its source code
*/
int CreateAndBuildProgram(ocl_args_d_t *ocl)
{
	cl_int err = CL_SUCCESS;

	// Upload the OpenCL C source code from the input file to source
	// The size of the C program is returned in sourceSize
	char* source = NULL;
	size_t src_size = 0;
	err = ReadSourceFromFile("ray_algorithm.cl", &source, &src_size);
	if (CL_SUCCESS != err)
	{
		printf("Error: ReadSourceFromFile returned %s.\n", TranslateOpenCLError(err));
		goto Finish;
	}

	// And now after you obtained a regular C string call clCreateProgramWithSource to create OpenCL program object.
	ocl->program = clCreateProgramWithSource(ocl->context, 1, (const char**)&source, &src_size, &err);
	if (CL_SUCCESS != err)
	{
		printf("Error: clCreateProgramWithSource returned %s.\n", TranslateOpenCLError(err));
		goto Finish;
	}

	// Build the program
	// During creation a program is not built. You need to explicitly call build function.
	// Here you just use create-build sequence,
	// but there are also other possibilities when program consist of several parts,
	// some of which are libraries, and you may want to consider using clCompileProgram and clLinkProgram as
	// alternatives.
	err = clBuildProgram(ocl->program, 1, &ocl->device, "", NULL, NULL);
	if (CL_SUCCESS != err)
	{
		printf("Error: clBuildProgram() for source program returned %s.\n", TranslateOpenCLError(err));

		// In case of error print the build log to the standard output
		// First check the size of the log
		// Then allocate the memory and obtain the log from the program
		if (err == CL_BUILD_PROGRAM_FAILURE)
		{
			size_t log_size = 0;
			clGetProgramBuildInfo(ocl->program, ocl->device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);

			std::vector<char> build_log(log_size);
			clGetProgramBuildInfo(ocl->program, ocl->device, CL_PROGRAM_BUILD_LOG, log_size, &build_log[0], NULL);

			printf("Error happened during the build of OpenCL program.\nBuild log:%s", &build_log[0]);
		}
	}

Finish:
	if (source)
	{
		delete[] source;
		source = NULL;
	}

	return err;
}


/*
* Create OpenCL buffers from host memory
* These buffers will be used later by the OpenCL kernel
*/
int CreateBufferArguments(ocl_args_d_t *ocl, RectangleLight* Lightlist, int LightCount, Plane* Shapelist, int ShapeCount, cl_uint sampleCount, Camera* cam,
	cl_uint* output, cl_uint* seeds, cl_uint tmpworkAmount, cl_uint width, cl_uint height)
{
	cl_int err = CL_SUCCESS;

	// Create new OpenCL buffer objects
	// As these buffer are used only for read by the kernel, you are recommended to create it with flag CL_MEM_READ_ONLY.
	// Always set minimal read/write flags for buffers, it may lead to better performance because it allows runtime
	// to better organize data copying.
	// You use CL_MEM_COPY_HOST_PTR here, because the buffers should be populated with bytes at inputA and inputB.

	ocl->Lights = clCreateBuffer(ocl->context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(RectangleLight) * LightCount, Lightlist, &err);
	if (CL_SUCCESS != err)
	{
		printf("Error: clCreateBuffer for srcA returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	ocl->LightCount = LightCount;
	
	ocl->Shapes = clCreateBuffer(ocl->context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, sizeof(Plane) * ShapeCount, Shapelist, &err);
	if (CL_SUCCESS != err)
	{
		printf("Error: clCreateBuffer for srcB returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	ocl->ShapeCount = ShapeCount;

	ocl->sampleCount = sampleCount;
	
	ocl->cam = clCreateBuffer(ocl->context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, sizeof(Camera), cam, &err);
	if (CL_SUCCESS != err)
	{
		printf("Error: clCreateBuffer for dstMem returned %s\n", TranslateOpenCLError(err));
		return err;
	}
	// If the output buffer is created directly on top of output buffer using CL_MEM_USE_HOST_PTR,
	// then, depending on the OpenCL runtime implementation and hardware capabilities, 
	// it may save you not necessary data copying.
	// As it is known that output buffer will be write only, you explicitly declare it using CL_MEM_WRITE_ONLY.
	ocl->Pixels = clCreateBuffer(ocl->context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, sizeof(cl_uint) * width * height, output, &err);
	if (CL_SUCCESS != err)
	{
		printf("Error: clCreateBuffer for dstMem returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	ocl->Seeds = clCreateBuffer(ocl->context, CL_MEM_WRITE_ONLY | CL_MEM_USE_HOST_PTR, sizeof(cl_uint) * tmpworkAmount * 2, seeds, &err);
	if (CL_SUCCESS != err)
	{
		printf("Error: clCreateBuffer for dstMem returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	return CL_SUCCESS;
}


/*
* Set kernel arguments
*/
cl_uint SetKernelArguments(ocl_args_d_t *ocl)
{
	cl_int err = CL_SUCCESS;

	err = clSetKernelArg(ocl->kernel, 0, sizeof(cl_mem), (void *)&ocl->Lights);
	if (CL_SUCCESS != err)
	{
		printf("error: Failed to set argument Lights, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	err = clSetKernelArg(ocl->kernel, 1, sizeof(cl_uint), (void *)&ocl->LightCount);
	if (CL_SUCCESS != err)
	{
		printf("Error: Failed to set argument LightCount, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	err = clSetKernelArg(ocl->kernel, 2, sizeof(cl_mem), (void *)&ocl->Shapes);
	if (CL_SUCCESS != err)
	{
		printf("error: Failed to set argument Shapes, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	err = clSetKernelArg(ocl->kernel, 3, sizeof(cl_uint), (void *)&ocl->ShapeCount);
	if (CL_SUCCESS != err)
	{
		printf("Error: Failed to set argument ShapeCount, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	err = clSetKernelArg(ocl->kernel, 4, sizeof(cl_uint), (void *)&ocl->sampleCount);
	if (CL_SUCCESS != err)
	{
		printf("Error: Failed to set argument ShapeCount, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	err = clSetKernelArg(ocl->kernel, 5, sizeof(cl_uint), (void *)&ocl->width);
	if (CL_SUCCESS != err)
	{
		printf("Error: Failed to set argument ShapeCount, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	err = clSetKernelArg(ocl->kernel, 6, sizeof(cl_uint), (void *)&ocl->height);
	if (CL_SUCCESS != err)
	{
		printf("Error: Failed to set argument ShapeCount, returned %s\n", TranslateOpenCLError(err));
		return err;
	}


	err = clSetKernelArg(ocl->kernel, 7, sizeof(cl_mem), (void *)&ocl->cam);
	if (CL_SUCCESS != err)
	{
		printf("Error: Failed to set argument ShapeCount, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	err = clSetKernelArg(ocl->kernel, 8, sizeof(cl_uint), (void *)&ocl->Seeds);
	if (CL_SUCCESS != err)
	{
		printf("Error: Failed to set argument ShapeCount, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	err = clSetKernelArg(ocl->kernel, 9, sizeof(cl_mem), (void *)&ocl->Pixels);
	if (CL_SUCCESS != err)
	{
		printf("Error: Failed to set argument Pixels, returned %s\n", TranslateOpenCLError(err));
		return err;
	}

	return err;
}


/*
* Execute the kernel
*/
cl_uint ExecuteAddKernel(ocl_args_d_t *ocl, cl_uint* Pixels, cl_uint globalSize, cl_uint localSize, cl_uint width, cl_uint height, int workAmount, int workCount)
{
	cl_int err = CL_SUCCESS;

	unsigned int remain_size = width * height;

	for (int i = 0; i < workCount; i++)
	{
		// Define global iteration space for clEnqueueNDRangeKernel.
		unsigned int tmpGlobalSize;
		if ((i + 1) == workCount && (remain_size % workAmount != 0))
			tmpGlobalSize = remain_size;
		else
			tmpGlobalSize = globalSize;

		remain_size = remain_size - globalSize;
		if (tmpGlobalSize < localSize)
			localSize = tmpGlobalSize;

		size_t globalWorkSize[1] = { tmpGlobalSize };
		size_t localWorkSize[1] = { localSize };

		err = clSetKernelArg(ocl->kernel, 10, sizeof(cl_uint), (void *)&i);
		if (CL_SUCCESS != err)
		{
			printf("Error: Failed to set argument Pixels, returned %s\n", TranslateOpenCLError(err));
			return err;
		}

		// execute kernel
		err = clEnqueueNDRangeKernel(ocl->commandQueue, ocl->kernel, 1, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
		if (CL_SUCCESS != err)
		{
			printf("Error: Failed to run kernel, return %s\n", TranslateOpenCLError(err));
			return err;
		}

	}

	return CL_SUCCESS;
}


/*
* "Read" the result buffer (mapping the buffer to the host memory address)
*/
bool ReleaseInfo(ocl_args_d_t *ocl, cl_uint width, cl_uint height)
{
	cl_int err = CL_SUCCESS;
	bool result = true;

	// Enqueue a command to map the buffer object (ocl->dstMem) into the host address space and returns a pointer to it
	// The map operation is blocking
	cl_uint *resultPtr = (cl_uint *)clEnqueueMapBuffer(ocl->commandQueue, ocl->Pixels, true, CL_MAP_READ, 0, sizeof(cl_uint) * width * height, 0, NULL, NULL, &err);

	if (CL_SUCCESS != err)
	{
		printf("Error: clEnqueueMapBuffer returned %s\n", TranslateOpenCLError(err));
		return false;
	}

	// Call clFinish to guarantee that output region is updated
	err = clFinish(ocl->commandQueue);
	if (CL_SUCCESS != err)
	{
		printf("Error: clFinish returned %s\n", TranslateOpenCLError(err));
	}

	// Unmapped the output buffer before releasing it
	err = clEnqueueUnmapMemObject(ocl->commandQueue, ocl->Pixels, resultPtr, 0, NULL, NULL);
	if (CL_SUCCESS != err)
	{
		printf("Error: clEnqueueUnmapMemObject returned %s\n", TranslateOpenCLError(err));
	}

	std::ostringstream headerStream;
	headerStream << "P6\n";
	headerStream << kWidth << ' ' << kHeight << '\n';
	headerStream << "255\n";
	std::ofstream fileStream("out.ppm", std::ios::out | std::ios::binary);

	fileStream << headerStream.str();
	for (unsigned int y = 0; y < height; y++)
	{
		for (unsigned int x = 0; x < width; x++)
		{
			unsigned char r, g, b;
			unsigned int tmp = resultPtr[y*width + x];
			r = (unsigned char)((tmp >> 16) & 0xFF);
			g = (unsigned char)((tmp >> 8) & 0xFF);
			b = (unsigned char)((tmp)& 0xFF);
			fileStream << r << g << b;
		}
	}

	fileStream.flush();
	fileStream.close();

	return result;
}

void generateArgument(SphereSet* tmpSphereSet, Camera* tmpCam, cl_uint* tmpSeeds)
{
	tmpSphereSet->m_rectLight = new RectangleLight[2];
	tmpSphereSet->m_plane = new Plane[1];
	tmpSphereSet->PlaneCount = 1;
	Point tmp_p = { 0.0f, -2.0f, 0.0f };
	Vector tmp_v1 = { 0.0f, 1.0f, 0.0f };
	Vector tmp_v2 = { 0.0f, 0.0f, 0.0f };
	Color tmp_c = { 1.0f, 1.0f, 1.0f };

	float tmp_power = 0.0;
	int i = 0;

	// Plane Set
	tmpSphereSet->m_plane[0] = { tmp_p, tmp_v1, tmp_c };

	// Rectangle Light Set
	tmpSphereSet->LightCount = 2;
	tmp_p = { 2.5f, 2.0f, -2.5f };
	tmp_v1 = { 5.0f, 0.0f, 0.0f };
	tmp_v2 = { 0.0f, 0.0f, 5.0f };
	tmp_c = { 1.0f, 0.5f, 1.0f };
	tmp_power = 3.0f;
	tmpSphereSet->m_rectLight[0] = { tmp_p, tmp_v1, tmp_v2, tmp_c, tmp_power };
	
	tmp_p = { -2.0f, -1.0f, -2.0f };
	tmp_v1 = { 4.0f, 0.0f, 0.0f };
	tmp_v2 = { 0.0f, 0.0f, 4.0f };
	tmp_c = { 1.0f, 1.0f, 0.5f };
	tmp_power = 0.75f;
	tmpSphereSet->m_rectLight[1] = { tmp_p, tmp_v1, tmp_v2, tmp_c, tmp_power };
	
	Point tmp_p1 = { 0.0f, 5.0f, 15.0f };
	Point tmp_p2 = { 0.0f, 0.0f, 0.0f };
	Point tmp_p3 = { 0.0f, 1.0f, 0.0f };

	*tmpCam = {45.0f, tmp_p1, tmp_p2, tmp_p3};

	srand((unsigned int)time(NULL));

	for (i = 0; i < workAmount; i++)
	{
		tmpSeeds[i] = rand();
		if (tmpSeeds[i] < 2)
			tmpSeeds[i] = 2;
	}

}

int main(int argc, char **argv)
{
	cl_int err;
	ocl_args_d_t ocl;
	cl_device_type deviceType = CL_DEVICE_TYPE_GPU;

	cl_uint arrayWidth = kWidth;
	cl_uint arrayHeight = kHeight;
	cl_uint sampleCount = kNumPixelSamples;
	cl_uint workCount;
	cl_uint globalWorkSize = workAmount;
	cl_uint localWorkSize;
	if ((arrayWidth * arrayHeight) % workAmount == 0)
		workCount = (arrayWidth * arrayHeight) / workAmount;
	else
		workCount = (arrayWidth * arrayHeight) / workAmount + 1;

	ocl.width = kWidth;
	ocl.height = kHeight;
	
	clock_t begin, end;

	SphereSet masterSet;
	Camera cam;
	//initialize Open CL objects (context, queue, etc.)
	if (CL_SUCCESS != SetupOpenCL(&ocl, deviceType))
	{
		return -1;
	}

	// allocate working buffers. 
	// the buffer should be aligned with 4K page and size should fit 64-byte cached line
	cl_uint optimizedSize = ((sizeof(cl_uint) * arrayWidth * arrayHeight - 1) / 64 + 1) * 64;
	cl_uint Seeds[workAmount * 2];
	cl_uint* Pixels = (cl_uint*)_aligned_malloc(optimizedSize, 4096);

	generateArgument(&masterSet, &cam, Seeds);

	begin = clock();
	
	// Create OpenCL buffers from host memory
	// These buffers will be used later by the OpenCL kernel
	if (CL_SUCCESS != CreateBufferArguments(&ocl, masterSet.m_rectLight, masterSet.LightCount, 
		masterSet.m_plane, masterSet.PlaneCount, sampleCount, &cam, Pixels, Seeds, workAmount, arrayWidth, arrayHeight))
	{
		return -1;
	}

	// Create and build the OpenCL program
	if (CL_SUCCESS != CreateAndBuildProgram(&ocl))
	{
		return -1;
	}

	// Program consists of kernels.
	// Each kernel can be called (enqueued) from the host part of OpenCL application.
	// To call the kernel, you need to create it from existing program.
	ocl.kernel = clCreateKernel(ocl.program, "ray_cal", &err);
	if (CL_SUCCESS != err)
	{
		printf("Error: clCreateKernel returned %s\n", TranslateOpenCLError(err));
		return -1;
	}

	// Passing arguments into OpenCL kernel.
	if (CL_SUCCESS != SetKernelArguments(&ocl))
	{
		return -1;
	}

	if (CL_SUCCESS != GetWorkGroupInfo(&ocl, workAmount, &localWorkSize))
	{
		return -1;
	}
	//localWorkSize = 1;
	// Execute (enqueue) the kernel
	if (CL_SUCCESS != ExecuteAddKernel(&ocl, Pixels, globalWorkSize, localWorkSize, arrayWidth, arrayHeight, workAmount, workCount))
	{
		return -1;
	}

	// The last part of this function: getting processed results back.
	// use map-unmap sequence to update original memory area with output buffer.
	
	ReleaseInfo(&ocl, arrayWidth, arrayHeight);
	
	end = clock();
	printf("elapsed time : %lfs\n", (double)(end - begin) / CLOCKS_PER_SEC);

	_aligned_free(Pixels);
	//getchar();
	return 0;
}
