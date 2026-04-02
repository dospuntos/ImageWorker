/*
 * Copyright 2024, My Name
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef IMAGE_VIEW_H
#define IMAGE_VIEW_H

#include <View.h>
#include <Bitmap.h>

class ImageView : public BView {
public:
    ImageView();
    virtual ~ImageView();

    void SetBitmap(BBitmap* bitmap);

    virtual void Draw(BRect updateRect);

private:
    BBitmap* fBitmap;
};

#endif // IMAGE_VIEW_H
