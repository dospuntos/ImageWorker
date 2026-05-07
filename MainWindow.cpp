/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "MainWindow.h"
#include "Constants.h"
#include "IconMenuItem.h"
#include "SettingsWindow.h"
#include "Utils.h"

#include <Alert.h>
#include <Application.h>
#include <BitmapStream.h>
#include <Button.h>
#include <Catalog.h>
#include <Clipboard.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <LayoutBuilder.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MimeType.h>
#include <Node.h>
#include <NodeInfo.h>
#include <Path.h>
#include <PopUpMenu.h>
#include <RecentItems.h>
#include <Roster.h>
#include <SeparatorView.h>
#include <String.h>
#include <TranslationKit.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>
#include <View.h>
#include <algorithm>
#include <stdlib.h>

#include <cstdio>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Window"


MainWindow::MainWindow()
	:
	BWindow(BRect(100, 100, 800, 600), B_TRANSLATE_SYSTEM_NAME(kApplicationName), B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS | B_QUIT_ON_WINDOW_CLOSE),
	fHasImage(false),
	fUndoSteps(5),
	fSettingsWindow(nullptr),
	fInfoWindow(nullptr),
	fSelectedFormat(B_PNG_FORMAT),
	fSelectedFormatIndex(1),
	fSelectedTranslator(0)
{
	BMenuBar* menuBar = _BuildMenu();
	fImageView = new ImageView();
	fStatusView = new StatusView();
	fToolBar = CreateToolbar(this);

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

	BMessenger messenger(this);
	fOpenPanel = new BFilePanel(B_OPEN_PANEL, &messenger, NULL, B_FILE_NODE, false);
	_InitSavePanel();
	BMessage settings;
	_LoadSettings(settings);

	// Restore settings
	BRect frame;
	if (settings.FindRect("main_window_rect", &frame) == B_OK) {
		MoveTo(frame.LeftTop());
		// ResizeTo(frame.Width(), frame.Height());
	}

	settings.FindRef("lastSaveDir", &fLastSaveDir);
	if (settings.FindBool("closeOnEscape", &fCloseOnEscape) != B_OK)
		fCloseOnEscape = true; // default

	bool onTop;
	if (settings.FindBool("alwaysOnTop", &onTop) == B_OK) {
		SetFeel(onTop ? B_FLOATING_ALL_WINDOW_FEEL : B_NORMAL_WINDOW_FEEL);
		fMAlwaysOnTop->SetMarked(onTop);
	}

	int32 undoSteps;
	if (settings.FindInt32("undoSteps", &undoSteps) == B_OK)
		fUndoSteps = undoSteps;

	int32 scaleMode;
	if (settings.FindInt32("scaleMode", &scaleMode) == B_OK)
		fImageView->SetScaleMode((ScaleMode)scaleMode);

	uint32 translatorId;

	if (settings.FindUInt32("translatorId", &translatorId) == B_OK)
		fSelectedTranslator = (translator_id)translatorId;

	settings.FindUInt32("selectedFormat", &fSelectedFormat);
	settings.FindInt32("selectedFormatIndex", &fSelectedFormatIndex);

	MoveOnScreen();
	_UpdateStatus();
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
		} break;

		case B_SAVE_REQUESTED:
			_SaveRequested(message);
			break;

		case M_NEW_FILE:
			fMSave->SetEnabled(false);
			printf("New\n");
			break;

		case M_OPEN_FILE:
		{
			if (fLastSaveDir.name) {
				BEntry entry(&fLastSaveDir);
				BDirectory dir(&entry);
				fOpenPanel->SetPanelDirectory(&dir);
			}
			fOpenPanel->Show();
		} break;

		case M_REOPEN_FILE:
		{
			_LoadDirectory(fCurrentRef);
			_LoadImage(fCurrentRef);
		} break;
		case M_SAVE_FILE:
			_ShowSavePanel();
			break;
		case M_FORMAT_SELECTED:
		{
			message->FindInt32("be:type", (int32*)&fSelectedFormat);
			message->FindInt32("be:translator", (int32*)&fSelectedTranslator);
			message->FindInt32("be:format", &fSelectedFormatIndex);

			_UpdateFilenameExtension();

			break;
		}
		case M_SHOW_TRANSLATOR_SETTINGS:
		{
			if (fSelectedTranslator < 0)
				break;

			BTranslatorRoster* roster = BTranslatorRoster::Default();

			BView* configView = nullptr;
			BRect rect;

			if (roster->MakeConfigurationView(
					fSelectedTranslator,
					nullptr,
					&configView,
					&rect) != B_OK || !configView) {
				printf("Translator has no settings\n");
				break;
			}

			BWindow* parent = fSavePanel->Window();

			BWindow* settingsWin = new BWindow(
				BRect(100, 100, 100 + rect.Width(), 100 + rect.Height()),
				"Translator Settings",
				B_TITLED_WINDOW,
				B_ASYNCHRONOUS_CONTROLS | B_AUTO_UPDATE_SIZE_LIMITS | B_CLOSE_ON_ESCAPE
			);

			parent->AddToSubset(settingsWin);

			settingsWin->AddChild(configView);
			BRect frame = parent->Frame();
			settingsWin->MoveTo(frame.left + 40, frame.top + 40);

			settingsWin->Show();
			break;
		}

		case B_UNDO:
			fImageView->Undo();
			break;
		case B_REDO:
			fImageView->Redo();
			break;
		case M_FIT_TO_WINDOW:
			fImageView->SetScaleMode(SCALE_FIT_WINDOW);
			_UpdateStatus();
			break;
		case M_ACTUAL_SIZE:
			fImageView->SetScaleMode(SCALE_ORIGINAL);
			_UpdateStatus();
			break;
		case M_FIT_LARGE_TO_WINDOW:
			fImageView->SetScaleMode(SCALE_FIT_LARGE_ONLY);
			_UpdateStatus();
			break;
		case M_FIT_WIDTH:
			fImageView->SetScaleMode(SCALE_FIT_WIDTH);
			_UpdateStatus();
			break;
		case M_FIT_HEIGHT:
			fImageView->SetScaleMode(SCALE_FIT_HEIGHT);
			_UpdateStatus();
			break;

		case M_ZOOM_IN:
			fImageView->ZoomIn();
			break;
		case M_ZOOM_OUT:
			fImageView->ZoomOut();
			break;

		case M_NEXT_IMAGE:
			NextImage();
			break;
		case M_PREV_IMAGE:
			PrevImage();
			break;
		case M_FIRST_IMAGE:
			FirstImage();
			break;
		case M_LAST_IMAGE:
			LastImage();
			break;
		case M_RANDOM_IMAGE:
			RandomImage();
			break;
		case M_DELETE_IMAGE:
			DeleteCurrentImage();
			break;
		case M_ROTATE_90_CW:
			fImageView->Rotate90CW();
			break;
		case M_ROTATE_90_CCW:
			fImageView->Rotate90CCW();
			break;
		case M_FLIP_VERTICAL:
			fImageView->FlipVertical();
			break;
		case M_FLIP_HORIZONTAL:
			fImageView->FlipHorizontal();
			break;
		case M_CONVERT_TO_GRAYSCALE:
			fImageView->ConvertToGrayscale();
			break;
		case M_CONVERT_TO_SEPIA:
			fImageView->Sepia();
			break;

		case M_SWAP_COLOR_RBG:
		{
			int order[4] = {1, 0, 2, 3}; // R B G
			fImageView->SwapColors(order);
		} break;
		case M_SWAP_COLOR_GRB:
		{
			int order[4] = {0, 2, 1, 3}; // G R B
			fImageView->SwapColors(order);
		} break;
		case M_SWAP_COLOR_GBR:
		{
			int order[4] = {1, 2, 0, 3}; // G B R
			fImageView->SwapColors(order);
		} break;
		case M_SWAP_COLOR_BRG:
		{
			int order[4] = {2, 0, 1, 3}; // B R G
			fImageView->SwapColors(order);
		} break;
		case M_SWAP_COLOR_BGR:
		{
			int order[4] = {2, 1, 0, 3}; // B G R
			fImageView->SwapColors(order);
		} break;

		case M_ISOLATE_CHANNEL_RED:
			fImageView->IsolateChannel(CHANNEL_RED, true);
			break;
		case M_ISOLATE_CHANNEL_GREEN:
			fImageView->IsolateChannel(CHANNEL_GREEN, true);
			break;
		case M_ISOLATE_CHANNEL_BLUE:
			fImageView->IsolateChannel(CHANNEL_BLUE, true);
			break;

		case M_INVERT_COLORS:
			fImageView->InvertColors();
			break;
		case M_AUTO_ADJUST_COLORS:
			fImageView->AutoAdjustColors();
			break;
		case M_SHARPEN_IMAGE:
			fImageView->Sharpen();
			break;
		case M_BLUR_IMAGE:
			fImageView->Blur();
			break;
		case M_EMBOSS_IMAGE:
			fImageView->Emboss();
			break;
		case M_SCREENSHOT:
			fImageView->GrabScreen();
			break;

		case 'stat':
			_UpdateStatus();
			break;

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

				if (data->FindData("image/png", B_MIME_TYPE, &buffer, &size) == B_OK
					|| data->FindData("image/jpeg", B_MIME_TYPE, &buffer, &size) == B_OK) {

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
				fHasImage = false;

				_UpdateStatus();
			}
		} break;

		case M_SHOW_INFO:
			_ShowImageInfo();
			break;
		case M_SHOW_SETTINGS:
		{
			if (!fSettingsWindow) {
				fSettingsWindow = new SettingsWindow(fCloseOnEscape, fUndoSteps);
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
			bool flag;
			int32 num;
			if (message->FindBool("value", &flag) == B_OK)
				fCloseOnEscape = flag;
			if (message->FindInt32("undoSteps", &num) == B_OK)
				fUndoSteps = num;

			_SaveSettings();
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
			BWindow::MessageReceived(message);
			break;
	}
}


BMenuBar*
MainWindow::_BuildMenu()
{
	BMenuBar* menuBar = new BMenuBar("menubar");
	BMenu* menu;
	BMenu* submenu;
	BMenuItem* item;

	// App menu
	menu = new BMenu("");
	item = new BMenuItem(B_TRANSLATE("About" B_UTF8_ELLIPSIS), new BMessage(B_ABOUT_REQUESTED));
	item->SetTarget(be_app);
	menu->AddItem(item);
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Help" B_UTF8_ELLIPSIS), new BMessage(M_SHOW_HELP), 'H'));
	menu->AddItem(
		new BMenuItem(B_TRANSLATE("Report a bug" B_UTF8_ELLIPSIS), new BMessage(M_REPORT_A_BUG)));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Settings" B_UTF8_ELLIPSIS),
		new BMessage(M_SHOW_SETTINGS), ',', B_COMMAND_KEY));
	menu->AddSeparatorItem();
	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q'));

	IconMenuItem* iconMenu = new IconMenuItem(menu, NULL, kApplicationSignature, B_MINI_ICON);
	menuBar->AddItem(iconMenu);

	// menu 'File'
	menu = new BMenu(B_TRANSLATE("File"));

	BMenuItem* openItem
		= new BMenuItem(BRecentFilesList::NewFileListMenu(B_TRANSLATE("Open" B_UTF8_ELLIPSIS), NULL,
							NULL, be_app, 9, true, NULL, kApplicationSignature),
			new BMessage(M_OPEN_FILE));
	openItem->SetShortcut('O', 0);
	menu->AddItem(openItem);

	fMReopen = new BMenuItem(B_TRANSLATE("Reopen"), new BMessage(M_REOPEN_FILE), 'R', B_SHIFT_KEY);
	menu->AddItem(fMReopen);

	menu->AddSeparatorItem();

	fMDeleteImage = new BMenuItem(B_TRANSLATE("Delete file"), new BMessage(M_DELETE_IMAGE));
	menu->AddItem(fMDeleteImage);

	menu->AddSeparatorItem();

	fMSave = new BMenuItem(B_TRANSLATE("Save (original folder)"), new BMessage(M_SAVE_FILE), 'S');
	menu->AddItem(fMSave);

	fMSaveAs = new BMenuItem(B_TRANSLATE("Save as" B_UTF8_ELLIPSIS), new BMessage(M_SAVE_FILE));
	menu->AddItem(fMSaveAs);

	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED)));

	menuBar->AddItem(menu);

	// menu 'Edit'
	menu = new BMenu(B_TRANSLATE("Edit"));

	menu->AddItem(new BMenuItem("Paste", new BMessage(M_PASTE), 'V'));

	menu->AddSeparatorItem();

	fMUndo = new BMenuItem(B_TRANSLATE("Undo"), new BMessage(B_UNDO), 'Z');
	menu->AddItem(fMUndo);

	fMRedo = new BMenuItem(B_TRANSLATE("Redo"), new BMessage(B_REDO), 'Z', B_SHIFT_KEY);
	menu->AddItem(fMRedo);

	menuBar->AddItem(menu);

	// menu 'Image'
	menu = new BMenu(B_TRANSLATE("Image"));

	fMInformation = new BMenuItem(B_TRANSLATE("Information"), new BMessage(M_SHOW_INFO), 'I');
	menu->AddItem(fMInformation);

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Create new (empty) image..."), new BMessage(M_NEW_FILE), 'N');
	menu->AddItem(item);

	menu->AddSeparatorItem();

	fMRotate90CCW = new BMenuItem(B_TRANSLATE("Rotate left (counter-clockwise)"),
		new BMessage(M_ROTATE_90_CCW), 'L');
	menu->AddItem(fMRotate90CCW);

	fMRotate90CW
		= new BMenuItem(B_TRANSLATE("Rotate right (clockwise)"), new BMessage(M_ROTATE_90_CW), 'R');
	menu->AddItem(fMRotate90CW);

	fMFlipVertical = new BMenuItem(B_TRANSLATE("Vertical flip"), new BMessage(M_FLIP_VERTICAL));
	menu->AddItem(fMFlipVertical);

	fMFlipHorizontal
		= new BMenuItem(B_TRANSLATE("Horizontal flip"), new BMessage(M_FLIP_HORIZONTAL));
	menu->AddItem(fMFlipHorizontal);

	menu->AddSeparatorItem();

	fMConvertToGrayscale = new BMenuItem(B_TRANSLATE("Convert to grayscale"),
		new BMessage(M_CONVERT_TO_GRAYSCALE), 'G');
	menu->AddItem(fMConvertToGrayscale);

	// submenu 'Show channel'
	fMShowChannel = new BMenu(B_TRANSLATE("Show channel"));
	item = new BMenuItem("Isolate red", new BMessage(M_ISOLATE_CHANNEL_RED));
	fMShowChannel->AddItem(item);
	item = new BMenuItem("Isolate green", new BMessage(M_ISOLATE_CHANNEL_GREEN));
	fMShowChannel->AddItem(item);
	item = new BMenuItem("Isolate blue", new BMessage(M_ISOLATE_CHANNEL_BLUE));
	fMShowChannel->AddItem(item);
	menu->AddItem(fMShowChannel);

	fMInvertColors
		= new BMenuItem(B_TRANSLATE("Negative (invert colors)"), new BMessage(M_INVERT_COLORS));
	menu->AddItem(fMInvertColors);

	menu->AddSeparatorItem();

	fMAutoAdjust = new BMenuItem(B_TRANSLATE("Auto-adjust colors"),
		new BMessage(M_AUTO_ADJUST_COLORS));
	menu->AddItem(fMAutoAdjust);
	fMSharpen = new BMenuItem(B_TRANSLATE("Sharpen"), new BMessage(M_SHARPEN_IMAGE));
	menu->AddItem(fMSharpen);

	// submenu 'Effects'
	fMEffects = new BMenu(B_TRANSLATE("Effects"));
	item = new BMenuItem(B_TRANSLATE("3D Button"), new BMessage(M_INVERT_COLORS));
	fMEffects->AddItem(item);
	item = new BMenuItem(B_TRANSLATE("Blur"), new BMessage(M_BLUR_IMAGE));
	fMEffects->AddItem(item);
	item = new BMenuItem(B_TRANSLATE("Emboss"), new BMessage(M_EMBOSS_IMAGE));
	fMEffects->AddItem(item);
	item = new BMenuItem(B_TRANSLATE("Sepia"), new BMessage(M_CONVERT_TO_SEPIA));
	fMEffects->AddItem(item);

	menu->AddItem(fMEffects);

	menu->AddSeparatorItem();
	// submenu 'Swap colors'
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

	item = new BMenuItem(B_TRANSLATE("Capture/Screenshot"), new BMessage(M_SCREENSHOT));
	menu->AddItem(item);

	menu->AddSeparatorItem();

	item = new BMenuItem(B_TRANSLATE("Minimize"), new BMessage(M_MINIMIZE_WINDOW), 'M');
	menu->AddItem(item);

	fMAlwaysOnTop = new BMenuItem(B_TRANSLATE("Always on top"), new BMessage(M_ALWAYS_ON_TOP));
	menu->AddItem(fMAlwaysOnTop);

	menuBar->AddItem(menu);

	// menu 'View'
	menu = new BMenu(B_TRANSLATE("View"));

	submenu = new BMenu(B_TRANSLATE("Display options (window mode)"));

	fMFit = new BMenuItem(B_TRANSLATE("Fit to window"), new BMessage(M_FIT_TO_WINDOW));
	submenu->AddItem(fMFit);

	fMFitLarge = new BMenuItem(B_TRANSLATE("Fit only big images to window"),
		new BMessage(M_FIT_LARGE_TO_WINDOW));
	submenu->AddItem(fMFitLarge);

	fMActualSize
		= new BMenuItem(B_TRANSLATE("Actual size (1:1)"), new BMessage(M_ACTUAL_SIZE), '1');
	submenu->AddItem(fMActualSize);

	fMFitWidth = new BMenuItem(B_TRANSLATE("Fit width"), new BMessage(M_FIT_WIDTH));
	submenu->AddItem(fMFitWidth);

	fMFitHeight = new BMenuItem(B_TRANSLATE("Fit height"), new BMessage(M_FIT_HEIGHT));
	submenu->AddItem(fMFitHeight);

	menu->AddItem(submenu);

	menu->AddSeparatorItem();

	fMNext = new BMenuItem(B_TRANSLATE("Next file in folder"), new BMessage(M_NEXT_IMAGE));
	menu->AddItem(fMNext);

	fMPrevious = new BMenuItem(B_TRANSLATE("Previous file in folder"), new BMessage(M_PREV_IMAGE));
	menu->AddItem(fMPrevious);

	fMFirst = new BMenuItem(B_TRANSLATE("First file in folder"), new BMessage(M_FIRST_IMAGE));
	menu->AddItem(fMFirst);

	fMLast = new BMenuItem(B_TRANSLATE("Last file in folder"), new BMessage(M_LAST_IMAGE));
	menu->AddItem(fMLast);

	fMRandom = new BMenuItem(B_TRANSLATE("Random file in folder"), new BMessage(M_RANDOM_IMAGE));
	menu->AddItem(fMRandom);

	menu->AddSeparatorItem();

	menu->AddItem(new BMenuItem(B_TRANSLATE("Zoom in"), new BMessage(M_ZOOM_IN), '+'));
	menu->AddItem(new BMenuItem(B_TRANSLATE("Zoom out"), new BMessage(M_ZOOM_OUT), '-'));

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
	settings.AddBool("alwaysOnTop", _AlwaysOnTop());
	settings.AddInt32("undoSteps", fUndoSteps);
	settings.AddInt32("scaleMode", fImageView->getScaleMode());
	settings.AddUInt32("translatorId", (uint32)fSelectedTranslator);
	settings.AddUInt32("selectedFormat", fSelectedFormat);
	settings.AddInt32("selectedFormatIndex", fSelectedFormatIndex);

	if (status == B_OK)
		status = settings.Flatten(&file);

	return status;
}


void
MainWindow::MenusBeginning()
{
	fMSave->SetEnabled(fHasImage);
	fMSaveAs->SetEnabled(fHasImage);
	fMInformation->SetEnabled(fHasImage);
	fMRotate90CW->SetEnabled(fHasImage);
	fMRotate90CCW->SetEnabled(fHasImage);
	fMFlipVertical->SetEnabled(fHasImage);
	fMFlipHorizontal->SetEnabled(fHasImage);
	fMConvertToGrayscale->SetEnabled(fHasImage);
	fMSwapColors->SetEnabled(fHasImage);
	fMInvertColors->SetEnabled(fHasImage);
	fMShowChannel->SetEnabled(fHasImage);
	fMReopen->SetEnabled(fHasImage);
	fMDeleteImage->SetEnabled(fHasImage);
	fMAutoAdjust->SetEnabled(fHasImage);
	fMEffects->SetEnabled(fHasImage);
	fMSharpen->SetEnabled(fHasImage);

	fMUndo->SetEnabled(fImageView->CanUndo());
	fMRedo->SetEnabled(fImageView->CanRedo());

	fMFit->SetMarked(fImageView->getScaleMode() == SCALE_FIT_WINDOW);
	fMFitLarge->SetMarked(fImageView->getScaleMode() == SCALE_FIT_LARGE_ONLY);
	fMActualSize->SetMarked(fImageView->getScaleMode() == SCALE_ORIGINAL);
	fMFitWidth->SetMarked(fImageView->getScaleMode() == SCALE_FIT_WIDTH);
	fMFitHeight->SetMarked(fImageView->getScaleMode() == SCALE_FIT_HEIGHT);
}


void
MainWindow::_LoadImage(const entry_ref& ref)
{
	fImageView->ClearHistory();
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
	fImageView->SaveState();

	BEntry entry(&ref);
	BEntry parent;
	if (entry.GetParent(&parent) == B_OK)
		parent.GetRef(&fLastSaveDir);

	bigtime_t end = system_time();
	fLoadTime = end - start;
	if (fInfoWindow && !fInfoWindow->IsHidden())
		_ShowImageInfo();

	_UpdateStatus();
}


static bool
_IsImageFile(const entry_ref& ref)
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


void
MainWindow::_LoadDirectory(const entry_ref& ref)
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
		[](const entry_ref& a, const entry_ref& b) { return BString(a.name) < BString(b.name); });

	// find current index
	fCurrentIndex = -1;
	for (size_t i = 0; i < fFileList.size(); ++i) {
		if (fFileList[i] == ref) {
			fCurrentIndex = i;
			break;
		}
	}
}


void
MainWindow::_LoadImageAtIndex(int32 index)
{
	if (index < 0 || index >= (int32)fFileList.size())
		return;

	fCurrentIndex = index;
	_LoadImage(fFileList[index]);
}


void
MainWindow::NextImage()
{
	if (fFileList.empty())
		return;

	int32 next = fCurrentIndex + 1;
	if (next >= (int32)fFileList.size())
		next = 0; // wrap around

	_LoadImageAtIndex(next);
}


void
MainWindow::PrevImage()
{
	if (fFileList.empty())
		return;

	int32 prev = fCurrentIndex - 1;
	if (prev < 0)
		prev = fFileList.size() - 1; // wrap around

	_LoadImageAtIndex(prev);
}


void
MainWindow::FirstImage()
{
	if (fFileList.empty())
		return;

	_LoadImageAtIndex(0);
}


void
MainWindow::LastImage()
{
	if (fFileList.empty())
		return;

	_LoadImageAtIndex(fFileList.size() - 1);
}


void
MainWindow::RandomImage()
{
	if (fFileList.empty())
		return;

	int32 index = rand() % fFileList.size();
	_LoadImageAtIndex(index);
}


void
MainWindow::DeleteCurrentImage()
{
	BAlert* alert = new BAlert("Confirm", "Delete this image permanently?", "Cancel", "Delete",
		nullptr, B_WIDTH_AS_USUAL, B_OFFSET_SPACING, B_WARNING_ALERT);
	alert->SetShortcut(0, B_ESCAPE);
	alert->SetDefaultButton(0);

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
		PostMessage(M_CLEAR_IMAGE);
		return;
	}

	// Stay at same index if possible
	if (fCurrentIndex >= (int32)fFileList.size())
		fCurrentIndex = fFileList.size() - 1;

	_LoadImageAtIndex(fCurrentIndex);
	_UpdateStatus();
}


void
MainWindow::_UpdateStatus()
{
	// Update title bar
	BString name = (fCurrentRef.name) ? fCurrentRef.name : (fHasImage ? "(unnamed)" : "(No image)");
	name << " - " << kApplicationName;
	SetTitle(name);
	// Update status bar
	fStatusView->Update(&fCurrentRef, fImageView, fCurrentIndex, fFileList.size());
	// Update toolbar
	fToolBar->SetActionEnabled(M_SAVE_FILE, fHasImage);
	fToolBar->SetActionEnabled(M_PREV_IMAGE, fHasImage);
	fToolBar->SetActionEnabled(M_NEXT_IMAGE, fHasImage);
	fToolBar->SetActionEnabled(M_DELETE_IMAGE, fHasImage);
	fToolBar->SetActionEnabled(M_SHOW_INFO, fHasImage);
	fToolBar->SetActionEnabled(B_UNDO, fHasImage && fImageView->CanUndo());
	fToolBar->SetActionEnabled(B_REDO, fHasImage && fImageView->CanRedo());
}


void
MainWindow::_ShowImageInfo()
{
	if (!fInfoWindow)
		fInfoWindow = new InfoWindow();

	BMessage msg(MSG_SET_INFO);

	auto AddSection = [&](const char* title) {
		BMessage item;
		item.AddString("type", "section");
		item.AddString("title", title);
		msg.AddMessage("item", &item);
	};

	auto AddRow = [&](const char* label, const BString& value) {
		BMessage item;
		item.AddString("type", "row");
		item.AddString("label", label);
		item.AddString("value", value);
		msg.AddMessage("item", &item);
	};

	BBitmap* bitmap = fImageView->Bitmap();

	if (!bitmap) {
		AddRow("Status:", "No image loaded");
	} else {

		// File
		if (fCurrentRef.name) {
			AddSection("File");

			AddRow("File name:", fCurrentRef.name);

			BEntry entry(&fCurrentRef);
			BPath path;
			entry.GetPath(&path);

			BEntry parent;
			if (entry.GetParent(&parent) == B_OK) {
				BPath parentPath;
				parent.GetPath(&parentPath);
				AddRow("Folder:", parentPath.Path());
			}

			AddRow("Full path:", path.Path());

			BNode node(&entry);
			BNodeInfo nodeInfo(&node);

			char mimeType[B_MIME_TYPE_LENGTH];
			if (nodeInfo.GetType(mimeType) == B_OK) {
				BMimeType mime(mimeType);

				char shortDesc[B_MIME_TYPE_LENGTH];
				char longDesc[256];

				if (mime.GetShortDescription(shortDesc) == B_OK) {
					if (mime.GetLongDescription(longDesc) == B_OK) {
						BString typeStr;
						typeStr << longDesc << " (" << mimeType << ")";
						AddRow("Type:", typeStr);
					} else {
						BString typeStr;
						typeStr << shortDesc << " (" << mimeType << ")";
						AddRow("Type:", typeStr);
					}
				} else {
					AddRow("Type:", mimeType);
				}
			}

			char diskBuf[32], memBuf[32];
			FormatSize(diskBuf, GetFileSize(&fCurrentRef));
			FormatSize(memBuf, bitmap->BitsLength());

			AddRow("Disk size:", diskBuf);
			AddRow("Memory size:", memBuf);

			time_t modTime;
			if (node.GetModificationTime(&modTime) == B_OK) {
				BString t(ctime(&modTime));
				t.Trim();
				AddRow("File date/time:", t);
			}
		}

		// Image
		AddSection("Image");

		int32 width = bitmap->Bounds().IntegerWidth() + 1;
		int32 height = bitmap->Bounds().IntegerHeight() + 1;

		AddRow("Original size:", FormatDimensions(width, height));

		float zoom = fImageView->EffectiveZoom();
		int32 currentW = width * zoom;
		int32 currentH = height * zoom;

		AddRow("Current size:", FormatDimensions(currentW, currentH));

		BString zoomStr;
		zoomStr.SetToFormat("%.2f%%", zoom * 100);
		AddRow("Zoom:", zoomStr);

		AddRow("Color space:", ColorSpaceToString(bitmap->ColorSpace()));

		BString loadStr;
		loadStr.SetToFormat("%.2f ms", fLoadTime / 1000.0);
		AddRow("Load time:", loadStr);

		if (!fFileList.empty() && fCurrentIndex >= 0) {
			BString idx;
			idx.SetToFormat("%d / %lu", fCurrentIndex + 1, fFileList.size());
			AddRow("Index:", idx);
		}
	}

	fInfoWindow->PostMessage(&msg);
	if (fInfoWindow->IsHidden())
		fInfoWindow->Show();
}


void
MainWindow::_SaveRequested(BMessage* message)
{
    entry_ref ref;
    const char* name;
    if (message->FindRef("directory", &ref) != B_OK
        || message->FindString("name", &name) != B_OK)
        return;

    BBitmap* bitmap = fImageView->Bitmap();
    if (bitmap == NULL)
        return;

    BDirectory directory(&ref);
    BEntry entry(&directory, name);
    BPath path(&entry);
    BFile file(path.Path(), B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
    if (file.InitCheck() != B_OK) {
        printf("Failed to open file for writing\n");
        return;
    }

    uint32 format = fSelectedFormat;
    if (_SaveBitmap(bitmap, file, format) == B_OK) {
        printf("Saved to: %s\n", path.Path());
        entry.GetRef(&fCurrentRef);
        fLastSaveDir = ref;
        _UpdateStatus();
    } else {
        printf("Save failed\n");
    }
}


void
MainWindow::_ShowSavePanel()
{
    if (fImageView->Bitmap() == NULL)
        return;

    if (fCurrentRef.name != NULL)
        fSavePanel->SetSaveText(fCurrentRef.name);

    if (fLastSaveDir.name != NULL) {
        BEntry entry(&fLastSaveDir);
        BDirectory dir(&entry);
        fSavePanel->SetPanelDirectory(&dir);
    }

    fSavePanel->Show();
	PostMessage(M_FORMAT_SELECTED);
}


status_t
MainWindow::_SaveBitmap(BBitmap* bitmap, BFile& file, uint32 format)
{
    BBitmapStream stream(bitmap);
    status_t result = BTranslatorRoster::Default()->Translate(
        &stream, NULL, NULL, &file, format);
    stream.DetachBitmap(&bitmap);
    return result;
}

void
MainWindow::_InitSavePanel()
{
    fSavePanel = new BFilePanel(
        B_SAVE_PANEL,
        new BMessenger(this),
        NULL,
        0,
        false,
        NULL,
        NULL,
        false
    );

    BWindow* win = fSavePanel->Window();
    win->Lock();

	BView* textView = win->FindView("text view");
	textView->SetFlags(textView->Flags() | B_FOLLOW_RIGHT);
    BView* root = win->ChildAt(0);

    // --- 1. Format menu ---
    BPopUpMenu* formatMenu = new BPopUpMenu("Format");
    BMessage msg(M_FORMAT_SELECTED);
    BTranslationUtils::AddTranslationItems(
        formatMenu,
        B_TRANSLATOR_BITMAP,
        &msg,
        NULL, NULL, NULL);
    formatMenu->SetTargetForItems(this);

    BMenuField* field = new BMenuField(
        BRect(10, textView->Frame().bottom + 10,
              300, textView->Frame().bottom + 40),
        "format",
        "Format:",
        formatMenu,
		B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);

    field->SetDivider(field->StringWidth("Format:") + 10);
    root->AddChild(field);

    // --- 2. Default format (PNG) ---
    for (int32 i = 0; i < formatMenu->CountItems(); i++) {
		BMenuItem* item = formatMenu->ItemAt(i);

		BMessage* msg = item->Message();
		if (!msg)
			continue;

		int32 type;
		int32 translator;

		if (msg->FindInt32("be:type", &type) == B_OK &&
			msg->FindInt32("be:translator", &translator) == B_OK) {

			if (type == B_PNG_FORMAT) {
				item->SetMarked(true);

				// Manually apply selection
				fSelectedFormat = type;
				fSelectedTranslator = translator;
				break;
			}
		}
	}

	int32 count = root->CountChildren();
	for (int32 i = 0; i < count; ++i) {
		BView* child = root->ChildAt(i);
		if ((child->ResizingMode() & B_FOLLOW_BOTTOM)
			|| !(child->ResizingMode() & B_FOLLOW_TOP_BOTTOM))
			child->MoveBy(0.0, -30.0);
		else if (child->ResizingMode() & B_FOLLOW_TOP_BOTTOM)
			child->ResizeBy(0.0, -30.0);
	}

    // --- 3. Settings button ---
    BView* cancel = win->FindView("cancel button");

    if (cancel) {
        BButton* settingsButton = new BButton(
            BRect(0,0,1,1),
            "settings",
            "Settings" B_UTF8_ELLIPSIS,
            new BMessage(M_SHOW_TRANSLATOR_SETTINGS),
			B_FOLLOW_RIGHT | B_FOLLOW_BOTTOM
        );

        settingsButton->ResizeToPreferred();
		settingsButton->SetTarget(this);
        root->AddChild(settingsButton);

        BRect r = cancel->Frame();
        settingsButton->MoveTo(
            r.left - settingsButton->Bounds().Width() - 10,
            r.top);

		if (textView && settingsButton) {
			BRect t = textView->Frame();
			BRect s = settingsButton->Frame();

			float newWidth = s.left - t.left - 10.0f;
			if (newWidth > 50) {
				textView->ResizeTo(newWidth, t.Height());
			}
		}
	}

    win->Unlock();
}


bool
MainWindow::QuitRequested(void)
{
	if (fSettingsWindow && fSettingsWindow->LockLooper()) {
		fSettingsWindow->Quit();
		fSettingsWindow = nullptr;
	}
	if (fInfoWindow && fInfoWindow->LockLooper()) {
		fInfoWindow->Quit();
		fInfoWindow = nullptr;
	}
	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}


void
MainWindow::_UpdateFilenameExtension()
{
    if (!fSavePanel)
        return;

	BWindow* win = fSavePanel->Window();
    if (!win)
        return;

    BTextControl* text = dynamic_cast<BTextControl*>(win->FindView("text view"));
    if (!text)
        return;

    BString name = text->Text();
    if (name.IsEmpty())
        return;

    // Remove existing extension
    int32 dot = name.FindLast('.');
    if (dot > 0)
        name.Truncate(dot);

    BString ext = _GetExtension();

    if (ext)
        name << "." << ext;

    fSavePanel->SetSaveText(name.String());
}


BString
MainWindow::_GetExtension()
{
    if (fSelectedTranslator < 0 || fSelectedFormatIndex < 0)
        return "img";

	const translation_format* formats = nullptr;
    int32 count = 0;

    if (BTranslatorRoster::Default()->GetOutputFormats(
            fSelectedTranslator, &formats, &count) != B_OK)
        return "img";

    if (fSelectedFormatIndex >= count)
        return "img";

    const translation_format& fmt = formats[fSelectedFormatIndex];

    // Prefer MIME extension
    BString mime(fmt.MIME);
	BMimeType mimeType(mime.String());
	BMessage extensions;
	BString ext;

	mimeType.GetFileExtensions(&extensions);
	extensions.FindString("extensions", 0, &ext);

	if (ext.Length()) {
		// Normalize common extensions
		if (ext == "jpeg") return "jpg";
		if (ext == "tiff") return "tif";

		return ext;
	}
	// Fallback if unable to get extension from translator
    mime.ToLower();

    if (mime.FindFirst("image/png") == 0) return "png";
    if (mime.FindFirst("image/jpeg") == 0) return "jpg";
    if (mime.FindFirst("image/bmp") == 0) return "bmp";
	if (mime.FindFirst("image/x-be-bitmap") == 0) return "bmp";
    if (mime.FindFirst("image/gif") == 0) return "gif";
    if (mime.FindFirst("image/tiff") == 0) return "tif";
    if (mime.FindFirst("image/x-tga") == 0) return "tga";

    // Modern Web Formats
    if (mime.FindFirst("image/webp") == 0) return "webp";
    if (mime.FindFirst("image/svg+xml") == 0) return "svg";
    if (mime.FindFirst("image/avif") == 0) return "avif";
	if (mime.FindFirst("image/qoi") == 0) return "qoi";

    // Apple / High Efficiency
    if (mime.FindFirst("image/heic") == 0) return "heic";
    if (mime.FindFirst("image/heif") == 0) return "heif";

    // Specialized & Vendor
    if (mime.FindFirst("image/vnd.adobe.photoshop") == 0) return "psd";
    if (mime.FindFirst("image/x-icon") == 0) return "ico";
    if (mime.FindFirst("image/vnd.microsoft.icon") == 0) return "ico";
    if (mime.FindFirst("image/x-portable-anymap") == 0) return "pnm";
    if (mime.FindFirst("image/vnd.wap.wbmp") == 0) return "wbmp";

    return "img";
}
