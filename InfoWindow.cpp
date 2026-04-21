/*
 * Copyright 2026, Johan Wagenheim <johan@dospuntos.no>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "InfoWindow.h"
#include "Constants.h"

#include <Catalog.h>
#include <Font.h>
#include <LayoutBuilder.h>
#include <StringView.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InfoWindow"


InfoWindow::InfoWindow()
	:
	BWindow(BRect(100, 100, 500, 800), B_TRANSLATE("Image properties"), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_CLOSE_ON_ESCAPE),
	fRow(0)
{
	fGrid = new BGridView();
	fGrid->GridLayout()->SetInsets(10, 10, 10, 10);
	fGrid->GridLayout()->SetHorizontalSpacing(10);
	fGrid->GridLayout()->SetVerticalSpacing(4);

	BLayoutBuilder::Group<>(this, B_VERTICAL).Add(fGrid).AddGlue();
}


InfoWindow::~InfoWindow()
{
}


bool
InfoWindow::QuitRequested()
{
	Hide(); // keep window alive for reuse
	return false;
}


void
InfoWindow::MessageReceived(BMessage* msg)
{
	switch (msg->what) {
		case MSG_SET_INFO:
			_BuildFromMessage(msg);
			break;

		default:
			BWindow::MessageReceived(msg);
	}
}


void
InfoWindow::_BuildFromMessage(BMessage* msg)
{
	_Clear();

	BMessage item;
	for (int32 i = 0; msg->FindMessage("item", i, &item) == B_OK; i++) {

		const char* type;
		if (item.FindString("type", &type) != B_OK)
			continue;

		if (strcmp(type, "section") == 0) {
			const char* title;
			if (item.FindString("title", &title) == B_OK)
				_AddSection(title);

		} else if (strcmp(type, "row") == 0) {
			const char* label;
			const char* value;

			if (item.FindString("label", &label) == B_OK
				&& item.FindString("value", &value) == B_OK) {
				_AddItem(label, value);
			}
		}
	}
	ResizeToPreferred();
}


void
InfoWindow::_Clear()
{
	while (BView* child = fGrid->ChildAt(0)) {
		child->RemoveSelf();
		delete child;
	}
	fRow = 0;
}


void
InfoWindow::_AddSection(const char* title)
{
	BStringView* section = new BStringView(nullptr, title);

	BFont font;
	section->GetFont(&font);
	font.SetFace(B_BOLD_FACE);
	section->SetFont(&font);

	fGrid->GridLayout()->AddView(section, 0, fRow, 2, 1);
	fRow++;
}


void
InfoWindow::_AddItem(const char* label, const char* value)
{
	BStringView* labelView = new BStringView(nullptr, label);

	BTextControl* text = new BTextControl(nullptr, nullptr, value, nullptr);
	text->SetEnabled(false);
	text->SetExplicitMinSize(BSize(300, B_SIZE_UNSET));
	text->TextView()->MakeSelectable(true);
	text->TextView()->MakeEditable(false);

	labelView->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	text->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

	fGrid->GridLayout()->AddView(labelView, 0, fRow);
	fGrid->GridLayout()->AddView(text, 1, fRow);

	fRow++;
}
