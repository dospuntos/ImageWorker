/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "StatusView.h"
#include "ImageView.h"

#include <Bitmap.h>
#include <stdio.h>
#include <InterfaceDefs.h>
#include <File.h>

StatusView::StatusView()
    : BStringView("status", "No image")
{
    SetAlignment(B_ALIGN_LEFT);
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void StatusView::Draw(BRect updateRect)
{
	BStringView::Draw(updateRect);

    // Draw top separator line
    SetHighColor(ui_color(B_WINDOW_BORDER_COLOR));

    StrokeLine(BPoint(Bounds().left, Bounds().top),
               BPoint(Bounds().right, Bounds().top));
}

void StatusView::Update(const entry_ref* ref,
                        const ImageView* imageView,
                        int32 index,
                        int32 total)
{
    if (!imageView) {
        SetText("No image");
        return;
    }

    BBitmap* bmp = imageView->Bitmap();
    if (!bmp) {
        SetText("No image");
        return;
    }

    int32 width = bmp->Bounds().IntegerWidth() + 1;
    int32 height = bmp->Bounds().IntegerHeight() + 1;

    const char* name = (ref && ref->name) ? ref->name : "(unnamed)";

	const char* mode = (imageView->getScaleMode() == SCALE_FIT)
    ? "Fit"
    : "";

    // index
    char indexBuf[32] = "";
    if (total > 0 && index >= 0) {
        snprintf(indexBuf, sizeof(indexBuf), "%d/%d  | ",
            index + 1, total);
    }

    // zoom
    float zoom = imageView->EffectiveZoom();
    char zoomBuf[32];
    snprintf(zoomBuf, sizeof(zoomBuf), "%.0f%%", zoom * 100.0f);

    // sizes
    char diskBuf[32], memBuf[32];
    _FormatSize(diskBuf, _GetFileSize(ref));
    _FormatSize(memBuf, _GetBitmapSize(bmp));

    char buffer[512];
    snprintf(buffer, sizeof(buffer),
    "%s%s  |  %d x %d  |  %.0f%% %s  |  %s/%s",
		indexBuf,
		name,
		width, height,
		zoom * 100.0f,
		mode,
		diskBuf, memBuf);

    SetText(buffer);
}


static off_t _GetFileSize(const entry_ref* ref)
{
    if (!ref)
        return 0;

    BFile file(ref, B_READ_ONLY);
    off_t size = 0;
    file.GetSize(&size);
    return size;
}


static size_t _GetBitmapSize(BBitmap* bmp)
{
    if (!bmp)
        return 0;

    return bmp->BitsLength();
}


static void _FormatSize(char* out, size_t size)
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
