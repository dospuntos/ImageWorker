/*
 * Copyright 2024, My Name
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

    virtual void Draw(BRect updateRect);

private:
    BBitmap* fBitmap;
	ScaleMode fScaleMode;
};

#endif // IMAGE_VIEW_H
