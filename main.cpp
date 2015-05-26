#include <cstdio>
#include <iostream>
#include "setup_cl.h"
#include "imageio.h"

int main(void)
{
	// Initialise COM so we can export image data using WIC
	initCOM();

	// Create and validate the OpenCL context
	cl_context context = createContext();

	if (!context)
	{
		std::cout << "cl context not created\n";
		shutdownCOM();
		return 1;
	}

	// Query the device from the context - should be the GPU since we requested this before
	size_t deviceBufferSize;
	cl_int errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, 0, 0, &deviceBufferSize);

	cl_device_id* contextDevices = static_cast<cl_device_id*>(malloc(deviceBufferSize));

	errNum = clGetContextInfo(context, CL_CONTEXT_DEVICES, deviceBufferSize, contextDevices, 0);

	cl_device_id device = contextDevices[0];

	// Create and validate the program object based on HelloWorld.cl
	cl_program program = createProgram(context, device, "Resources\\Kernels\\HelloWorld.cl");

	// Create and validate the command queue (for first device in context)
	// Note: add profiling flag so we can get timing data from event
	cl_command_queue commandQueue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, 0);

	if (!commandQueue)
	{
		std::cout << "command queue not created\n";
		shutdownCOM();
		return 1;
	}

	CPFloatImage F;

	// Use WIC to load image and extract RGBA channels as float buffers
	loadImage(std::wstring(L"Resources\\Images\\Llandaf_highres.jpg"), &F);

	// Setup input image as read only (appears as const parameter in the kernel)
	cl_mem inputBufferRed  = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
										    F.w * F.h * sizeof(float), F.redChannel, 0);

	cl_mem inputBufferGreen = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
											 F.w * F.h * sizeof(float), F.greenChannel, 0);

	cl_mem inputBufferBlue  = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, 
											 F.w * F.h * sizeof(float), F.blueChannel, 0);
	// ------------------------------------------------------------------------------

	// Setup buffer to store the output image (again don't need to provide data 
	// - we fill the buffer from the kernels)
	cl_mem outputBufferRed   = clCreateBuffer(context, CL_MEM_READ_WRITE, F.w * F.h * sizeof(float), 0, 0);
	cl_mem outputBufferGreen = clCreateBuffer(context, CL_MEM_READ_WRITE, F.w * F.h * sizeof(float), 0, 0);
	cl_mem outputBufferBlue  = clCreateBuffer(context, CL_MEM_READ_WRITE, F.w * F.h * sizeof(float), 0, 0);

	cl_mem outputBufferRedOne   = clCreateBuffer(context, CL_MEM_READ_WRITE, F.w * F.h * sizeof(float), 0, 0);
	cl_mem outputBufferGreenOne = clCreateBuffer(context, CL_MEM_READ_WRITE, F.w * F.h * sizeof(float), 0, 0);
	cl_mem outputBufferBlueOne  = clCreateBuffer(context, CL_MEM_READ_WRITE, F.w * F.h * sizeof(float), 0, 0);

	// Get the relevant kernel from the program object
	cl_kernel xyyImageKernel   = clCreateKernel(program, "RGB_XYY", 0);
	cl_kernel xyzImageKernel   = clCreateKernel(program, "XYY_XYZ", 0);
	cl_kernel xyy_LImageKernel = clCreateKernel(program, "XYY_L", 0);

	// Set kernel arguements - note the width is passed by reference to the image 
	// structure variable
	clSetKernelArg(xyyImageKernel, 0, sizeof(cl_mem), &inputBufferRed);
	clSetKernelArg(xyyImageKernel, 1, sizeof(cl_mem), &inputBufferGreen);
	clSetKernelArg(xyyImageKernel, 2, sizeof(cl_mem), &inputBufferBlue);
	clSetKernelArg(xyyImageKernel, 3, sizeof(cl_mem), &outputBufferRed);
	clSetKernelArg(xyyImageKernel, 4, sizeof(cl_mem), &outputBufferGreen);
	clSetKernelArg(xyyImageKernel, 5, sizeof(cl_mem), &outputBufferBlue);
	clSetKernelArg(xyyImageKernel, 6, sizeof(cl_int), &F.w);

	clSetKernelArg(xyzImageKernel, 0, sizeof(cl_mem), &outputBufferRed);
	clSetKernelArg(xyzImageKernel, 1, sizeof(cl_mem), &outputBufferGreen);
	clSetKernelArg(xyzImageKernel, 2, sizeof(cl_mem), &outputBufferBlue);
	clSetKernelArg(xyzImageKernel, 3, sizeof(cl_mem), &outputBufferRedOne);
	clSetKernelArg(xyzImageKernel, 4, sizeof(cl_mem), &outputBufferGreenOne);
	clSetKernelArg(xyzImageKernel, 5, sizeof(cl_mem), &outputBufferBlueOne);
	clSetKernelArg(xyzImageKernel, 6, sizeof(cl_int), &F.w);

	clSetKernelArg(xyy_LImageKernel, 0, sizeof(cl_mem), &outputBufferRedOne);
	clSetKernelArg(xyy_LImageKernel, 1, sizeof(cl_mem), &outputBufferGreenOne);
	clSetKernelArg(xyy_LImageKernel, 2, sizeof(cl_mem), &outputBufferBlueOne);
	clSetKernelArg(xyy_LImageKernel, 3, sizeof(cl_mem), &outputBufferRed);
	clSetKernelArg(xyy_LImageKernel, 4, sizeof(cl_mem), &outputBufferGreen);
	clSetKernelArg(xyy_LImageKernel, 5, sizeof(cl_mem), &outputBufferBlue);
	clSetKernelArg(xyy_LImageKernel, 6, sizeof(cl_int), &F.w);

	// Setup the global work size based on the image dimensions
	size_t imageWrkSize[2]      = { F.w, F.h };
	size_t imageLocalWrkSize[2] = { 16, 16 };

	cl_event xyyEvent, xyzEvent, xyy_lEvent;

	cl_int err = clEnqueueNDRangeKernel(commandQueue, xyyImageKernel, 2, 0, imageWrkSize, 
										imageLocalWrkSize, 0, 0, &xyyEvent);

	err = clEnqueueNDRangeKernel(commandQueue, xyzImageKernel, 2, 0, imageWrkSize, 
								 imageLocalWrkSize, 1, &xyyEvent, &xyzEvent); 
	
	err = clEnqueueNDRangeKernel(commandQueue, xyy_LImageKernel, 2, 0, imageWrkSize, 
								 imageLocalWrkSize, 1, &xyzEvent, &xyy_lEvent);

	// Synchronisation point
	clWaitForEvents(1, &xyy_lEvent);

	cl_ulong cl_t0 = static_cast<cl_ulong>(0);
	cl_ulong cl_t1 = static_cast<cl_ulong>(0);

	clGetEventProfilingInfo(xyyEvent, CL_PROFILING_COMMAND_START, sizeof(cl_ulong), &cl_t0, 0);
	clGetEventProfilingInfo(xyy_lEvent, CL_PROFILING_COMMAND_END, sizeof(cl_ulong), &cl_t1, 0);

	double cl_tdelta = static_cast<double>((cl_t1 - cl_t0));

	std::cout << "Time taken = " << (cl_tdelta * 1.0e-9) << std::endl;

	// Setup buffers to store the result
	float* redOut   = static_cast<float*>(malloc(F.w * F.h * sizeof(float)));
	float* greenOut = static_cast<float*>(malloc(F.w * F.h * sizeof(float)));
	float* blueOut  = static_cast<float*>(malloc(F.w * F.h * sizeof(float)));

	// Get results
	err = clEnqueueReadBuffer(commandQueue, outputBufferRed, CL_TRUE, 0, F.w * F.h * sizeof(float), redOut, 0, 0, 0);
	err = clEnqueueReadBuffer(commandQueue, outputBufferGreen, CL_TRUE, 0, F.w * F.h * sizeof(float), greenOut, 0, 0, 0);
	err = clEnqueueReadBuffer(commandQueue, outputBufferBlue, CL_TRUE, 0, F.w * F.h * sizeof(float), blueOut, 0, 0, 0);

	saveImage(F.w, F.h, redOut, greenOut, blueOut, std::wstring(L"result.bmp"));

	shutdownCOM();
	return 0;
}