/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MainWindow.h"
#include "Constants.h"
#include "SettingsWindow.h"
#include "Utils.h"

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
#include <Roster.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <TranslationKit.h>
#include <BitmapStream.h>
#include <Clipboard.h>
#include <RecentItems.h>
#include <View.h>
#include <SeparatorView.h>
#include <NodeInfo.h>
#include <Path.h>
#include <Entry.h>
#include <Node.h>

#include <cstdio>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Window"

MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 500, 400), B_TRANSLATE_SYSTEM_NAME(kApplicationName), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE)
{
	BMenuBar* menuBar = _BuildMenu();
	fImageView = new ImageView();
	fStatusView = new StatusView();
	fToolBar = CreateToolbar(this);
	fSettingsWindow = nullptr;
	fHasImage = false;

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(menuBar)
		.Add(fToolBar)
		.Add(new BSeparatorView(B_HORIZONTAL))
		.Add(fImageView)
		.Add(new BSeparatorView(B_HORIZONTAL))
		.AddGroup(B_HORIZONTAL)
			.SetInsets(5, 3, 5, 3)
			.Add(fStatusView)
		.End();

	fStatusView->SetExplicitMaxSize(BSize(B_SIZE_UNLIMITED, B_SIZE_UNSET));
	fStatusView->SetExplicitMinSize(BSize(0, 20));

	BMessenger messenger(this);
	fOpenPanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL, B_FILE_NODE, false);
	fSavePanel = new BFilePanel(B_SAVE_PANEL, &messenger, NULL, B_FILE_NODE, false);

	BMessage settings;
	_LoadSettings(settings);

	// Restore settings
	BRect frame;
	if (settings.FindRect("main_window_rect", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		ResizeTo(frame.Width(), frame.Height());
	}

	settings.FindRef("lastSaveDir", &fLastSaveDir);
	if (settings.FindBool("closeOnEscape", &fCloseOnEscape) != B_OK)
		fCloseOnEscape = true; // default

	bool onTop;
	if (settings.FindBool("alwaysOnTop", &onTop) == B_OK) {
		SetFeel(onTop ? B_FLOATING_ALL_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL);
		fMAlwaysOnTop->SetMarked(onTop);
	}

	MoveOnScreen();
	fStatusView->SetAlignment(B_ALIGN_LEFT);
	fStatusView->SetExplicitMinSize(BSize(B_SIZE_UNSET, 20));
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

				BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
				if (file.InitCheck() != B_OK) {
					printf("Failed to open file for writing\n");
					break;
				}

				BBitmap* bitmap = fImageView->Bitmap();
				if (!bitmap)
					break;

				BBitmapStream stream(bitmap);

				// Save as PNG for now
				status_t result = BTranslatorRoster::Default()->Translate(
					&stream,
					NULL,
					NULL,
					&file,
					B_PNG_FORMAT
				);

				stream.DetachBitmap(&bitmap);

				if (result == B_OK) {
					printf("Saved to: %s\n", path.Path());

					// Update current file
					fCurrentRef = entry_ref();
					entry.GetRef(&fCurrentRef);

					// Remember directory
					fLastSaveDir = ref;

					_UpdateStatus();
				} else {
					printf("Save failed\n");
				}
			}
		} break;

		case M_NEW_FILE:
		{
			fSaveMenuItem->SetEnabled(false);

			printf("New\n");
		} break;

		case M_OPEN_FILE:
		{
			if (fLastSaveDir.name) {
				BEntry entry(&fLastSaveDir);
				BDirectory dir(&entry);
				fOpenPanel->SetPanelDirectory(&dir);
			}
			fOpenPanel->Show();
		} break;

		case M_SAVE_FILE:
		{
			if (!fImageView->Bitmap())
				break;

			if (fCurrentRef.name) {
				fSavePanel->SetSaveText(fCurrentRef.name);
			}

			// Set directory
			if (fLastSaveDir.name) {
				BEntry entry(&fLastSaveDir);
				BDirectory dir(&entry);
				fSavePanel->SetPanelDirectory(&dir);
			}

			fSavePanel->Show();
		} break;

		case M_FIT_TO_WINDOW:
		{
			fImageView->SetScaleMode(SCALE_FIT);
			_UpdateStatus();
		} break;

		case M_ACTUAL_SIZE:
		{
			fImageView->SetScaleMode(SCALE_ORIGINAL);
			_UpdateStatus();
		} break;

		case M_NEXT_IMAGE:
		{
			NextImage();
		} break;

		case M_PREV_IMAGE:
		{
			PrevImage();
		} break;

		case M_DELETE_IMAGE:
		{
			DeleteCurrentImage();
			break;
		}
		case M_ROTATE_90_CW:
		{
			fImageView->Rotate90CW();
		} break;

		case M_ROTATE_90_CCW:
		{
			fImageView->Rotate90CCW();
		} break;

		case M_FLIP_VERTICAL:
		{
			fImageView->FlipVertical();
		} break;

		case M_FLIP_HORIZONTAL:
		{
			fImageView->FlipHorizontal();
		} break;

		case M_CONVERT_TO_GRAYSCALE:
		{
			fImageView->ConvertToGrayscale();
		} break;

		case M_SWAP_COLOR_RBG:
		{
			int order[4] = {0, 2, 1, 3}; // R B G
			fImageView->SwapColors(order);
		} break;

		case M_SWAP_COLOR_GRB:
		{
			int order[4] = {1, 0, 2, 3}; // G R B
			fImageView->SwapColors(order);
		} break;

		case M_SWAP_COLOR_GBR:
		{
			int order[4] = {2, 0, 1, 3}; // G B R
			fImageView->SwapColors(order);
		} break;

		case M_SWAP_COLOR_BRG:
		{
			int order[4] = {1, 2, 0, 3}; // B R G
			fImageView->SwapColors(order);
		} break;

		case M_SWAP_COLOR_BGR:
		{
			int order[4] = {2, 1, 0, 3}; // B G R (swap R and B)
			fImageView->SwapColors(order);
		} break;


		case 'stat':
		{
			_UpdateStatus();
		} break;

		case M_PASTE:
		{
			if (!be_clipboard->Lock())
				break;

			BMessage* data = be_clipboard->Data();
			if (!data) {
				be_clipboard->Unlock();
				break;
			}

			BBitmap* bitmap = nullptr;

			// Try direct bitmap (rare but possible)
			if (data->FindPointer("bitmap", (void**)&bitmap) == B_OK && bitmap) {
				fImageView->SetBitmap(new BBitmap(bitmap)); // copy
			} else {
				// Try standard image formats via Translation Kit
				const void* buffer;
				ssize_t size;

				if (data->FindData("image/png", B_MIME_TYPE, &buffer, &size) == B_OK ||
					data->FindData("image/jpeg", B_MIME_TYPE, &buffer, &size) == B_OK) {

					BMemoryIO stream(buffer, size);
					bitmap = BTranslationUtils::GetBitmap(&stream);

					if (bitmap)
						fImageView->SetBitmap(bitmap);
				}
			}

			be_clipboard->Unlock();

			// Reset state (new "image session")
			PostMessage(M_CLEAR_IMAGE);
		} break;

		case M_CLEAR_IMAGE:
		{
			if (fHasImage) {
				fImageView->Clear();

				fCurrentRef = entry_ref();
				fFileList.clear();
				fCurrentIndex = -1;

				_UpdateStatus();
			}
		} break;

		case M_SHOW_INFO:
		{
			_ShowImageInfo();
			break;
		}

		case M_SHOW_SETTINGS:

		{
			if (!fSettingsWindow) {
				fSettingsWindow
					= new SettingsWindow(fCloseOnEscape);
				fSettingsWindow->CenterIn(Frame());
				fSettingsWindow->Show();
			} else {
				if (fSettingsWindow->IsHidden())
					fSettingsWindow->Show();
				fSettingsWindow->Activate();
			}
		} break;

		case M_APPLY_SETTINGS:
		{
			bool value;
			if (message->FindBool("value", &value) == B_OK) {
				fCloseOnEscape = value;
				_SaveSettings();
			}
		} break;

		case M_MINIMIZE_WINDOW:
			Minimize(true);
			break;

		case M_ALWAYS_ON_TOP:
		{
			bool onTop = _AlwaysOnTop();
			SetFeel(onTop ? B_NORMAL_WINDOW_FEEL : B_FLOATING_ALL_WINDOW_FEEL);
			fMAlwaysOnTop->SetMarked(!onTop);
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
	BMenu* submenu;
	BMenuItem* item;

	// menu 'File'
	menu = new BMenu(B_TRANSLATE("File"));

	BMenuItem* openItem
		= new BMenuItem(BRecentFilesList::NewFileListMenu(B_TRANSLATE("Open" B_UTF8_ELLIPSIS), NULL,
							NULL, be_app, 9, true, NULL, kApplicationSignature),
			new BMessage(M_OPEN_FILE));
	openItem->SetShortcut('O', 0);
	menu->AddItem(openItem);

	fSaveMenuItem = new BMenuItem(B_TRANSLATE("Save"), new BMessage(M_SAVE_FILE), 'S');
	fSaveMenuItem->SetEnabled(false);
	menu->AddItem(fSaveMenuItem);

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS), new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(be_app);
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q');
	menu->AddItem(item);

	menuBar->AddItem(menu);

	// menu 'Edit'
	menu = new BMenu(B_TRANSLATE("Edit"));

	menu->AddItem(new BMenuItem("Paste", new BMessage(M_PASTE), 'V'));

	menu->AddSeparatorItem();


	menuBar->AddItem(menu);

	// menu 'Image'
	menu = new BMenu(B_TRANSLATE("Image"));

	fMInformation = new BMenuItem(B_TRANSLATE("Information"), new BMessage(M_SHOW_INFO), 'I');
	menu->AddItem(fMInformation);

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Create new (empty) image..."), new BMessage(M_NEW_FILE), 'N');
	menu->AddItem(item);

	menu->AddSeparatorItem();

	fMRotate90CCW = new BMenuItem(B_TRANSLATE("Rotate left (counter-clockwise)"), new BMessage(M_ROTATE_90_CCW), 'L');
	menu->AddItem(fMRotate90CCW);

	fMRotate90CW = new BMenuItem(B_TRANSLATE("Rotate right (clockwise)"), new BMessage(M_ROTATE_90_CW), 'R');
	menu->AddItem(fMRotate90CW);

	fMFlipVertical = new BMenuItem(B_TRANSLATE("Vertical flip"), new BMessage(M_FLIP_VERTICAL));
	menu->AddItem(fMFlipVertical);

	fMFlipHorizontal = new BMenuItem(B_TRANSLATE("Horizontal flip"), new BMessage(M_FLIP_HORIZONTAL));
	menu->AddItem(fMFlipHorizontal);

	menu->AddSeparatorItem();

	fMConvertToGrayscale = new BMenuItem(B_TRANSLATE("Convert to grayscale"), new BMessage(M_CONVERT_TO_GRAYSCALE), 'G');
	menu->AddItem(fMConvertToGrayscale);

	menu->AddSeparatorItem();

	fMSwapColors = new BMenu(B_TRANSLATE("Swap colors"));

	item = new BMenuItem("RGB -> RBG", new BMessage(M_SWAP_COLOR_RBG));
	fMSwapColors->AddItem(item);

	item = new BMenuItem("RGB -> GRB", new BMessage(M_SWAP_COLOR_GRB));
	fMSwapColors->AddItem(item);

	item = new BMenuItem("RGB -> GBR", new BMessage(M_SWAP_COLOR_GBR));
	fMSwapColors->AddItem(item);

	item = new BMenuItem("RGB -> BRG", new BMessage(M_SWAP_COLOR_BRG));
	fMSwapColors->AddItem(item);

	item = new BMenuItem("RGB -> BGR", new BMessage(M_SWAP_COLOR_BGR));
	fMSwapColors->AddItem(item);

	menu->AddItem(fMSwapColors);

	menuBar->AddItem(menu);

	// menu 'Options'
	menu = new BMenu(B_TRANSLATE("Options"));

	item = new BMenuItem(B_TRANSLATE("Properties/Settings..."), new BMessage(M_SHOW_SETTINGS), 'P');
	menu->AddItem(item);

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Minimize"), new BMessage(M_MINIMIZE_WINDOW), 'M');
	menu->AddItem(item);

	fMAlwaysOnTop = new BMenuItem(B_TRANSLATE("Always on top"), new BMessage(M_ALWAYS_ON_TOP));
	menu->AddItem(fMAlwaysOnTop);

	menuBar->AddItem(menu);

	// menu 'View'
	menu = new BMenu(B_TRANSLATE("View"));

	item = new BMenuItem(B_TRANSLATE("Fit to window"), new BMessage(M_FIT_TO_WINDOW), 'F');
	menu->AddItem(item);

	item = new BMenuItem(B_TRANSLATE("Actual size"), new BMessage(M_ACTUAL_SIZE), '1');
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
	settings.AddRect("main_window_rect", Frame());
	if (fLastSaveDir.device >= 0)
		status = settings.AddRef("lastSaveDir", &fLastSaveDir);
	settings.AddBool("closeOnEscape", fCloseOnEscape);
	settings.AddBool("alwaysOnTop",_AlwaysOnTop());

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}


void
MainWindow::MenusBeginning()
{
	fMInformation->SetEnabled(fHasImage);
	fMRotate90CW->SetEnabled(fHasImage);
	fMRotate90CCW->SetEnabled(fHasImage);
	fMFlipVertical->SetEnabled(fHasImage);
	fMFlipHorizontal->SetEnabled(fHasImage);
	fMConvertToGrayscale->SetEnabled(fHasImage);
	fMSwapColors->SetEnabled(fHasImage);
}


void
MainWindow::_LoadImage(const entry_ref& ref)
{
	bigtime_t start = system_time();
	BString statusMsg;
	BBitmap* bitmap = BTranslationUtils::GetBitmap(&ref);
	if (bitmap == nullptr) {
		statusMsg.SetToFormat("Failed to load image: %s", ref.name);
		fStatusView->SetStatus(statusMsg);
		fImageView->Clear();
		fHasImage = false;
		return;
	}

	fCurrentRef = ref;
	fImageView->SetBitmap(bitmap);
	fHasImage = true;

	BEntry entry(&ref);
	BEntry parent;
	if (entry.GetParent(&parent) == B_OK)
		parent.GetRef(&fLastSaveDir);

	bigtime_t end = system_time();
	fLoadTime = end - start;
	_UpdateStatus();
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
    BNode node(&ref);
    if (node.InitCheck() != B_OK)
        return false;

    BNodeInfo nodeInfo(&node);
    char mimeType[B_MIME_TYPE_LENGTH];
    if (nodeInfo.GetType(mimeType) != B_OK)
        return false;

    return strncmp(mimeType, "image/", 6) == 0;
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


void MainWindow::DeleteCurrentImage()
{
	BAlert* alert = new BAlert("Confirm",
		"Delete this image permanently?",
		"Cancel", "Delete", nullptr,
		B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);

	if (alert->Go() != 1)
		return;

    if (fFileList.empty() || fCurrentIndex < 0)
        return;

    entry_ref ref = fFileList[fCurrentIndex];
    BEntry entry(&ref);

    // Try move to Trash (safer)
    status_t result = entry.Remove();

    if (result != B_OK) {
        printf("Failed to delete image\n");
        return;
    }

    // Remove from list
    fFileList.erase(fFileList.begin() + fCurrentIndex);

    if (fFileList.empty()) {
        // No images left
        fCurrentIndex = -1;
        fImageView->SetBitmap(nullptr);
        return;
    }

    // Stay at same index if possible
    if (fCurrentIndex >= (int32)fFileList.size())
        fCurrentIndex = fFileList.size() - 1;

    _LoadImageAtIndex(fCurrentIndex);
	_UpdateStatus();
}


void MainWindow::_UpdateStatus()
{
	BString name = (fCurrentRef.name) ? fCurrentRef.name : "(unnamed)";
	name << " - " << kApplicationName;
	SetTitle(name);
    fStatusView->Update(&fCurrentRef,
                        fImageView,
                        fCurrentIndex,
                        fFileList.size());
}


void MainWindow::_ShowImageInfo()
{
    BBitmap* bitmap = fImageView->Bitmap();
    if (!bitmap) {
        BAlert* alert = new BAlert("Info", "No image loaded.", "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
		alert->SetShortcut(0, B_ESCAPE);
		alert->Go();
        return;
    }

    BString info("IMAGE PROPERTIES:\n\n");

    // --- File info ---
    if (fCurrentRef.name) {
        info << "File name: " << fCurrentRef.name << "\n";

        BEntry entry(&fCurrentRef);
        BPath path;
        entry.GetPath(&path);

		// Folder
        BEntry parent;
        if (entry.GetParent(&parent) == B_OK) {
            BPath parentPath;
            parent.GetPath(&parentPath);
            info << "Folder: " << parentPath.Path() << "\n";
        }

		BNode node(&entry);
		BNodeInfo nodeInfo(&node);

		char mimeType[B_MIME_TYPE_LENGTH];

		if (nodeInfo.GetType(mimeType) == B_OK) {
			info << "Type: " << mimeType << "\n";
		}

        info << "Full path: " << path.Path() << "\n";

        // File sizes
		char diskBuf[32], memBuf[32];
		FormatSize(diskBuf, GetFileSize(&fCurrentRef));
		FormatSize(memBuf, bitmap->BitsLength());
		info << "Disk size: " << diskBuf << "\n";
		info << "Current memory size: " << memBuf << "\n";

        time_t modTime;
        if (node.GetModificationTime(&modTime) == B_OK) {
            info << "File date/time: " << ctime(&modTime) << "\n";
        }
    }

    // --- Bitmap info ---
    int32 width  = bitmap->Bounds().IntegerWidth() + 1;
    int32 height = bitmap->Bounds().IntegerHeight() + 1;
    info << "Original size: " << FormatDimensions(width, height) << "\n";

    // Current zoom size
    float zoom = fImageView->EffectiveZoom();
    int32 currentW = width * zoom;
    int32 currentH = height * zoom;
    info << "Current size: " << FormatDimensions(currentW, currentH) << "\n";

	// Color space
    color_space cs = bitmap->ColorSpace();
    info << "Color space: " << cs << "\n";

	// Load time
	info << "Load time: " << fLoadTime / 1000.0 << " ms\n";

    // Folder index
    if (!fFileList.empty() && fCurrentIndex >= 0) {
        info << "Index: " << (fCurrentIndex + 1)
             << " / " << fFileList.size() << "\n";
    }

    BAlert* alert = new BAlert("Image properties", info.String(), "OK");
	alert->SetShortcut(0, B_ESCAPE);
	alert->Go();
}


bool MainWindow::QuitRequested(void)
{
	if (fSettingsWindow && fSettingsWindow->LockLooper()) {
		fSettingsWindow->Quit();
		fSettingsWindow = nullptr;
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}