/*
 * Copyright 2024, My Name
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ImageView.h"

ImageView::ImageView()
    : BView("image_view", B_WILL_DRAW),
      fBitmap(nullptr)
{
    //SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

ImageView::~ImageView()
{
    delete fBitmap;
}

void ImageView::SetBitmap(BBitmap* bitmap)
{
    delete fBitmap;
    fBitmap = bitmap;
    Invalidate();
}

void ImageView::Draw(BRect)
{
    if (fBitmap != nullptr) {
        DrawBitmap(fBitmap, Bounds());
    }
}

