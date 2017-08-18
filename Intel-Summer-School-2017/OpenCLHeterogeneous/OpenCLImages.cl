#include "colorenum.h"

__constant sampler_t Sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST;

__kernel void maskToImage(__read_only image2d_t inImage, __write_only image2d_t outImage, int color)
{
    // Gray = (R + G + B) / 3;
    int2 position = (int2)(get_global_id(0), get_global_id(1));
    float4 currentPixel = (float4)(0, 0, 0, 0);

    currentPixel = read_imagef(inImage, Sampler, position);
    float *rgba = (float*)&currentPixel;
    float gray = (rgba[0] + rgba[1] + rgba[2]) / 3;
    float4 bwPixel;
    if (color == BW)
        bwPixel = (float4)(gray, gray, gray, 255);
    else if (color == RED)
        bwPixel = (float4)(gray, gray, 255, 255);
    else
        bwPixel = (float4)(255, gray, gray, 255);
    write_imagef(outImage, position, bwPixel);
}
