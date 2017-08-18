#include "OpenCLWrapper.h"
#include "errorcodes.h"
#include "colorenum.h"
#include <algorithm>
#include <iostream>
#include <sstream>
#include <fstream>

OpenCLWrapper::OpenCLWrapper()
    : m_NDRangeRatio(0.5),
      m_writeTime(0),
      m_readTime(0)
{ }

void OpenCLWrapper::setPlatformAndDevice(OpenCLPlatformType platformType, OpenCLDeviceType deviceType)
{
    if (platformType == OpenCLPlatformType::Intel)
    {
        m_platform = getIntelOCLPlatform();
    }
    else if (platformType == OpenCLPlatformType::AMD)
    {
        m_platform = getATIOCLPlatform();
    }
    else
    {
        throw cl::Error(OCL_UNKNOWN_PLATFORM, "Error! Unknown platform!");
    }

    std::vector<cl::Device> devices;
    if (deviceType == OpenCLDeviceType::CPU)
    {
        m_deviceType = CL_DEVICE_TYPE_CPU;
        m_platform.getDevices(m_deviceType, &devices);
        m_devices.push_back(devices[0]);
        m_xPieceSize = m_devices[0].getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() / (sizeof(unsigned char) * 4); // 4 is the number of array elements for one color (rgba)
        m_yPieceSize = m_xPieceSize;
    }
    else if (deviceType == OpenCLDeviceType::GPU)
    {
        m_deviceType = CL_DEVICE_TYPE_GPU;
        m_platform.getDevices(m_deviceType, &devices);
        m_devices.push_back(devices[0]);
        m_xPieceSize = m_devices[0].getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() / (sizeof(unsigned char) * 4); // 4 is the number of array elements for one color (rgba)
        m_yPieceSize = m_xPieceSize;
    }
    else
    {
        m_deviceType = CL_DEVICE_TYPE_ALL;
        m_platform.getDevices(m_deviceType, &devices);
        m_xPieceSize = 0; m_yPieceSize = 0;
        for (auto &device : devices)
        {
            m_devices.push_back(device);
            auto maxAllocSize = device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() / (sizeof(unsigned char) * 4);
            if (m_xPieceSize == 0 || m_yPieceSize == 0)
            {
                m_xPieceSize = maxAllocSize;
                m_yPieceSize = m_xPieceSize;
            }
            else
            {
                m_xPieceSize = std::min(m_xPieceSize, maxAllocSize);
                m_yPieceSize = m_xPieceSize;
            }
        }
    }
}

std::string OpenCLWrapper::getDeviceName()
{
    std::ostringstream deviceNameStr;
    for (auto &device : m_devices)
    {
        deviceNameStr << device.getInfo<CL_DEVICE_NAME>() << "\n";
    }
    return deviceNameStr.str();
}

void OpenCLWrapper::createContextAndQueue()
{
    m_context = cl::Context(m_devices);
    if (m_deviceType != CL_DEVICE_TYPE_ALL)
    {
        m_queue.push_back(cl::CommandQueue(m_context, m_devices[0], CL_QUEUE_PROFILING_ENABLE));
    }
    else
    {
        cl::Device cpuDevice;
        cl::Device gpuDevice;
        for (auto & device : m_devices)
        {
            cl_device_type deviceType = device.getInfo<CL_DEVICE_TYPE>();
            if (deviceType == CL_DEVICE_TYPE_CPU)
                cpuDevice = device;
            else if (deviceType == CL_DEVICE_TYPE_GPU)
                gpuDevice = device;
        }
        m_queue.push_back(cl::CommandQueue(m_context, cpuDevice, CL_QUEUE_PROFILING_ENABLE));
        m_queue.push_back(cl::CommandQueue(m_context, gpuDevice, CL_QUEUE_PROFILING_ENABLE));
    }
}

void OpenCLWrapper::getProgramSourcesFromFile(std::string fileName)
{
    std::ifstream kernel_src(fileName);
    if (!kernel_src.is_open())
        throw cl::Error(CANNOT_OPEN_FILE, "Cannot open file with kernel!");

    std::string str((std::istreambuf_iterator<char>(kernel_src)), std::istreambuf_iterator<char>());

    kernel_src.close();

    getProgramSourcesFromString(str);
}

void OpenCLWrapper::getProgramSourcesFromString(std::string src)
{
    m_sourceStr = src;
    m_source.push_back({ m_sourceStr.c_str(), m_sourceStr.length() });
}

void OpenCLWrapper::buildProgram(std::string options)
{
    m_program = cl::Program(m_context, m_source);
    if (options.size() == 0)
        m_program.build(m_devices);
    else
        m_program.build(m_devices, options.c_str());
}

void OpenCLWrapper::createInputAndOutputImages(std::vector<unsigned char> &img, cl_int2 imgSize)
{
    m_imgSource = img;
    m_imgSize = imgSize;
    m_results.resize(m_imgSource.size());

    m_xPieceSize /= m_imgSize.y;
    m_yPieceSize /= m_imgSize.x;
}

void OpenCLWrapper::createKernel(std::string kernelName)
{
    m_kernel = cl::Kernel(m_program, kernelName.c_str());
}

void OpenCLWrapper::runKernel()
{
    if (m_deviceType != CL_DEVICE_TYPE_ALL)
    {
        runOnOneDevice();
    }
    else
    {
        runOnCombo();
    }
}

void OpenCLWrapper::printTimes()
{
    std::cout << "Time of writing image to device: " << m_writeTime << " ms." << std::endl;

    if (m_kernelNDRangeTimes.size() == 1)
    {
        std::cout << "Execution time: " << m_kernelNDRangeTimes[0] << " ms." << std::endl;
    }
    else
    {
        std::cout << "Execution CPU time: " << m_kernelNDRangeTimes[0] << " ms." << std::endl;
        std::cout << "Execution GPU time: " << m_kernelNDRangeTimes[1] << " ms." << std::endl;
    }

    std::cout << "Time of reading image from device: " << m_readTime << " ms." << std::endl;
}

cl::Platform OpenCLWrapper::getIntelOCLPlatform()
{
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    for (auto &platform : platforms)
    {
        auto platformName = platform.getInfo<CL_PLATFORM_NAME>();

        if (platformName.find("Intel(R) OpenCL") != std::string::npos)
            return platform;
    }

    return cl::Platform();
}

cl::Platform OpenCLWrapper::getATIOCLPlatform()
{
    std::vector<cl::Platform> platforms;
    cl::Platform::get(&platforms);
    for (auto &platform : platforms)
    {
        auto platformName = platform.getInfo<CL_PLATFORM_NAME>();

        if (platformName.find("ATI Stream") != std::string::npos || platformName.find("AMD Accelerated Parallel Processing") != std::string::npos)
            return platform;
    }

    return cl::Platform();
}

bool OpenCLWrapper::isCPUDevicePresented()
{
    std::vector<cl::Device> devices;
    m_platform.getDevices(CL_DEVICE_TYPE_CPU, &devices);
    return !devices.empty();
}

bool OpenCLWrapper::isGPUDevicePresented()
{
    std::vector<cl::Device> devices;
    m_platform.getDevices(CL_DEVICE_TYPE_GPU, &devices);
    return !devices.empty();
}

void OpenCLWrapper::runOnOneDevice()
{
    cl::size_t<3> origin;
    cl::size_t<3> region;
    size_t rowPitch = 0;
    size_t slicePitch = 0;
    int xNumberOfPieces = m_imgSize.x / m_xPieceSize;
    int yNumberOfPieces = m_imgSize.y / m_yPieceSize;
    xNumberOfPieces = (m_imgSize.x % m_xPieceSize) ? xNumberOfPieces + 1 : xNumberOfPieces;
    yNumberOfPieces = (m_imgSize.y % m_yPieceSize) ? yNumberOfPieces + 1 : yNumberOfPieces;
    m_kernelEvents.resize(1);
    m_kernelNDRangeTimes.resize(1, 0);
    origin[0] = 0; origin[1] = 0; origin[2] = 0;
    cl::ImageFormat format(CL_RGBA, CL_UNORM_INT8); // structure to define image description

    for (int x = 0; x < xNumberOfPieces; ++x)
    {
        auto width = m_xPieceSize; auto height = m_yPieceSize;
        for (int y = 0; y < yNumberOfPieces; ++y)
        {
            auto xOffset = x*m_xPieceSize;
            auto yOffset = y*m_yPieceSize;
            width = ((xOffset + width) > m_imgSize.x) ? (m_imgSize.x - xOffset) : width;
            height = ((yOffset + height) > m_imgSize.y) ? (m_imgSize.y - yOffset) : height;
            region[0] = width; region[1] = height; region[2] = 1;

            auto imgPiece = splitImage(xOffset, yOffset, width, height);
            // Allocate input_image
            m_inputImage = cl::Image2D(m_context, CL_MEM_READ_ONLY, format, width, height);
            // Allocate non-initialized output buffer
            m_outputImage = cl::Image2D(m_context, CL_MEM_WRITE_ONLY, format, width, height);

            m_queue[0].enqueueWriteImage(m_inputImage, CL_TRUE, origin, region, rowPitch, slicePitch, &imgPiece[0], nullptr, &m_writeEvent);
            m_writeEvent.wait();
            auto startTime = m_writeEvent.getProfilingInfo<CL_PROFILING_COMMAND_START>();
            auto endTime = m_writeEvent.getProfilingInfo<CL_PROFILING_COMMAND_END>();
            m_writeTime += (cl_double)(endTime - startTime)*(cl_double)(1e-06); // From nano seconds to milli

            m_kernel.setArg(0, m_inputImage);
            m_kernel.setArg(1, m_outputImage);
            m_kernel.setArg(2, BW);
            m_queue[0].enqueueNDRangeKernel(m_kernel, cl::NullRange, cl::NDRange(width, height), cl::NullRange, nullptr, &m_kernelEvents[0]);
            m_queue[0].finish();

            cl::Event::waitForEvents(m_kernelEvents);
            startTime = m_kernelEvents[0].getProfilingInfo<CL_PROFILING_COMMAND_START>();
            endTime = m_kernelEvents[0].getProfilingInfo<CL_PROFILING_COMMAND_END>();
            m_kernelNDRangeTimes[0] += (cl_double)(endTime - startTime)*(cl_double)(1e-06); // From nano seconds to milli

            size_t resultsSize = width * height * 4;
            unsigned char *p = new unsigned char[resultsSize];

            m_queue[0].enqueueReadImage(m_outputImage, CL_TRUE, origin, region, rowPitch, slicePitch, p, nullptr, &m_readEvent);
            m_readEvent.wait();
            startTime = m_readEvent.getProfilingInfo<CL_PROFILING_COMMAND_START>();
            endTime = m_readEvent.getProfilingInfo<CL_PROFILING_COMMAND_END>();
            m_readTime += (cl_double)(endTime - startTime)*(cl_double)(1e-06); // From nano seconds to milli
            glueImage(xOffset, yOffset, width, height, p);

            delete [] p;
        }
    }
}

void OpenCLWrapper::runOnCombo()
{
    cl::size_t<3> origin;
    cl::size_t<3> region;
    size_t rowPitch = 0;
    size_t slicePitch = 0;
    int xNumberOfPieces = m_imgSize.x / m_xPieceSize;
    int yNumberOfPieces = m_imgSize.y / m_yPieceSize;
    xNumberOfPieces = (m_imgSize.x % m_xPieceSize) ? xNumberOfPieces + 1 : xNumberOfPieces;
    yNumberOfPieces = (m_imgSize.y % m_yPieceSize) ? yNumberOfPieces + 1 : yNumberOfPieces;
    m_kernelEvents.resize(2);
    m_kernelNDRangeTimes.resize(2, 0);
    origin[0] = 0; origin[1] = 0; origin[2] = 0;
    cl::ImageFormat format(CL_RGBA, CL_UNORM_INT8); // structure to define image description

    for (int x = 0; x < xNumberOfPieces; ++x)
    {
        auto width = m_xPieceSize; auto height = m_yPieceSize;
        for (int y = 0; y < yNumberOfPieces; ++y)
        {
            auto xOffset = x*m_xPieceSize;
            auto yOffset = y*m_yPieceSize;
            width = ((xOffset + width) > m_imgSize.x) ? (m_imgSize.x - xOffset) : width;
            height = ((yOffset + height) > m_imgSize.y) ? (m_imgSize.y - yOffset) : height;

            // CPU
            region[0] = width; region[1] = height; region[2] = 1;

            auto imgPiece = splitImage(xOffset, yOffset, width, height);
            // Allocate input_image
            m_inputImage = cl::Image2D(m_context, CL_MEM_READ_ONLY, format, width, height);
            // Allocate non-initialized output buffer
            m_outputImage = cl::Image2D(m_context, CL_MEM_WRITE_ONLY, format, width, height);
            m_queue[0].enqueueWriteImage(m_inputImage, CL_TRUE, origin, region, rowPitch, slicePitch, &imgPiece[0], nullptr, &m_writeEvent);
            m_writeEvent.wait();
            auto startTime = m_writeEvent.getProfilingInfo<CL_PROFILING_COMMAND_START>();
            auto endTime = m_writeEvent.getProfilingInfo<CL_PROFILING_COMMAND_END>();
            m_writeTime += (cl_double)(endTime - startTime)*(cl_double)(1e-06); // From nano seconds to milli

            m_kernel.setArg(0, m_inputImage);
            m_kernel.setArg(1, m_outputImage);
            m_kernel.setArg(2, BLUE);
            auto ySize = height * (1 - m_NDRangeRatio);
            m_queue[0].enqueueNDRangeKernel(m_kernel, cl::NullRange, cl::NDRange(width, ySize), cl::NullRange, nullptr, &m_kernelEvents[0]);

            // GPU
            m_kernel.setArg(0, m_inputImage);
            m_kernel.setArg(1, m_outputImage);
            m_kernel.setArg(2, RED);
            auto gpuYOffset = ySize;
            ySize = height * m_NDRangeRatio;
            m_queue[1].enqueueNDRangeKernel(m_kernel, cl::NDRange(0, gpuYOffset), cl::NDRange(width, ySize), cl::NullRange, nullptr, &m_kernelEvents[1]);
            m_queue[0].finish();
            m_queue[1].finish();

            cl::Event::waitForEvents(m_kernelEvents);
            startTime = m_kernelEvents[0].getProfilingInfo<CL_PROFILING_COMMAND_START>();
            endTime = m_kernelEvents[0].getProfilingInfo<CL_PROFILING_COMMAND_END>();
            m_kernelNDRangeTimes[0] += (cl_double)(endTime - startTime)*(cl_double)(1e-06); // From nano seconds to milli

            startTime = m_kernelEvents[1].getProfilingInfo<CL_PROFILING_COMMAND_START>();
            endTime = m_kernelEvents[1].getProfilingInfo<CL_PROFILING_COMMAND_END>();
            m_kernelNDRangeTimes[1] += (cl_double)(endTime - startTime)*(cl_double)(1e-06); // From nano seconds to milli

            size_t resultsSize = width * height * 4;
            unsigned char *p = new unsigned char[resultsSize];

            m_queue[0].enqueueReadImage(m_outputImage, CL_TRUE, origin, region, rowPitch, slicePitch, p, nullptr, &m_readEvent);
            m_readEvent.wait();
            startTime = m_readEvent.getProfilingInfo<CL_PROFILING_COMMAND_START>();
            endTime = m_readEvent.getProfilingInfo<CL_PROFILING_COMMAND_END>();
            m_readTime += (cl_double)(endTime - startTime)*(cl_double)(1e-06); // From nano seconds to milli
            glueImage(xOffset, yOffset, width, height, p);

            delete [] p;
        }
    }
}

std::vector<unsigned char> OpenCLWrapper::splitImage(int xOffset, int yOffset, int width, int height)
{
    std::vector<unsigned char> img;
    img.resize(width * height * 4 * sizeof(unsigned char));
    for (int row = 0; row < height; ++row)
    {
        auto destIndex = row * width * 4;
        auto srcIndex = xOffset * 4 + (yOffset + row) * m_imgSize.x * 4;
        auto count = width * 4;
        memcpy(&img[destIndex], &m_imgSource[srcIndex], count);
    }
    return img;
}

void OpenCLWrapper::glueImage(int xOffset, int yOffset, int width, int height, unsigned char *p)
{
    for (int row = 0; row < height; ++row)
    {
        auto destIndex = (yOffset + row) * m_imgSize.x * 4 + xOffset * 4;
        auto srcIndex = row * width * 4;
        auto count = width * 4;
        memcpy(&m_results[destIndex], &p[srcIndex], count);
    }
}
