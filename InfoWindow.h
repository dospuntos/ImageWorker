/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef INFO_WINDOW_H
#define INFO_WINDOW_H

#include <Window.h>
#include <String.h>
#include <GridView.h>
#include <functional>

class InfoWindow : public BWindow {
public:
                                InfoWindow();
    virtual                     ~InfoWindow();

	virtual void				MessageReceived(BMessage* msg);
	virtual bool				QuitRequested();

	std::function<void()> 		OnClose;

private:
	void						_BuildFromMessage(BMessage* msg);
    void                        _Clear();
    void                        _AddItem(const char* label, const char* value);
    void                        _AddSection(const char* title);

    BGridView*                  fGrid;
    int32                       fRow;
};

#endif // INFO_WINDOW_H
