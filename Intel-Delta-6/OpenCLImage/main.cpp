#ifdef __APPLE__
#include <OpenCL/opencl.h>
#else
#include <CL/cl.h>
#endif
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <cstring>
#include <cassert>
#include "error.h"
#include "imagefunctions.h"

int main()
{
    const std::string in_image = "intel_orig.bmp";
    const std::string out_image = "out_intel.bmp";

    cl_platform_id platform_id;
    cl_uint ret_num_platforms;
    cl_device_id device_id;
    cl_uint ret_num_devices;

    // ============ INIT ENV ============
    int err = clGetPlatformIDs(1, &platform_id, &ret_num_platforms);
    assert(err == CL_SUCCESS);
    err = clGetDeviceIDs(platform_id, CL_DEVICE_TYPE_CPU, 1, &device_id, &ret_num_devices);
    assert(err == CL_SUCCESS);

    cl_context context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    assert(err == CL_SUCCESS);
    cl_command_queue command_queue = clCreateCommandQueue(context, device_id, 0, &err);
    assert(err == CL_SUCCESS);

    // ============ READ KERNEL ============
    std::ifstream kernel_src("test_kernel.cl");
    if (!kernel_src.is_open())
        return 1;

    std::string str((std::istreambuf_iterator<char>(kernel_src)), std::istreambuf_iterator<char>());
    kernel_src.close();

    size_t source_size = str.length();
    char *source_str = new char[source_size + 1];
    strcpy(source_str, str.c_str());

    // ============ CREATE PROGRAMM ============
    cl_program program = clCreateProgramWithSource(context, 1, const_cast<const char **>(&source_str), &source_size, &err);
    assert(err == CL_SUCCESS);
    err = clBuildProgram(program, 1, &device_id, NULL, NULL, NULL);
    assert(err == CL_SUCCESS);

    cl_kernel kernel = clCreateKernel(program, "colorToBW", &err);
    assert(err == CL_SUCCESS);

    // ============ READ IMAGES ============
    cl_int2 img_size;
    unsigned char *img;
    try
    {
        img = LoadImageAsBMP(in_image, img_size);
    }
    catch (Error& e)
    {
        std::cerr << "ERROR: " << e.getMsg() << std::endl;
        return 1;
    }

    // ============ CREATE INPUT AND OUPUT IMAGES ============
    cl_image_format format;             // structure to define image format
    format.image_channel_data_type = CL_UNORM_INT8;
    format.image_channel_order = CL_RGBA;

    // init image description
    cl_image_desc desc = { 0 };               // structure to define image description
    desc.image_type = CL_MEM_OBJECT_IMAGE2D;
    desc.image_width = img_size.s[0];
    desc.image_height = img_size.s[1];

    // Allocate input_img image initialized by pixels loaded from .BMP file
    cl_mem input_img = clCreateImage(
        context,
        CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
        &format,
        &desc,
        &img[0],
        &err);
    assert(err == CL_SUCCESS);

    // Allocate non-initialized output buffer 
    cl_mem output_image = clCreateImage(
        context,
        CL_MEM_WRITE_ONLY,
        &format,
        &desc,
        NULL,
        &err);
    assert(err == CL_SUCCESS);

    size_t global_work_size[2] = { static_cast<size_t>(img_size.s[0]), static_cast<size_t>(img_size.s[1]) };

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), (void *)&input_img);
    assert(err == CL_SUCCESS);
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), (void *)&output_image);
    assert(err == CL_SUCCESS);

    err = clEnqueueNDRangeKernel(command_queue, kernel, 2, NULL, global_work_size, NULL, 0, NULL, NULL);
    assert(err == CL_SUCCESS);

    // GET RESULT
    size_t originst[3];
    size_t regionst[3];
    size_t rowPitch = 0;
    size_t slicePitch = 0;
    originst[0] = 0; originst[1] = 0; originst[2] = 0;
    regionst[0] = img_size.s[0]; regionst[1] = img_size.s[1]; regionst[2] = 1;

    char *p = new char[img_size.s[0] * img_size.s[1] * 4];
    err = clEnqueueReadImage(command_queue, output_image, CL_TRUE, originst, regionst, rowPitch, slicePitch, p, 0, NULL, NULL);
    assert(err == CL_SUCCESS);

    SaveImageAsBMP((unsigned int*)p, img_size.s[0], img_size.s[1], out_image);

    delete [] p;
    delete [] source_str;

    std::cout << "Done!" << std::endl;

    return 0;
}
