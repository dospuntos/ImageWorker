/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "StatusView.h"
#include "ImageView.h"

#include <Bitmap.h>
#include <stdio.h>
#include <InterfaceDefs.h>

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
        : "1:1";

    char indexBuffer[32] = "";
    if (total > 0 && index >= 0) {
        snprintf(indexBuffer, sizeof(indexBuffer), "%d/%d  |  ",
            index + 1, total);
    }

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "%s%s  |  %d x %d  |  %s",
        indexBuffer, name, width, height, mode);

    SetText(buffer);
}

