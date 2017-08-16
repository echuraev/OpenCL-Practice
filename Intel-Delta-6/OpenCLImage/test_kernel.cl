__constant sampler_t Sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_FILTER_NEAREST;

__kernel void colorToBW(__read_only image2d_t inImage, __write_only image2d_t outImage)
{
    // Gray = (R + G + B) / 3;
    int2 position = (int2)(get_global_id(0), get_global_id(1));
    float4 currentPixel = (float4)(0, 0, 0, 0);

    currentPixel = read_imagef(inImage, Sampler, position);
    float *rgba = (float*)&currentPixel;
    float gray = (rgba[0] + rgba[1] + rgba[2]) / 3;
    float4 bwPixel = (float4)(gray, gray, gray, 255);
    write_imagef(outImage, position, bwPixel);
}
