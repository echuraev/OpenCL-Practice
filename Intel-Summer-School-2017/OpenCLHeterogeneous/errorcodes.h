#ifndef ERRORCODES_H
#define ERRORCODES_H

enum ErrorCodes
{
    /* image functions errors */
    CANNOT_OPEN_FILE             = 1,
    INV_FILE_LENGTH              = 2,
    BAD_FORMAT_BMP               = 3,
    CANNOT_READ_BMP              = 4,
    BITMAPFILEHEADER_WRITE_ERROR = 5,
    BITMAPINFOHEADER_WRITE_ERROR = 6,
    CANNOT_WRITE_PIXEL_TO_FILE   = 7,
    /* OpenCLWrapper errors */
    OCL_UNKNOWN_PLATFORM         = -1,
};

#endif
