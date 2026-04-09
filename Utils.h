/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef UTILS_H
#define UTILS_H

#include <String.h>
#include <Bitmap.h>
#include <File.h>
#include <cstdio>
#include <cmath>


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
    const char* units[] = {"B", "KB", "MB", "GB"};
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
        out.SetToFormat("%d x %d Pixels (%.2f MPixels) (%s)",
            width, height, mp, ratioLabel);
    } else {
        out.SetToFormat("%d x %d Pixels (%.2f MPixels) (%.2f:1)",
            width, height, mp, ratio);
    }

    return out;
}



#endif // UTILS_H
