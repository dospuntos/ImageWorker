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

    void Update(const entry_ref* ref, const ImageView* imageView);
};

#endif // STATUS_VIEW_H
