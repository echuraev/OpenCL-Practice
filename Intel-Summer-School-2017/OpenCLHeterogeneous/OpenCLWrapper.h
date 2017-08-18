#ifndef OPENCLWRAPPER_H
#define OPENCLWRAPPER_H

#define __CL_ENABLE_EXCEPTIONS
#include <CL/cl.hpp>
#include <vector>

enum class OpenCLPlatformType {Intel, AMD};
enum class OpenCLDeviceType {CPU, GPU, COMBO};

class OpenCLWrapper
{
public:
    OpenCLWrapper();
    virtual ~OpenCLWrapper() = default;
    void setPlatformAndDevice(OpenCLPlatformType, OpenCLDeviceType);
    void createContextAndQueue();
    void getProgramSourcesFromFile(std::string fileName);
    void getProgramSourcesFromString(std::string src);
    void buildProgram(std::string options = "");
    void createInputAndOutputImages(std::vector<unsigned char> &img, cl_int2 imgSize);
    void createKernel(std::string kernelName);
    /**
    Set ratio of calculating between CPU and GPU.
    Value should be in range between 0.1 and 0.9.
    near to 0 - more calculations are on CPU
    near to 1 - more calculations are on GPU

    @param ratio the value in range from 0.1 to 0.9.
    */
    inline void setRatio(cl_double ratio) { m_NDRangeRatio = (ratio >= 0.1 && ratio <= 0.9) ? ratio : 0.5; }
    void runKernel();
    inline std::vector<unsigned char> getResults() { return m_results; }
    void printTimes();
    inline std::string getPlatformName() { return m_platform.getInfo<CL_PLATFORM_NAME>(); }
    std::string getDeviceName();
protected:
    virtual cl::Platform getIntelOCLPlatform();
    virtual cl::Platform getATIOCLPlatform();
    bool isCPUDevicePresented();
    bool isGPUDevicePresented();
private:
    void runOnOneDevice();
    void runOnCombo();
    std::vector<unsigned char> splitImage(int xOffset, int yOffset, int width, int height);
    void glueImage(int xOffset, int yOffset, int width, int height, unsigned char *p);
    cl::Platform m_platform;
    std::vector<cl::Device> m_devices;
    cl::Context m_context;
    std::vector<cl::CommandQueue> m_queue;
    cl::Program::Sources m_source;
    std::string m_sourceStr;
    cl::Program m_program;
    std::vector<unsigned char> m_imgSource;
    cl_int2 m_imgSize;
    cl::Image2D m_inputImage;
    cl::Image2D m_outputImage;
    cl::Kernel m_kernel;
    std::vector<unsigned char> m_results;
    cl_device_type m_deviceType;
    cl_double m_NDRangeRatio;
    size_t m_xPieceSize;
    size_t m_yPieceSize;
    std::vector<cl::Event> m_kernelEvents;
    std::vector<cl_double> m_kernelNDRangeTimes;
    cl::Event m_writeEvent;
    cl_double m_writeTime;
    cl::Event m_readEvent;
    cl_double m_readTime;
};

#endif // OPENCLWRAPPER_H
