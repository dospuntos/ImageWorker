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

    void SetBitmap(BBitmap* bitmap);
	void SetScaleMode(ScaleMode mode);
	ScaleMode getScaleMode() const;

    virtual void Draw(BRect updateRect);
	virtual void FrameResized(float width, float height);
	virtual void KeyDown(const char* bytes, int32 numBytes);

	void Rotate90CW();
	void Rotate90CCW();

private:
    BBitmap* fBitmap;
	ScaleMode fScaleMode;
};

#endif // IMAGE_VIEW_H
