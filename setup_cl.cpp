//
// Helper functions to setup OpenCL and load kernels
//
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <locale>
#include <exception>
#include "setup_cl.h"

// Helper function to report available platforms and devices and create and return an OpenCL context
cl_context createContext(void) 
{
	cl_int				clerr;
	cl_uint				numPlatforms;
	cl_platform_id		*platformArray;
	std::locale			loc;
	cl_context			context = nullptr;
	const				std::string platformToFind = std::string("NVIDIA");
	int					platformIndex = -1;

	try 
	{
		// Query the number of platforms
		clerr = clGetPlatformIDs(0, nullptr, &numPlatforms);

		if (clerr != CL_SUCCESS || numPlatforms == 0)
			throw std::exception("No OpenCL platforms found");

		// Create the array of available platforms
		platformArray = (cl_platform_id*)malloc(numPlatforms * sizeof(cl_platform_id));

		if (!platformArray)
			throw std::exception("Unable to create platform array");

		// Get platform list - for this application we'll use only the first platform found
		clerr = clGetPlatformIDs(numPlatforms, platformArray, nullptr);

		// Validate returned platform
		if (clerr != CL_SUCCESS || numPlatforms == 0)
			throw std::exception("Unable to obtain platform information");

		// Cycle through each platform and display information
		for (cl_uint i = 0; i < numPlatforms; ++i) 
		{
			// Used to store size of buffer for clGetPlatformInfo to put results of queries
			size_t resultSize; 

			clerr = clGetPlatformInfo(platformArray[i], CL_PLATFORM_PROFILE, 0, nullptr, &resultSize);
			char *platformProfile = static_cast<char*>(malloc(resultSize));
			clerr = clGetPlatformInfo(platformArray[i], CL_PLATFORM_PROFILE, resultSize, platformProfile, nullptr);

			clerr = clGetPlatformInfo(platformArray[i], CL_PLATFORM_NAME, 0, nullptr, &resultSize);
			char *platformName = static_cast<char*>(malloc(resultSize));
			clerr = clGetPlatformInfo(platformArray[i], CL_PLATFORM_NAME, resultSize, platformName, nullptr);

			clerr = clGetPlatformInfo(platformArray[i], CL_PLATFORM_VERSION, 0, nullptr, &resultSize);
			char *platformVersion = static_cast<char*>(malloc(resultSize));
			clerr = clGetPlatformInfo(platformArray[i], CL_PLATFORM_VERSION, resultSize, platformVersion, nullptr);

			clerr = clGetPlatformInfo(platformArray[i], CL_PLATFORM_VENDOR, 0, nullptr, &resultSize);
			char *platformVendor = static_cast<char*>(malloc(resultSize));
			clerr = clGetPlatformInfo(platformArray[i], CL_PLATFORM_VENDOR, resultSize, platformVendor, nullptr);

			std::cout << "Platform " << i << " profile : " << platformProfile
					  << "\nPlatform " << i << " name    : " << platformName
					  << "\nPlatform " << i << " version : " << platformVersion
					  << "\nPlatform " << i << " vendor  : " << platformVendor << std::endl;

			// Query and report info on the devices available in the current platform
			cl_uint numDevices;

			// Query number of devices
			clGetDeviceIDs(platformArray[i], CL_DEVICE_TYPE_ALL, 0, nullptr, &numDevices);

			// Allocate buffer to store device info
			cl_device_id *devices = static_cast<cl_device_id*>(malloc(numDevices * sizeof(cl_device_id)));

			// Get device info
			clGetDeviceIDs(platformArray[i], CL_DEVICE_TYPE_ALL, numDevices, devices, nullptr);

			for (cl_uint j = 0; j < numDevices; ++j) 
			{
				cl_uint maxComputeUnits, maxWorkItemDim;

				clGetDeviceInfo(devices[j], CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(cl_uint), &maxComputeUnits, &resultSize);
				clGetDeviceInfo(devices[j], CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &maxWorkItemDim, &resultSize);

				std::cout << "\nMax compute units for device " << j << " = " << maxComputeUnits 
						  << "\nMax work item dimensions for device " << j << " = " << maxWorkItemDim 
						  << std::endl;
			}

			std::cout << "// --------------------------\n\n";

			// Check if platform i is an nVidia-based platform
			for (size_t k = 0; k < strlen(platformName); ++k)
				platformName[k] = toupper(platformName[k], loc);

			size_t found = std::string(platformName).find(platformToFind);
			if (found != std::string::npos && platformIndex == -1)
				platformIndex = i;
		}

		// Create OpenCL context based on the first available nVidia platform
		cl_context_properties contextProperties[] = 
		{
			CL_CONTEXT_PLATFORM,
			reinterpret_cast<cl_context_properties>(platformArray[platformIndex]),
			0
		};

		context = clCreateContextFromType(contextProperties, CL_DEVICE_TYPE_GPU, nullptr, nullptr, nullptr);

		if (clerr != CL_SUCCESS || !context)
			throw std::exception("Unable to create a valid GPU context");

		return context;
	}
	catch (std::exception& err)
	{
		std::cout << err.what() << std::endl;
		return nullptr;
	}
}


// Helper function to load the kernel source code from a suitable (text) file and setup an OpenCL program object
cl_program createProgram(cl_context context, cl_device_id device, const char* fileName) 
{
	// Open the kernel file - the extension can be anything, including .txt since it's just a text file
	std::ifstream kernelFile(fileName, std::ios::in);

	if (!kernelFile.is_open()) 
	{
		std::cout << "Cannot open source file " << fileName << std::endl;
		return nullptr;
	}

	// Store the file contents in a stringstream object
	std::ostringstream oss;
	oss << kernelFile.rdbuf();

	// Extract a std::string from the stringstream object
	std::string srcString = oss.str();

	// Obtain the pointer to the contained C string
	const char* src = srcString.c_str();

	cl_int err;

	cl_program program = clCreateProgramWithSource(context, 1, static_cast<const char**>(&src), 0, &err);

	if (!program)
	{
		std::cout << "Cannot create program from source file " << fileName << std::endl;
		return nullptr;
	}

	// Attempt to build program object
	err = clBuildProgram(program, 0, 0, 0, 0, 0);

	if (err != CL_SUCCESS) 
	{
		size_t logSize;
		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);

		char *buildLog = static_cast<char*>(calloc(logSize + 1, 1));

		clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, logSize, buildLog, nullptr);

		std::cout << "Error in kernel:\n\n";
		std::cout << buildLog;

		// Clean-up
		clReleaseProgram(program);
		return nullptr;
	}
	return program;
}