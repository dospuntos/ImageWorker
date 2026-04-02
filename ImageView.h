/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef IMAGE_VIEW_H
#define IMAGE_VIEW_H

#include <View.h>
#include <Bitmap.h>

enum ScaleMode {
	SCALE_FIT,
	SCALE_ORIGINAL
};

class ImageView : public BView {
public:
    ImageView();
    virtual ~ImageView();

	void MessageReceived(BMessage* message);

    void SetBitmap(BBitmap* bitmap);
	void SetScaleMode(ScaleMode mode);
	ScaleMode getScaleMode() const;
	BBitmap* Bitmap() const;

    virtual void Draw(BRect updateRect);
	virtual void FrameResized(float width, float height);
	virtual void KeyDown(const char* bytes, int32 numBytes);
	virtual void MouseWheelChanged(BPoint where, float x, float y);
	BPoint fOffset; // Image offset from center

	virtual void MouseDown(BPoint where);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 code, const BMessage* dragMessage);

	bool fDragging;
	BPoint fLastMouse;
	void _ClampOffset();

	void Rotate90CW();
	void Rotate90CCW();

	float Zoom() const;
	void SetZoom(float zoom);
	void ZoomIn();
	void ZoomOut();

	float fZoom = 1.0f;
	float EffectiveZoom() const;

private:
    BBitmap* fBitmap;
	ScaleMode fScaleMode;
};

#endif // IMAGE_VIEW_H
