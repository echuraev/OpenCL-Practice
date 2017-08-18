#include <iostream>
#include <string>
#include <chrono>
#include "imagefunctions.h"
#include "OpenCLWrapper.h"

const std::string in_image = "intel_orig.bmp";
const std::string out_image = "out_intel.bmp";

int main()
{
    int errCode = 0;

    OpenCLWrapper ocl;
    try
    {
        auto totalTimeStart = std::chrono::high_resolution_clock::now();
        // Get platform and get default device
        ocl.setPlatformAndDevice(OpenCLPlatformType::Intel, OpenCLDeviceType::COMBO);
        ocl.setRatio(0.86);
        std::cout << "Using platforms: " << ocl.getPlatformName() << std::endl;
        std::cout << "Using device: " << ocl.getDeviceName() << std::endl;

        // Create context and queue
        ocl.createContextAndQueue();

        // Read OpenCL source from file
        ocl.getProgramSourcesFromFile("OpenCLImages.cl");

        // Build program
        ocl.buildProgram();
        //ocl.buildProgram("-g -s OpenCLImages.cl");

        // Read image
        cl_int2 img_size;
        auto readImageTimeStart = std::chrono::high_resolution_clock::now();
        std::vector<unsigned char> img = LoadImageAsBMP(in_image, img_size);
        auto readImageTimeEnd = std::chrono::high_resolution_clock::now();
        std::cout << "Time of reading image: " << std::chrono::duration_cast<std::chrono::milliseconds>(readImageTimeEnd - readImageTimeStart).count() << " ms." << std::endl;

        // Create input and output images
        ocl.createInputAndOutputImages(img, img_size);

        // Create kernel
        ocl.createKernel("maskToImage");

        // Run OpenCL program
        ocl.runKernel();

        // Get results
        auto results = ocl.getResults();
        unsigned int *p = reinterpret_cast<unsigned int *>(&results[0]);

        ocl.printTimes();

        // Write output image
        auto writeImageTimeStart = std::chrono::high_resolution_clock::now();
        SaveImageAsBMP(p, img_size.s[0], img_size.s[1], out_image);
        auto writeImageTimeEnd = std::chrono::high_resolution_clock::now();
        std::cout << "Time of writing image: " << std::chrono::duration_cast<std::chrono::milliseconds>(writeImageTimeEnd - writeImageTimeStart).count() << " ms." << std::endl;

        auto totalTimeEnd = std::chrono::high_resolution_clock::now();
        std::cout << "Total time: " << std::chrono::duration_cast<std::chrono::milliseconds>(totalTimeEnd - totalTimeStart).count() << " ms." << std::endl;
        std::cout << "Done!" << std::endl;
    }
    catch (cl::Error err)
    {
       std::cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << std::endl;
       errCode = err.err();
    }
    system("pause");

    return errCode;
}
