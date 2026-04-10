/*
 * Copyright 2026, Johan Wagenheim <johan@dospuntos.no>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef SETTINGS_WINDOW_H
#define SETTINGS_WINDOW_H

#include <Window.h>

class BCheckBox;

class SettingsWindow : public BWindow {
public:
    SettingsWindow(bool closeOnEscape);

    virtual void MessageReceived(BMessage* message);
	bool QuitRequested();

private:
    BCheckBox* fCloseOnEscape;
};

#endif // SETTINGS_WINDOW_H
