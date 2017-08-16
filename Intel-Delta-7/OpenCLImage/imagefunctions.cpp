#include "imagefunctions.h"
#include <fstream>
#include <vector>
#include <cstring>

unsigned char *LoadImageAsBMP(const std::string& fileName, cl_int2 &size)
{
    const unsigned int bit32 = 32;
    unsigned int bytesPerPixel = 0;
    // read whole file
    std::ifstream image_src(fileName, std::ios_base::ate | std::ios_base::binary);
    if (!image_src.is_open())
        throw cl::Error(1, std::string("Could not open " + fileName + "!").c_str());

    std::streamoff file_length = image_src.tellg();
    if (file_length == -1)
        throw cl::Error(2, std::string("Cannot determine the length of file " + fileName).c_str());

    image_src.seekg(0, std::ios_base::beg);   // go to the file beginning
    std::vector<char> input_data;
    input_data.resize(static_cast<size_t>(file_length));
    image_src.read(&input_data[0], file_length);

    image_src.close();

    // offset of the start for data
    int data_offset = *(int*)&input_data[10];
    // get image size
    size = *(cl_int2*)&input_data[18];
    // get image bits per pixel
    cl_short bits = *(cl_short*)&input_data[28];

    // calc pixel_pitch as width for 32bit images
    int pixel_pitch = size.s[0];

    if (bits != bit32)
        throw cl::Error(3, std::string("Bad format of " + fileName + " BMP file. Only 32 bits per pixel is supported").c_str());

    bytesPerPixel = bits / 8;
    if (size.s[0] * size.s[1] * bytesPerPixel + data_offset > input_data.size())
        throw cl::Error(4, std::string("Could not read " + fileName + " BMP file.").c_str());

    // prepare storage for pixels
    unsigned char *p = new unsigned char[size.s[1] * pixel_pitch * bytesPerPixel];
    // convert BMP ABGR pixels into RGBA and
    // bottom-top format into top-bottom
    for (int y = 0; y<size.s[1]; ++y)
    {
        for (int x = 0; x < size.s[0]; ++x)
        {
            unsigned char* pSrc = (unsigned char*)&input_data[data_offset + (y * size.s[0] + x) * bytesPerPixel];
            unsigned char* pDst = p + bytesPerPixel * (y*pixel_pitch + x);
            pDst[0] = pSrc[1];
            pDst[1] = pSrc[2];
            pDst[2] = pSrc[3];
            pDst[3] = pSrc[0];
        }
    }
    return p;
};

void SaveImageAsBMP(unsigned int* ptr, int width, int height, std::string fileName)
{
    FILE* stream = nullptr;
    int* ppix = (int*)ptr;

    stream = fopen(fileName.c_str(), "wb");

    if (stream == nullptr)
        throw cl::Error(1, std::string("Cannot open " + fileName + " with writing access!").c_str());
    const unsigned int bytesPerPixel = 3;
    BITMAPFILEHEADER_OWN fileHeader;
    BITMAPINFOHEADER_OWN infoHeader;

    int alignSize = width * bytesPerPixel;
    alignSize ^= 0x03;
    alignSize++;
    alignSize &= 0x03;

    int rowLength = width * bytesPerPixel + alignSize;

    fileHeader.bfReserved1 = 0x0000;
    fileHeader.bfReserved2 = 0x0000;

    infoHeader.biSize = sizeof(BITMAPINFOHEADER_OWN);
    infoHeader.biWidth = width;
    infoHeader.biHeight = height;
    infoHeader.biPlanes = 1;
    infoHeader.biBitCount = 8 * bytesPerPixel;
    infoHeader.biCompression = 0L; // BI_RGB;
    infoHeader.biSizeImage = rowLength * height;
    infoHeader.biXPelsPerMeter = 0;
    infoHeader.biYPelsPerMeter = 0;
    infoHeader.biClrUsed = 0;
    infoHeader.biClrImportant = 0;
    fileHeader.bfType = 0x4D42;
    fileHeader.bfSize = sizeof(BITMAPFILEHEADER_OWN) + sizeof(BITMAPINFOHEADER_OWN) + rowLength * height;
    fileHeader.bfOffBits = sizeof(BITMAPFILEHEADER_OWN) + sizeof(BITMAPINFOHEADER_OWN);

    if (sizeof(BITMAPFILEHEADER_OWN) != fwrite(&fileHeader, 1, sizeof(BITMAPFILEHEADER_OWN), stream)) {
        fclose(stream);
        throw cl::Error(5, std::string("Cannot write BITMAPFILEHEADER!").c_str());
    }

    if (sizeof(BITMAPINFOHEADER_OWN) != fwrite(&infoHeader, 1, sizeof(BITMAPINFOHEADER_OWN), stream)) {
        fclose(stream);
        throw cl::Error(6, std::string("Cannot write BITMAPINFOHEADER_OWN!").c_str());
    }

    unsigned char buffer[bytesPerPixel];
    for (int y = 0; y < height; ++y)
    {
        for (int x = 0; x < width; ++x, ++ppix)
        {
            if (bytesPerPixel != fwrite(ppix, 1, bytesPerPixel, stream)) {
                fclose(stream);
                throw cl::Error(7, std::string("Cannot write pixel to file!").c_str());
            }
        }
        memset(buffer, 0x00, bytesPerPixel);
        fwrite(buffer, 1, alignSize, stream);
    }

    fclose(stream);
}
