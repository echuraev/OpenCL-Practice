#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include <fstream>
#include <iostream>
#include <string.h>
#include <cassert>

#define BUFF_SIZE 10

int main()
{
    // Init environment
    cl_platform_id platform_id;
    cl_uint ret_num_platforms;
    cl_device_id device_id;
    cl_uint ret_num_devices;

    int err = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    assert(err == CL_SUCCESS);
    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_CPU, 1, &device_id, &ret_num_devices);
    assert(err == CL_SUCCESS);

    // Create context
    cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    assert(err == CL_SUCCESS);
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &err);
    assert(err == CL_SUCCESS);

    // Read Kernel
    std::ifstream kernel_src("vectorAdd.cl");
    if (!kernel_src.is_open())
        return 1;

    std::string str((std::istreambuf_iterator<char>(kernel_src)), std::istreambuf_iterator<char>());

    kernel_src.close();

    size_t source_size = str.length();
    char *source_str = new char[source_size + 1];
    strcpy(source_str, str.c_str());

    // Create program
    cl_program program = clCreateProgramWithSource(context, 1,  const_cast<const char **>(&source_str), NULL, &err);
    assert(err == CL_SUCCESS);
    //err = clBuildProgram(program, 1, &device_id, "-g -s vectorAdd.cl", NULL, NULL);
    err = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    assert(err == CL_SUCCESS);

    // Create buffers
    cl_int *a_arr = new cl_int[BUFF_SIZE];
    cl_int *b_arr = new cl_int[BUFF_SIZE];
    cl_int *c_arr = new cl_int[BUFF_SIZE];

    // Create and print arrays
    for (int i(0); i < BUFF_SIZE; ++i)
    {
        a_arr[i] = i;
        std::cout << a_arr[i] << ' ';
    }
    std::cout << std::endl;
    std::cout << "+\n";
    for (int i(0); i < BUFF_SIZE; ++i)
    {
        b_arr[i] = 10*i;
        std::cout << b_arr[i] << ' ';
    }
    std::cout << std::endl;

    cl_mem a_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, BUFF_SIZE * sizeof(cl_int), a_arr, &err);
    assert(err == CL_SUCCESS);
    cl_mem b_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, BUFF_SIZE * sizeof(cl_int), b_arr, &err);
    assert(err == CL_SUCCESS);
    cl_mem c_mem = clCreateBuffer(context, CL_MEM_WRITE_ONLY, BUFF_SIZE * sizeof(cl_int), NULL, &err);
    assert(err == CL_SUCCESS);

    // Create kernel
    cl_kernel kernel = clCreateKernel(program, "vectorAdd", &err);
    assert(err == CL_SUCCESS);

    // Set kernel arguments
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&a_mem);
    assert(err == CL_SUCCESS);
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&b_mem);
    assert(err == CL_SUCCESS);
    err = clSetKernelArg(kernel, 2, sizeof(cl_mem), (void *)&c_mem);
    assert(err == CL_SUCCESS);

    // Run kernel
    size_t global_work_size[1] = { BUFF_SIZE }; // Define global size of execution
    err = clEnqueueNDRangeKernel(command_queue, kernel, 1, NULL, global_work_size, NULL, 0, NULL, NULL);
    assert(err == CL_SUCCESS);
    // Read buffer with result of calculation
    err = clEnqueueReadBuffer(command_queue, c_mem, CL_TRUE, 0, BUFF_SIZE * sizeof(cl_int), c_arr, 0, NULL, NULL);
    assert(err == CL_SUCCESS);

    std::cout << "=\n";

    for (int i(0); i < BUFF_SIZE; ++i)
    {
        std::cout << c_arr[i] << ' ';
    }
    std::cout << std::endl;

    delete [] source_str;
    delete [] a_arr;
    delete [] b_arr;
    delete [] c_arr;

    return 0;
}
