#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

#define BUFF_SIZE 10

int main()
{
    // Create and print arrays
    cl_int *a_arr = new cl_int[BUFF_SIZE];
    cl_int *b_arr = new cl_int[BUFF_SIZE];
    cl_int *c_arr = new cl_int[BUFF_SIZE];

    for (int i(0); i < BUFF_SIZE; ++i)
    {
        a_arr[i] = i;
        std::cout << a_arr[i] << ' ';
    }
    std::cout << "\n+\n";
    for (int i(0); i < BUFF_SIZE; ++i)
    {
        b_arr[i] = 10*i;
        std::cout << b_arr[i] << ' ';
    }
    std::cout << std::endl;

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
        std::ifstream kernel_src("OpenCLBuffers.cl");
        if (!kernel_src.is_open())
            throw cl::Error(1, "Cannot open file with kernel!");

        std::string str((std::istreambuf_iterator<char>(kernel_src)), std::istreambuf_iterator<char>());

        kernel_src.close();

        cl::Program::Sources sources;
        sources.push_back({str.c_str(), str.length()});

        // Create kernel
        cl::Program program(context, sources);
        program.build({device});
        //program.build({device}, "-g -s OpenCLBuffers.cl");

        // Create buffers
        cl::Buffer bufferA(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, BUFF_SIZE * sizeof(cl_int), a_arr);
        cl::Buffer bufferB(context, CL_MEM_READ_ONLY | CL_MEM_USE_HOST_PTR, BUFF_SIZE * sizeof(cl_int), b_arr);
        cl::Buffer bufferC(context, CL_MEM_WRITE_ONLY, BUFF_SIZE * sizeof(cl_int));
        //queue.enqueueWriteBuffer(bufferA, CL_TRUE, 0, BUFF_SIZE * sizeof(cl_int), a_arr);
        //queue.enqueueWriteBuffer(bufferB, CL_TRUE, 0, BUFF_SIZE * sizeof(cl_int), b_arr);

        cl::Kernel kernel = cl::Kernel(program, "vectorAdd");
        kernel.setArg(0, bufferA);
        kernel.setArg(1, bufferB);
        kernel.setArg(2, bufferC);
        queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(BUFF_SIZE), cl::NullRange);

        queue.enqueueReadBuffer(bufferC, CL_TRUE, 0, BUFF_SIZE * sizeof(cl_int), c_arr);

        std::cout << "=\n";

        for (int i(0); i < BUFF_SIZE; ++i)
        {
            std::cout << c_arr[i] << ' ';
        }
        std::cout << std::endl;
    }
    catch (cl::Error err)
    {
       std::cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << std::endl;
       errCode = err.err();
    }

    delete [] a_arr;
    delete [] b_arr;
    delete [] c_arr;

    return errCode;
}
