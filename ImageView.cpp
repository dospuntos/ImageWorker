/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ImageView.h"
#include <Window.h>

ImageView::ImageView()
    : BView("image_view", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE),
      fBitmap(nullptr),
	  fScaleMode(SCALE_FIT)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
}

ImageView::~ImageView()
{
    delete fBitmap;
}

void
ImageView::SetBitmap(BBitmap* bitmap)
{
    delete fBitmap;
    fBitmap = bitmap;
    Invalidate();
}

void
ImageView::SetScaleMode(ScaleMode mode)
{
	fScaleMode = mode;
	Invalidate();
}

ScaleMode
ImageView::getScaleMode() const
{
	return fScaleMode;
}

void
ImageView::Draw(BRect)
{
    if (!fBitmap)
		return;

	BRect viewBounds = Bounds();
	BRect bitmapBounds = fBitmap->Bounds();

	float bitmapWidth = bitmapBounds.Width() + 1;
	float bitmapHeight = bitmapBounds.Height() + 1;

	float viewWidth = viewBounds.Width() + 1;
	float viewHeight = viewBounds.Height() + 1;

	BRect destRect;

	if (fScaleMode == SCALE_ORIGINAL) {
		// Center image, no scaling
		float x = (viewWidth - bitmapWidth) / 2;
		float y = (viewHeight - bitmapHeight) / 2;

		destRect = BRect(x, y, x + bitmapWidth - 1, y + bitmapHeight - 1);
	} else {
		// SCALE_FIT: preserve aspect ratio
        float scaleX = viewWidth / bitmapWidth;
        float scaleY = viewHeight / bitmapHeight;
        float scale = std::min(scaleX, scaleY);

        float drawWidth = bitmapWidth * scale;
        float drawHeight = bitmapHeight * scale;

        float x = (viewWidth - drawWidth) / 2;
        float y = (viewHeight - drawHeight) / 2;

        destRect = BRect(x, y, x + drawWidth - 1, y + drawHeight - 1);
	}

	SetDrawingMode(B_OP_ALPHA);
	SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

	DrawBitmap(fBitmap, bitmapBounds, destRect);
}


void
ImageView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);
	Invalidate();
}


void ImageView::KeyDown(const char* bytes, int32 numBytes)
{
    if (numBytes == 1) {
        switch (bytes[0]) {
            case B_ESCAPE:
                if (Window())
                    Window()->PostMessage(B_QUIT_REQUESTED);
                return;

            case 'f':
            case 'F':
                // toggle scale mode
                if (fScaleMode == SCALE_FIT)
                    SetScaleMode(SCALE_ORIGINAL);
                else
                    SetScaleMode(SCALE_FIT);
                return;
			case B_RIGHT_ARROW:
				Window()->PostMessage('next');
				return;

			case B_LEFT_ARROW:
				Window()->PostMessage('prev');
				return;
			case B_DELETE:
				if (Window())
					Window()->PostMessage('delI');
				return;
			case 'r':
			case 'R':
				Rotate90CW();
				return;

			case 'l':
			case 'L':
				Rotate90CCW();
				return;
        }
    }

    BView::KeyDown(bytes, numBytes);
}


void ImageView::Rotate90CW()
{
    if (!fBitmap)
        return;

    BRect srcBounds = fBitmap->Bounds();
    int32 srcW = srcBounds.IntegerWidth() + 1;
    int32 srcH = srcBounds.IntegerHeight() + 1;

    BRect dstBounds(0, 0, srcH - 1, srcW - 1);
    BBitmap* rotated = new BBitmap(dstBounds, fBitmap->ColorSpace());

    uint8* srcBits = (uint8*)fBitmap->Bits();
    uint8* dstBits = (uint8*)rotated->Bits();

    int32 srcBPR = fBitmap->BytesPerRow();
    int32 dstBPR = rotated->BytesPerRow();

    for (int32 y = 0; y < srcH; ++y) {
        for (int32 x = 0; x < srcW; ++x) {
            uint8* srcPixel = srcBits + y * srcBPR + x * 4;

            // (x, y) -> (h - 1 - y, x)
            int32 newX = srcH - 1 - y;
            int32 newY = x;

            uint8* dstPixel = dstBits + newY * dstBPR + newX * 4;

            *(uint32*)dstPixel = *(uint32*)srcPixel;
        }
    }

    SetBitmap(rotated);
}


void ImageView::Rotate90CCW()
{
    if (!fBitmap)
        return;

    BRect srcBounds = fBitmap->Bounds();
    int32 srcW = srcBounds.IntegerWidth() + 1;
    int32 srcH = srcBounds.IntegerHeight() + 1;

    BRect dstBounds(0, 0, srcH - 1, srcW - 1);
    BBitmap* rotated = new BBitmap(dstBounds, fBitmap->ColorSpace());

    uint8* srcBits = (uint8*)fBitmap->Bits();
    uint8* dstBits = (uint8*)rotated->Bits();

    int32 srcBPR = fBitmap->BytesPerRow();
    int32 dstBPR = rotated->BytesPerRow();

    for (int32 y = 0; y < srcH; ++y) {
        for (int32 x = 0; x < srcW; ++x) {
            uint8* srcPixel = srcBits + y * srcBPR + x * 4;

            // (x, y) -> (y, w - 1 - x)
            int32 newX = y;
            int32 newY = srcW - 1 - x;

            uint8* dstPixel = dstBits + newY * dstBPR + newX * 4;

            *(uint32*)dstPixel = *(uint32*)srcPixel;
        }
    }

    SetBitmap(rotated);
}