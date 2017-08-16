#ifndef IMAGEFUNCTIONS_H
#define IMAGEFUNCTIONS_H

#define __CL_ENABLE_EXCEPTIONS

#include <CL/cl.hpp>
#include <string>

// Bitmap file headers and utilities
#pragma pack (push)
#pragma pack(1)
typedef struct  {
    unsigned short bfType;
    unsigned int   bfSize;
    unsigned short bfReserved1;
    unsigned short bfReserved2;
    unsigned int   bfOffBits;
} BITMAPFILEHEADER_OWN;

typedef struct  {
    unsigned int   biSize;
    int            biWidth;
    int            biHeight;
    unsigned short biPlanes;
    unsigned short biBitCount;
    unsigned int   biCompression;
    unsigned int   biSizeImage;
    int            biXPelsPerMeter;
    int            biYPelsPerMeter;
    unsigned int   biClrUsed;
    unsigned int   biClrImportant;
}  BITMAPINFOHEADER_OWN;
#pragma pack(pop)

unsigned char *LoadImageAsBMP(const std::string& fileName, cl_int2 &size);
void SaveImageAsBMP(unsigned int* ptr, int width, int height, std::string fileName);

#endif
