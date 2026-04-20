/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "ImageView.h"
#include "Constants.h"
#include <Window.h>
#include <Cursor.h>
#include <cstdio>

ImageView::ImageView()
    : BView("image_view", B_WILL_DRAW | B_FULL_UPDATE_ON_RESIZE | B_NAVIGABLE),
      fBitmap(nullptr),
	  fScaleMode(SCALE_FIT_WINDOW),
	  fZoom(1.0f),
	  fOffset(BPoint(0,0)),
	  fDragging(false)
{
    SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	float fZoom = 1.0f;
	int32 fHistoryIndex = -1;
	int32 fMaxHistorySteps = 10;
	SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
}

ImageView::~ImageView()
{
    delete fBitmap;
}

void ImageView::MessageReceived(BMessage* message)
{
    if (message->what == B_MOUSE_WHEEL_CHANGED) {
        float x, y;
        if (message->FindFloat("be:wheel_delta_x", &x) == B_OK &&
            message->FindFloat("be:wheel_delta_y", &y) == B_OK) {

            BPoint where;
            GetMouse(&where, nullptr);

            MouseWheelChanged(where, x, y);
            return;
        }
    }

    BView::MessageReceived(message);
}


void
ImageView::SetBitmap(BBitmap* bitmap)
{
    delete fBitmap;
    fBitmap = bitmap;
	fOffset = BPoint(0,0);
	fZoom = 1.0f;

    Invalidate();
}

void
ImageView::SetScaleMode(ScaleMode mode)
{
	fScaleMode = mode;
	if (mode == SCALE_FIT_WINDOW)
        fOffset = BPoint(0, 0);
	_ClampOffset();

	if (Window())
		Window()->PostMessage('stat');

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
    // Clear background
    SetHighColor(ViewColor());
    FillRect(Bounds());

    if (!fBitmap)
        return;

    BRect viewBounds = Bounds();
    BRect bitmapBounds = fBitmap->Bounds();

    float bitmapWidth = bitmapBounds.Width() + 1;
    float bitmapHeight = bitmapBounds.Height() + 1;

    float viewWidth = viewBounds.Width() + 1;
    float viewHeight = viewBounds.Height() + 1;

    // --- unified zoom ---
    float scaleX = viewWidth / bitmapWidth;
	float scaleY = viewHeight / bitmapHeight;

	float zoom = fZoom;

	switch (fScaleMode) {

		case SCALE_FIT_WINDOW:
			zoom = std::min(scaleX, scaleY);
			break;

		case SCALE_FIT_WIDTH:
			zoom = scaleX;
			break;

		case SCALE_FIT_HEIGHT:
			zoom = scaleY;
			break;

		case SCALE_FIT_LARGE_ONLY:
			if (bitmapWidth > viewWidth || bitmapHeight > viewHeight)
				zoom = std::min(scaleX, scaleY);
			else
				zoom = 1.0f;
			break;
		/*
		case SCALE_FIT_SMALL_ONLY:
			if (bitmapWidth < viewWidth && bitmapHeight < viewHeight)
				zoom = std::min(scaleX, scaleY);
			else
				zoom = 1.0f;
			break;


		case SCALE_FILL_WINDOW:
			// fills entire view, may crop
			zoom = std::max(scaleX, scaleY);
			break;
*/
		case SCALE_ORIGINAL:
		default:
			zoom = fZoom;
			break;
	}

    float drawWidth = bitmapWidth * zoom;
    float drawHeight = bitmapHeight * zoom;

    // --- ALWAYS apply offset ---
    float x = (viewWidth - drawWidth) / 2 + fOffset.x;
    float y = (viewHeight - drawHeight) / 2 + fOffset.y;

    BRect destRect(x, y, x + drawWidth - 1, y + drawHeight - 1);

    SetDrawingMode(B_OP_ALPHA);
    SetBlendingMode(B_PIXEL_ALPHA, B_ALPHA_OVERLAY);

    DrawBitmap(fBitmap, bitmapBounds, destRect);
}


void
ImageView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);

	_ClampOffset();
	Invalidate(Bounds());

    if (Window())
		Window()->PostMessage('stat'); // update status
}


void ImageView::KeyDown(const char* bytes, int32 numBytes)
{

	uint32 mods = modifiers();

    if (mods & B_CONTROL_KEY) {
        switch (bytes[0]) {
            case B_HOME:
                Window()->PostMessage(M_FIRST_IMAGE);
                return;

            case B_END:
                Window()->PostMessage(M_LAST_IMAGE);
                return;
        }
    }

    if (numBytes == 1) {
        switch (bytes[0]) {
            case B_ESCAPE:
				if (Window())
					Window()->PostMessage(B_QUIT_REQUESTED);
                return;

			case 'o':
			case 'O':
				if (Window())
					Window()->PostMessage(M_OPEN_FILE);
				return;
            case 'f':
            case 'F':
                {
				switch (fScaleMode) {
					case SCALE_FIT_WINDOW:   SetScaleMode(SCALE_FIT_WIDTH); break;
					case SCALE_FIT_WIDTH:    SetScaleMode(SCALE_FIT_HEIGHT); break;
					case SCALE_FIT_HEIGHT:   SetScaleMode(SCALE_ORIGINAL); break;
					case SCALE_ORIGINAL:     SetScaleMode(SCALE_FIT_LARGE_ONLY); break;
					default:                SetScaleMode(SCALE_FIT_WINDOW); break;
				}
			}
                return;
			case B_RIGHT_ARROW:
			case B_SPACE:
				if (Window())
					Window()->PostMessage('next');
				return;
			case B_LEFT_ARROW:
			case B_BACKSPACE:
				if (Window())
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
			case '+':
			case '=':
				ZoomIn();
				return;

			case '-':
				ZoomOut();
				return;

			case 'd':
			case 'D':
				if (Window())
					Window()->PostMessage(M_CLEAR_IMAGE);
				return;

			case 'h':
			case 'H':
				FlipHorizontal();
				return;

			case 'v':
			case 'V':
				FlipVertical();
				return;

			case 'i':
			case 'I':
				if (Window())
					Window()->PostMessage(M_SHOW_INFO);
				return;

			case 'p':
			case 'P':
				if (Window())
					Window()->PostMessage(M_SHOW_SETTINGS);
				return;
        }
    }

    BView::KeyDown(bytes, numBytes);
}


void ImageView::Clear()
{
	if (fBitmap != nullptr) {
		delete fBitmap;
		fBitmap = nullptr;

		fOffset = BPoint(0, 0);
		fZoom = 1.0f;
		fScaleMode = SCALE_FIT_WINDOW;

		ClearHistory();
		Invalidate();
	}
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
	SaveState();
	if (Window())
		Window()->PostMessage('stat');
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
	SaveState();
	if (Window())
		Window()->PostMessage('stat');
}


void ImageView::FlipHorizontal()
{
    if (!fBitmap)
        return;

    int32 width  = fBitmap->Bounds().IntegerWidth() + 1;
    int32 height = fBitmap->Bounds().IntegerHeight() + 1;

    uint8* bits = (uint8*)fBitmap->Bits();
    int32 bpr = fBitmap->BytesPerRow();

    for (int32 y = 0; y < height; ++y) {
        uint32* row = (uint32*)(bits + y * bpr);

        for (int32 x = 0; x < width / 2; ++x) {
            uint32 tmp = row[x];
            row[x] = row[width - 1 - x];
            row[width - 1 - x] = tmp;
        }
    }

	SaveState();
    Invalidate();
    if (Window())
        Window()->PostMessage('stat');
}


void ImageView::FlipVertical()
{
    if (!fBitmap)
        return;

    int32 width  = fBitmap->Bounds().IntegerWidth() + 1;
    int32 height = fBitmap->Bounds().IntegerHeight() + 1;

    uint8* bits = (uint8*)fBitmap->Bits();
    int32 bpr = fBitmap->BytesPerRow();

    for (int32 y = 0; y < height / 2; ++y) {
        uint8* rowTop = bits + y * bpr;
        uint8* rowBottom = bits + (height - 1 - y) * bpr;

        for (int32 x = 0; x < width * 4; ++x) {
            uint8 tmp = rowTop[x];
            rowTop[x] = rowBottom[x];
            rowBottom[x] = tmp;
        }
    }

	SaveState();
    Invalidate();
    if (Window())
        Window()->PostMessage('stat');
}


void ImageView::ConvertToGrayscale()
{
    if (!fBitmap)
        return;

    if (fBitmap->ColorSpace() != B_RGBA32 &&
        fBitmap->ColorSpace() != B_RGB32)
        return;

    int32 width  = fBitmap->Bounds().IntegerWidth() + 1;
    int32 height = fBitmap->Bounds().IntegerHeight() + 1;

    uint8* bits = (uint8*)fBitmap->Bits();
    int32 bpr = fBitmap->BytesPerRow();

    for (int32 y = 0; y < height; ++y) {
        uint8* row = bits + y * bpr;

        for (int32 x = 0; x < width; ++x) {
            uint8* pixel = row + x * 4;

            uint8 b = pixel[0];
            uint8 g = pixel[1];
            uint8 r = pixel[2];
            // pixel[3] = alpha (leave unchanged)

            // Standard luminance formula
            uint8 gray = (uint8)(0.299f * r + 0.587f * g + 0.114f * b);

            pixel[0] = gray; // B
            pixel[1] = gray; // G
            pixel[2] = gray; // R
        }
    }

	SaveState();
    Invalidate();
    if (Window())
        Window()->PostMessage('stat');
}


void ImageView::SwapColors(const int order[4])
{
    if (!fBitmap)
        return;

    if (fBitmap->ColorSpace() != B_RGBA32 &&
        fBitmap->ColorSpace() != B_RGB32)
        return;

    int32 width  = fBitmap->Bounds().IntegerWidth() + 1;
    int32 height = fBitmap->Bounds().IntegerHeight() + 1;

    uint8* bits = (uint8*)fBitmap->Bits();
    int32 bpr = fBitmap->BytesPerRow();

    for (int32 y = 0; y < height; ++y) {
        uint8* row = bits + y * bpr;

        for (int32 x = 0; x < width; ++x) {
            uint8* pixel = row + x * 4;

            uint8 original[4];
            original[0] = pixel[0]; // B
            original[1] = pixel[1]; // G
            original[2] = pixel[2]; // R
            original[3] = pixel[3]; // A

            pixel[0] = original[order[0]];
            pixel[1] = original[order[1]];
            pixel[2] = original[order[2]];
            pixel[3] = original[order[3]]; // usually keep alpha
        }
    }

	SaveState();
    Invalidate();
    if (Window())
        Window()->PostMessage('stat');
}


void ImageView::InvertColors()
{
    if (!fBitmap)
        return;

    if (fBitmap->ColorSpace() != B_RGBA32 &&
        fBitmap->ColorSpace() != B_RGB32)
        return;

    int32 width  = fBitmap->Bounds().IntegerWidth() + 1;
    int32 height = fBitmap->Bounds().IntegerHeight() + 1;

    uint8* bits = (uint8*)fBitmap->Bits();
    int32 bpr = fBitmap->BytesPerRow();

    for (int32 y = 0; y < height; ++y) {
        uint8* row = bits + y * bpr;

        for (int32 x = 0; x < width; ++x) {
            uint8* pixel = row + x * 4;

            // pixel layout: B, G, R, A
            pixel[0] = 255 - pixel[0]; // B
            pixel[1] = 255 - pixel[1]; // G
            pixel[2] = 255 - pixel[2]; // R
            // pixel[3] = alpha (unchanged)
        }
    }

	SaveState();
    Invalidate();
    if (Window())
        Window()->PostMessage('stat');
}


void ImageView::IsolateChannel(ColorChannel channel, bool replicateToAll)
{
    if (!fBitmap)
        return;

    if (fBitmap->ColorSpace() != B_RGBA32 &&
        fBitmap->ColorSpace() != B_RGB32)
        return;

    int32 width  = fBitmap->Bounds().IntegerWidth() + 1;
    int32 height = fBitmap->Bounds().IntegerHeight() + 1;

    uint8* bits = (uint8*)fBitmap->Bits();
    int32 bpr = fBitmap->BytesPerRow();

    // internal format: [0]=B, [1]=G, [2]=R, [3]=A
    int index = 0;
    switch (channel) {
        case CHANNEL_BLUE:  index = 0; break;
        case CHANNEL_GREEN: index = 1; break;
        case CHANNEL_RED:   index = 2; break;
    }

    for (int32 y = 0; y < height; ++y) {
        uint8* row = bits + y * bpr;

        for (int32 x = 0; x < width; ++x) {
            uint8* pixel = row + x * 4;

            uint8 value = pixel[index];

            if (replicateToAll) {
                // grayscale-like: show intensity of the channel
                pixel[0] = value; // B
                pixel[1] = value; // G
                pixel[2] = value; // R
            } else {
                // keep only that channel
                pixel[0] = (index == 0) ? value : 0;
                pixel[1] = (index == 1) ? value : 0;
                pixel[2] = (index == 2) ? value : 0;
            }
        }
    }

	SaveState();
    Invalidate();
    if (Window())
        Window()->PostMessage('stat');
}



float ImageView::Zoom() const
{
    return fZoom;
}

void ImageView::SetZoom(float zoom)
{
    if (zoom < 0.05f) zoom = 0.05f;
    if (zoom > 10.0f) zoom = 10.0f;

    fZoom = zoom;
    fScaleMode = SCALE_ORIGINAL; // zoom overrides fit
    Invalidate();
	if (Window())
		Window()->PostMessage('stat');
}

void ImageView::ZoomIn()
{
    SetZoom(fZoom * 1.05f);
	if (Window())
		Window()->PostMessage('stat');
}

void ImageView::ZoomOut()
{
    SetZoom(fZoom / 1.05f);
	if (Window())
		Window()->PostMessage('stat');
}

float ImageView::EffectiveZoom() const
{
    if (!fBitmap)
        return 1.0f;

    BRect viewBounds = Bounds();
    BRect bitmapBounds = fBitmap->Bounds();

    float bitmapWidth = bitmapBounds.Width() + 1;
    float bitmapHeight = bitmapBounds.Height() + 1;

    float viewWidth = viewBounds.Width() + 1;
    float viewHeight = viewBounds.Height() + 1;

    if (fScaleMode == SCALE_FIT_WINDOW) {
        float scaleX = viewWidth / bitmapWidth;
        float scaleY = viewHeight / bitmapHeight;
        return std::min(scaleX, scaleY);
    }

    return fZoom;
}


void ImageView::MouseWheelChanged(BPoint where, float, float y)
{
    if (!fBitmap)
        return;

    float oldZoom = EffectiveZoom();

    // Zoom factor
    float factor = (y < 0) ? 1.25f : 0.8f;

    // Switch from FIT → zoom mode
    if (fScaleMode == SCALE_FIT_WINDOW) {
        fZoom = oldZoom;
        fScaleMode = SCALE_ORIGINAL;
    }

    float newZoom = fZoom * factor;
    if (newZoom < 0.05f) newZoom = 0.05f;
    if (newZoom > 20.0f) newZoom = 20.0f;

    // --- STEP 1: find top-left of image BEFORE zoom ---
    BRect bounds = Bounds();

    float viewW = bounds.Width() + 1;
    float viewH = bounds.Height() + 1;

    float bmpW = fBitmap->Bounds().Width() + 1;
    float bmpH = fBitmap->Bounds().Height() + 1;

    float drawW = bmpW * oldZoom;
    float drawH = bmpH * oldZoom;

    float left = (viewW - drawW) / 2 + fOffset.x;
    float top  = (viewH - drawH) / 2 + fOffset.y;

    // --- STEP 2: get cursor position in image space ---
    float imgX = (where.x - left) / oldZoom;
    float imgY = (where.y - top) / oldZoom;

    // --- STEP 3: apply zoom ---
    fZoom = newZoom;

    float newDrawW = bmpW * fZoom;
    float newDrawH = bmpH * fZoom;

    // --- STEP 4: compute new top-left so cursor stays fixed ---
    float newLeft = where.x - imgX * fZoom;
    float newTop  = where.y - imgY * fZoom;

    // --- STEP 5: convert back to offset ---
    fOffset.x = newLeft - (viewW - newDrawW) / 2;
    fOffset.y = newTop  - (viewH - newDrawH) / 2;

	_ClampOffset();
    Invalidate();
}


void ImageView::MouseDown(BPoint where)
{
	uint32 mouseButtonStates = 0;
	if (Window()->CurrentMessage() != NULL)
		mouseButtonStates = Window()->CurrentMessage()->FindInt32("buttons");

	if (mouseButtonStates & B_SECONDARY_MOUSE_BUTTON) {
		// Right click
		fDragging = true;
		fLastMouse = where;

		SetMouseEventMask(B_POINTER_EVENTS, B_LOCK_WINDOW_FOCUS);
	}
}

void ImageView::MouseUp(BPoint)
{
	fDragging = false;
}

void ImageView::MouseMoved(BPoint where, uint32, const BMessage*)
{
    if (!fDragging)
        return;

    float dx = where.x - fLastMouse.x;
    float dy = where.y - fLastMouse.y;

    switch (fScaleMode) {

        case SCALE_ORIGINAL:
            // Free panning
            fOffset.x += dx;
            fOffset.y += dy;
            break;

        case SCALE_FIT_WIDTH:
            // Width is fixed → only vertical panning
            fOffset.y += dy;
            break;

        case SCALE_FIT_HEIGHT:
            // Height is fixed → only horizontal panning
            fOffset.x += dx;
            break;

        case SCALE_FIT_WINDOW:
        case SCALE_FIT_LARGE_ONLY:
            // Fully constrained → no panning
            return;

/*        case SCALE_FILL_WINDOW:
            // Optional: allow both directions (image is cropped)
            fOffset.x += dx;
            fOffset.y += dy;
            break; */

        default:
            break;
    }

    _ClampOffset();
    fLastMouse = where;
    Invalidate();
}


static float _Clamp(float v, float min, float max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}


void ImageView::_ClampOffset()
{
    if (!fBitmap)
        return;

    BRect bounds = Bounds();

    float viewW = bounds.Width() + 1;
    float viewH = bounds.Height() + 1;

    float bmpW = fBitmap->Bounds().Width() + 1;
    float bmpH = fBitmap->Bounds().Height() + 1;

    float scaleX = viewW / bmpW;
    float scaleY = viewH / bmpH;

    float zoom;

    switch (fScaleMode) {

        case SCALE_FIT_WINDOW:
            zoom = std::min(scaleX, scaleY);
            break;

        case SCALE_FIT_WIDTH:
            zoom = scaleX;
            break;

        case SCALE_FIT_HEIGHT:
            zoom = scaleY;
            break;

        case SCALE_FIT_LARGE_ONLY:
            if (bmpW > viewW || bmpH > viewH)
                zoom = std::min(scaleX, scaleY);
            else
                zoom = 1.0f;
            break;

		/*
        case SCALE_FIT_SMALL_ONLY:
            if (bmpW < viewW && bmpH < viewH)
                zoom = std::min(scaleX, scaleY);
            else
                zoom = 1.0f;
            break;

        case SCALE_FILL_WINDOW:
            zoom = std::max(scaleX, scaleY);
            break;
			*/

        case SCALE_ORIGINAL:
        default:
            zoom = fZoom;
            break;
    }

    float drawW = bmpW * zoom;
    float drawH = bmpH * zoom;

    float maxOffsetX = std::max(0.0f, (drawW - viewW) / 2.0f);
    float maxOffsetY = std::max(0.0f, (drawH - viewH) / 2.0f);

    switch (fScaleMode) {

        case SCALE_FIT_WINDOW:
        case SCALE_FIT_LARGE_ONLY:
        //case SCALE_FIT_SMALL_ONLY:
            // fully centered, no panning
            fOffset.Set(0, 0);
            break;

        case SCALE_FIT_WIDTH:
            // horizontal locked
            fOffset.x = 0;
            fOffset.y = _Clamp(fOffset.y, -maxOffsetY, maxOffsetY);
            break;

        case SCALE_FIT_HEIGHT:
            // vertical locked
            fOffset.y = 0;
            fOffset.x = _Clamp(fOffset.x, -maxOffsetX, maxOffsetX);
            break;

        //case SCALE_FILL_WINDOW:
        case SCALE_ORIGINAL:
        default:
            // free panning, but clamped
            fOffset.x = _Clamp(fOffset.x, -maxOffsetX, maxOffsetX);
            fOffset.y = _Clamp(fOffset.y, -maxOffsetY, maxOffsetY);
            break;
    }

    // Extra safety: if image smaller than view in a dimension → lock it
    if (drawW <= viewW)
        fOffset.x = 0;

    if (drawH <= viewH)
        fOffset.y = 0;
}

BBitmap* ImageView::Bitmap() const
{
	return fBitmap;
}

BBitmap* ImageView::CloneBitmap(const BBitmap* source)
{
	if (!source)
		return nullptr;

	BBitmap* copy = new BBitmap(source->Bounds(), source->ColorSpace());

	if (!copy->IsValid()) {
		delete copy;
		return nullptr;
	}

	memcpy(copy->Bits(), source->Bits(), source->BitsLength());

	return copy;
}


void ImageView::SaveState()
{
    if (!fBitmap)
        return;

    // Remove redo states
    while ((int32)fHistory.size() > fHistoryIndex + 1) {
        delete fHistory.back();
        fHistory.pop_back();
    }

    // Clone current bitmap
    BBitmap* copy = CloneBitmap(fBitmap);
    if (!copy)
        return;

    fHistory.push_back(copy);
    fHistoryIndex++;

    // Enforce max steps

    if ((int32)fHistory.size() > fMaxHistorySteps) {
        delete fHistory.front();
        fHistory.erase(fHistory.begin());
        fHistoryIndex--;
    }
	if (Window())
		Window()->PostMessage('stat');
}


void ImageView::Undo()
{
    if (fHistoryIndex <= 0)
        return;

	fHistoryIndex--;

	SetBitmap(CloneBitmap(fHistory[fHistoryIndex]));
    Invalidate();
	if (Window())
		Window()->PostMessage('stat');
}

void ImageView::Redo()
{
    if (fHistoryIndex >= (int32)fHistory.size() - 1)
        return;

    fHistoryIndex++;

    SetBitmap(CloneBitmap(fHistory[fHistoryIndex]));
    Invalidate();
	if (Window())
		Window()->PostMessage('stat');
}


void ImageView::ClearHistory()
{
    for (auto bitmap : fHistory)
        delete bitmap;

    fHistory.clear();
    fHistoryIndex = -1;
}