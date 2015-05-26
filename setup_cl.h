#ifndef _SETUP_CL_
#define _SETUP_CL_

#include <CL\opencl.h>

cl_context createContext();
cl_program createProgram(cl_context context, cl_device_id device, const char* fileName);

#endif