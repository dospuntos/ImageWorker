/*
 * Copyright 2026, Johan Wagenheim <johan@dospuntos.no>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SettingsWindow.h"
#include "Constants.h"

#include <Application.h>
#include <LayoutBuilder.h>
#include <CheckBox.h>
#include <Button.h>
#include <Message.h>

SettingsWindow::SettingsWindow(bool closeOnEscape, int undoSteps)
    :
    BWindow(BRect(200, 200, 500, 400), "Settings", B_TITLED_WINDOW,
		B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE | B_AUTO_UPDATE_SIZE_LIMITS
			| B_CLOSE_ON_ESCAPE)
{
    fCloseOnEscape = new BCheckBox("closeOnEscape",
        "Close on Escape",
        new BMessage(M_APPLY_SETTINGS));
    fCloseOnEscape->SetValue(closeOnEscape);

	fUndoSteps
		= new BSpinner("UndoSteps", "Undo steps:", new BMessage(M_APPLY_SETTINGS));
	fUndoSteps->SetMinValue(0);
	fUndoSteps->SetValue(undoSteps);

    BButton* closeBtn = new BButton("close", "Close",
        new BMessage(B_QUIT_REQUESTED));

    BLayoutBuilder::Group<>(this, B_VERTICAL, 10)
        .SetInsets(10, 10, 10, 10)
        .Add(fCloseOnEscape)
		.Add(fUndoSteps)
        .AddGlue()
        .AddGroup(B_HORIZONTAL)
            .AddGlue()
            .Add(closeBtn)
        .End();
    Show();
}


void SettingsWindow::MessageReceived(BMessage* message)
{
    switch (message->what) {
        case M_APPLY_SETTINGS:
        {
            BMessage msg(M_APPLY_SETTINGS);
            msg.AddBool("value", fCloseOnEscape->Value() == B_CONTROL_ON);
			msg.AddInt32("undoSteps", fUndoSteps->Value());

			PostMessage(M_CLOSE_SETTINGS);
            be_app->WindowAt(0)->PostMessage(&msg);
            break;
        }
		case M_CLOSE_SETTINGS:
			Hide();
			break;

        default:
            BWindow::MessageReceived(message);
    }
}

bool
SettingsWindow::QuitRequested()
{
	if (!IsHidden()) {
		Hide();
	}
	return false;
}

