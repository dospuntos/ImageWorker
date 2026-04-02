/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MainWindow.h"

#include <Alert.h>
#include <Application.h>
#include <Catalog.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <algorithm>
#include <String.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <Path.h>
#include <TranslationUtils.h>
#include <View.h>

#include <cstdio>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Window"

static const uint32 kMsgNewFile = 'fnew';
static const uint32 kMsgOpenFile = 'fopn';
static const uint32 kMsgSaveFile = 'fsav';
static const uint32 kMsgFitToWindow = 'fitw';
static const uint32 kMsgActualSize = 'acts';
static const uint32 kMsgNextImage = 'next';
static const uint32 kMsgPrevImage = 'prev';

static const char* kSettingsFile = "quickBitmap_settings";


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 500, 400), B_TRANSLATE("Quick Bitmap"), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
	BMenuBar* menuBar = _BuildMenu();
	fImageView = new ImageView();

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.Add(fImageView)
		.End();

	BMessenger messenger(this);
	fOpenPanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL, B_FILE_NODE, false);
	fSavePanel = new BFilePanel(B_SAVE_PANEL, &messenger, NULL, B_FILE_NODE, false);

	BMessage settings;
	_LoadSettings(settings);

	BRect frame;
	if (settings.FindRect("main_window_rect", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}
	MoveOnScreen();
	fImageView->MakeFocus(true);
}


MainWindow::~MainWindow()
{
	_SaveSettings();

	delete fOpenPanel;
	delete fSavePanel;
}


void
MainWindow::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_SIMPLE_DATA:
		case B_REFS_RECEIVED:
		{
			entry_ref ref;
			if (message->FindRef("refs", &ref) != B_OK)
				break;

			_LoadDirectory(ref);
			_LoadImage(ref);
			fSaveMenuItem->SetEnabled(true);
		} break;

		case B_SAVE_REQUESTED:
		{
			entry_ref ref;
			const char* name;
			if (message->FindRef("directory", &ref) == B_OK
				&& message->FindString("name", &name) == B_OK) {
				BDirectory directory(&ref);
				BEntry entry(&directory, name);
				BPath path = BPath(&entry);

				printf("Save path: %s\n", path.Path());
			}
		} break;

		case kMsgNewFile:
		{
			fSaveMenuItem->SetEnabled(false);
			printf("New\n");
		} break;

		case kMsgOpenFile:
		{
			fOpenPanel->Show();
		} break;

		case kMsgSaveFile:
		{
			fSavePanel->Show();
		} break;

		case kMsgFitToWindow:
		{
			fImageView->SetScaleMode(SCALE_FIT);
		} break;

		case kMsgActualSize:
		{
			fImageView->SetScaleMode(SCALE_ORIGINAL);
		} break;

		case kMsgNextImage:
		{
			NextImage();
		} break;

		case kMsgPrevImage:
		{
			PrevImage();
		} break;

		default:
		{
			BWindow::MessageReceived(message);
			break;
		}
	}
}


BMenuBar*
MainWindow::_BuildMenu()
{
	BMenuBar* menuBar = new BMenuBar("menubar");
	BMenu* menu;
	BMenuItem* item;

	// menu 'File'
	menu = new BMenu(B_TRANSLATE("File"));

	item = new BMenuItem(B_TRANSLATE("New"), new BMessage(kMsgNewFile), 'N');
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Open" B_UTF8_ELLIPSIS), new BMessage(kMsgOpenFile), 'O');
	menu->AddItem(item);

	fSaveMenuItem = new BMenuItem(B_TRANSLATE("Save"), new BMessage(kMsgSaveFile), 'S');
	fSaveMenuItem->SetEnabled(false);
	menu->AddItem(fSaveMenuItem);

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS), new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(be_app);
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q');
	menu->AddItem(item);

	menuBar->AddItem(menu);

	// menu 'View'
	menu = new BMenu(B_TRANSLATE("View"));

	item = new BMenuItem(B_TRANSLATE("Fit to window"), new BMessage(kMsgFitToWindow), 'F');
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Actual size"), new BMessage(kMsgActualSize), '1');
	menu->AddItem(item);

	menuBar->AddItem(menu);

	return menuBar;
}


status_t
MainWindow::_LoadSettings(BMessage& settings)
{
	BPath path;
	status_t status;
	status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append(kSettingsFile);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_READ_ONLY);
	if (status != B_OK)
		return status;

	return settings.Unflatten(&file);
}


status_t
MainWindow::_SaveSettings()
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	status = path.Append(kSettingsFile);
	if (status != B_OK)
		return status;

	BFile file;
	status = file.SetTo(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	BMessage settings;
	status = settings.AddRect("main_window_rect", Frame());

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}


void
MainWindow::_LoadImage(const entry_ref& ref)
{
	BBitmap* bitmap = BTranslationUtils::GetBitmap(&ref);
	if (bitmap == nullptr) {
		printf("Failed to load image\n");
		return;
	}

	fImageView->SetBitmap(bitmap);
}


void
MainWindow::_ToggleScaleMode()
{
	if (!fImageView)
		return;

	if (fImageView->getScaleMode() == SCALE_FIT)
		fImageView->SetScaleMode(SCALE_ORIGINAL);
	else
		fImageView->SetScaleMode(SCALE_FIT);
}


static bool _IsImageFile(const entry_ref& ref)
{
    BString name(ref.name);
    name.ToLower();

    return name.EndsWith(".png")
        || name.EndsWith(".jpg")
        || name.EndsWith(".jpeg")
        || name.EndsWith(".bmp")
        || name.EndsWith(".gif");
}


void MainWindow::_LoadDirectory(const entry_ref& ref)
{
    BEntry entry(&ref);
    BDirectory dir;

    if (entry.GetParent(&dir) != B_OK)
        return;

    fFileList.clear();

    entry_ref current;
    while (dir.GetNextRef(&current) == B_OK) {
        if (_IsImageFile(current))
            fFileList.push_back(current);
    }

    // sort alphabetically
    std::sort(fFileList.begin(), fFileList.end(),
        [](const entry_ref& a, const entry_ref& b) {
            return BString(a.name) < BString(b.name);
        });

    // find current index
    fCurrentIndex = -1;
    for (size_t i = 0; i < fFileList.size(); ++i) {
        if (fFileList[i] == ref) {
            fCurrentIndex = i;
            break;
        }
    }
}


void MainWindow::_LoadImageAtIndex(int32 index)
{
    if (index < 0 || index >= (int32)fFileList.size())
        return;

    fCurrentIndex = index;
    _LoadImage(fFileList[index]);
}


void MainWindow::NextImage()
{
    if (fFileList.empty())
        return;

    int32 next = fCurrentIndex + 1;
    if (next >= (int32)fFileList.size())
        next = 0; // wrap around

    _LoadImageAtIndex(next);
}

void MainWindow::PrevImage()
{
    if (fFileList.empty())
        return;

    int32 prev = fCurrentIndex - 1;
    if (prev < 0)
        prev = fFileList.size() - 1; // wrap around

    _LoadImageAtIndex(prev);
}