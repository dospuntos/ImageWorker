/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef UTILS_H
#define UTILS_H

#include <String.h>
#include <Bitmap.h>
#include <Catalog.h>
#include <File.h>
#include <cstdio>
#include <cmath>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Utilities"


static off_t GetFileSize(const entry_ref* ref)
{
    if (!ref)
        return 0;

    BFile file(ref, B_READ_ONLY);
    off_t size = 0;
    file.GetSize(&size);
    return size;
}


static size_t GetBitmapSize(BBitmap* bmp)
{
    if (!bmp)
        return 0;

    return bmp->BitsLength();
}


static void FormatSize(char* out, size_t size)
{
    const char* units[] = {	B_TRANSLATE_COMMENT("B", "Bytes"),
							B_TRANSLATE_COMMENT("KB", "Kilobytes"),
							B_TRANSLATE_COMMENT("MB", "Megabytes"),
							B_TRANSLATE_COMMENT("GB", "Gigabytes")};
    int i = 0;
    double s = (double)size;

    while (s > 1024 && i < 3) {
        s /= 1024;
        i++;
    }

    snprintf(out, 32, "%.1f%s", s, units[i]);
}


static BString FormatDimensions(int32 width, int32 height)
{
    BString out;

    float mp = (width * height) / 1000000.0f;

    // --- decimal aspect ratio ---
    float ratio = (float)width / (float)height;

    // --- optional: snap to common ratios ---
    struct Ratio { float value; const char* label; };
    static const Ratio common[] = {
        {16.0f/9.0f,  "16:9"},
        {3.0f/2.0f,   "3:2"},
        {4.0f/3.0f,   "4:3"},
        {1.0f,        "1:1"},
        {21.0f/9.0f,  "21:9"}
    };

    const char* ratioLabel = nullptr;

    for (const auto& r : common) {
        if (fabs(ratio - r.value) < 0.02f) { // tolerance
            ratioLabel = r.label;
            break;
        }
    }

    if (ratioLabel) {
        out.SetToFormat(B_TRANSLATE("%d x %d Pixels (%.2f MPixels) (%s)"),
            width, height, mp, ratioLabel);
    } else {
        out.SetToFormat(B_TRANSLATE("%d x %d Pixels (%.2f MPixels) (%.2f:1)"),
            width, height, mp, ratio);
    }

    return out;
}


static const char*
ColorSpaceToString(color_space cs)
{
    switch (cs) {
        case B_RGB32:        return B_TRANSLATE("RGB 32-bit");
        case B_RGBA32:       return B_TRANSLATE("RGBA 32-bit");
        case B_RGB24:        return B_TRANSLATE("RGB 24-bit");
        case B_RGB16:        return B_TRANSLATE("RGB 16-bit (5:6:5)");
        case B_RGB15:        return B_TRANSLATE("RGB 15-bit (5:5:5)");
        case B_RGBA15:       return B_TRANSLATE("RGBA 15-bit");
        case B_CMAP8:        return B_TRANSLATE("Indexed 8-bit");
        case B_GRAY8:        return B_TRANSLATE("Grayscale 8-bit");
        case B_GRAY1:        return B_TRANSLATE("Monochrome 1-bit");
        case B_YCbCr422:     return B_TRANSLATE("YCbCr 4:2:2");
        case B_YCbCr411:     return B_TRANSLATE("YCbCr 4:1:1");
        case B_YCbCr444:     return B_TRANSLATE("YCbCr 4:4:4");
        case B_YCbCr420:     return B_TRANSLATE("YCbCr 4:2:0");
        case B_YUV422:       return B_TRANSLATE("YUV 4:2:2");
        case B_YUV411:       return B_TRANSLATE("YUV 4:1:1");
        case B_YUV444:       return B_TRANSLATE("YUV 4:4:4");
        case B_YUV420:       return B_TRANSLATE("YUV 4:2:0");
        default:             return B_TRANSLATE("Unknown");
    }
}


#endif // UTILS_H
