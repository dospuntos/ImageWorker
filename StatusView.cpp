/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "StatusView.h"
#include "ImageView.h"

#include <Bitmap.h>
#include <stdio.h>

StatusView::StatusView()
    : BStringView("status", "No image")
{
    SetAlignment(B_ALIGN_LEFT);
}

void StatusView::Update(const entry_ref* ref, const ImageView* imageView)
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
        : "1:1";

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s  |  %d x %d  |  %s",
        name, width, height, mode);

    SetText(buffer);
}

