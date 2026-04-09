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
#include <String.h>

StatusView::StatusView()
    : BStringView("status", "No image")
{
    SetAlignment(B_ALIGN_LEFT);
	//SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}


void StatusView::Draw(BRect updateRect)
{
	BStringView::Draw(updateRect);
}

void StatusView::SetStatus(BString status)
{
	SetText(status);
}

void StatusView::Update(const entry_ref* ref,
                        const ImageView* imageView,
                        int32 index,
                        int32 total)
{
    if (imageView == nullptr || imageView->Bitmap() == nullptr) {
        SetText("No image");
        return;
    }

    BBitmap* bmp = imageView->Bitmap();
    int32 width = bmp->Bounds().IntegerWidth() + 1;
    int32 height = bmp->Bounds().IntegerHeight() + 1;

    BString text;

    if (total > 0 && index >= 0)
        text.SetToFormat("%d/%d  |  ", index + 1, total);

    BString name = (ref != nullptr && ref->name != nullptr)
        ? ref->name : "(unnamed)";
    text << name << "  |  "
         << width << " x " << height << "  |  ";

    BString zoom;
	zoom.SetToFormat("%.0f%%", imageView->EffectiveZoom() * 100.0f);
	text << zoom;

    if (imageView->getScaleMode() == SCALE_FIT)
        text << " Fit";

    char diskBuf[32], memBuf[32];
    _FormatSize(diskBuf, _GetFileSize(ref));
    _FormatSize(memBuf, _GetBitmapSize(bmp));
    text << "  |  " << diskBuf << "/" << memBuf;

    SetText(text);
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
