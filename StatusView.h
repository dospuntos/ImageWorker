/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef STATUS_VIEW_H
#define STATUS_VIEW_H

#include <StringView.h>
#include <Entry.h>

class BBitmap;
class ImageView;

class StatusView : public BStringView {
public:
    StatusView();

	virtual void Draw(BRect updateRect);

    void Update(const entry_ref* ref,
            const ImageView* imageView,
            int32 index,
            int32 total);
};

static off_t _GetFileSize(const entry_ref* ref);
static size_t _GetBitmapSize(BBitmap* bmp);
static void _FormatSize(char* out, size_t size);

#endif // STATUS_VIEW_H
