#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include "imagefunctions.h"

int main()
{
    const std::string in_image = "intel_orig.bmp";
    const std::string out_image = "out_intel.bmp";

    int errCode = 0;

    try
    {
        // Get platforms
        std::vector<cl::Platform> platforms;
        cl::Platform::get(&platforms);

        cl::Platform platform = platforms[0];
        std::cout << "Using platforms: " << platform.getInfo<CL_PLATFORM_NAME>() << std::endl;

        // get default device
        std::vector<cl::Device> devices;
        platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
        cl::Device device = devices[0];
        std::cout << "Using device: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;

        // Create context
        cl::Context context(device);

        // Create queue
        cl::CommandQueue queue(context, device);

        // Read Kernel
        std::ifstream kernel_src("OpenCLImages.cl");
        if (!kernel_src.is_open())
            throw cl::Error(1, "Cannot open file with kernel!");

        std::string str((std::istreambuf_iterator<char>(kernel_src)), std::istreambuf_iterator<char>());

        kernel_src.close();

        cl::Program::Sources sources;
        sources.push_back({str.c_str(), str.length()});

        cl::Program program(context, sources);
        program.build({device});
        //program.build({device}, "-g -s OpenCLImages.cl");

        // Read image
        cl_int2 img_size;
        unsigned char *img = LoadImageAsBMP(in_image, img_size);

        // ============ CREATE INPUT AND OUPUT IMAGES ============
        cl::ImageFormat format(CL_RGBA, CL_UNORM_INT8); // structure to define image description

        // Allocate input_image image initialized by pixels loaded from .BMP file
        cl::Image2D input_image(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, format, img_size.s[0], img_size.s[1], 0, &img[0]);
        // Allocate non-initialized output buffer
        cl::Image2D output_image(context, CL_MEM_WRITE_ONLY, format, img_size.s[0], img_size.s[1]);

        cl::Kernel kernel = cl::Kernel(program, "colorToBW");
        kernel.setArg(0, input_image);
        kernel.setArg(1, output_image);
        size_t xSize = img_size.s[0];
        size_t ySize = img_size.s[1];
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(xSize, ySize), cl::NullRange);

        // GET RESULT
        cl::size_t<3> origin;
        cl::size_t<3> region;
        size_t rowPitch = 0;
        size_t slicePitch = 0;
        origin[0] = 0; origin[1] = 0; origin[2] = 0;
        region[0] = img_size.s[0]; region[1] = img_size.s[1]; region[2] = 1;

        char *p = new char[img_size.s[0] * img_size.s[1] * 4];

        queue.enqueueReadImage(output_image, CL_TRUE, origin, region, rowPitch, slicePitch, p);

        SaveImageAsBMP((unsigned int*)p, img_size.s[0], img_size.s[1], out_image);

        delete [] p;

        std::cout << "Done!" << std::endl;
    }
    catch (cl::Error err)
    {
       std::cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << std::endl;
       errCode = err.err();
    }

    return errCode;
}
