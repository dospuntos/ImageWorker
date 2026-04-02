/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef APP_H
#define APP_H


#include "MainWindow.h"
#include <Application.h>


class App : public BApplication
{
public:
							App();
	virtual					~App();
	void RefsReceived(BMessage* message) override {
        entry_ref ref;
        int32 index = 0;

        while (message->FindRef("refs", index++, &ref) == B_OK) {
            // Send to your main window for actual loading
            BMessenger messenger(fMainWindow);
            messenger.SendMessage(message);
        }
    }

    void ArgvReceived(int32 argc, char** argv) override {
        // Handles launching via command line: imageWorker file.jpg
        BMessage message(B_REFS_RECEIVED);

        for (int32 i = 1; i < argc; i++) {
            BEntry entry(argv[i]);
            entry_ref ref;
            entry.GetRef(&ref);
            message.AddRef("refs", &ref);
        }

        RefsReceived(&message);
    }

	virtual void			AboutRequested();

private:
	MainWindow* fMainWindow;
};

#endif // APP_H

