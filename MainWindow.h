/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include "ImageView.h"
#include "InfoWindow.h"
#include "StatusView.h"
#include "Toolbar.h"
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <MimeType.h>
#include <StringView.h>
#include <TextControl.h>
#include <TranslationDefs.h>
#include <Window.h>
#include <vector>

class MainWindow : public BWindow {
public:
							MainWindow();
	virtual					~MainWindow();
	bool					QuitRequested(void);
	virtual void			MessageReceived(BMessage* msg);
	void 					MenusBeginning();
	bool					closeOnEscape() { return fCloseOnEscape; }
	int						undoSteps() { return fUndoSteps; }
	bool					fHasImage;
	BMimeType				fMimeType;

private:
			BMenuBar*		_BuildMenu();
			void			_UpdateOpenWithMenu(BMenu* menu);
			ImageView*		fImageView;
			StatusView*		fStatusView;
			BToolBar*		fToolBar;
			InfoWindow*		fInfoWindow;

			status_t		_LoadSettings(BMessage& settings);
			status_t		_SaveSettings();
			void			_LoadImage(const entry_ref& ref);
			void			_UpdateStatus();
			void			_ShowImageInfo();

			BMenuItem*		fMSave;
			BMenuItem*		fMSaveAs;
			BFilePanel*		fOpenPanel;
			BFilePanel*		fSavePanel;
			translator_id	fSelectedTranslator = -1;
			uint32			fSelectedFormat;
			int32			fSelectedFormatIndex;
			void			_SaveRequested(BMessage* message);
			void			_ShowSavePanel();
			status_t		_SaveBitmap(BBitmap* bitmap, BFile& file, uint32 format);
			void			_InitSavePanel();
			BString			_GetExtension();
			void			_UpdateFilenameExtension();

			std::vector<entry_ref> fFileList;
			int32			fCurrentIndex = -1;
			entry_ref		fCurrentRef;
			entry_ref		fLastSaveDir;
			bigtime_t		fLoadTime;


			// navigation
			void			_LoadDirectory(const entry_ref& ref);
			void			_LoadImageAtIndex(int32 index);
			void			NextImage();
			void			PrevImage();
			void			FirstImage();
			void			LastImage();
			void			RandomImage();
			void			DeleteCurrentImage();

			// settings
			BWindow*		fSettingsWindow;
			bool			fCloseOnEscape;
			int32			fUndoSteps;
			bool			_AlwaysOnTop() {return Feel() == B_FLOATING_ALL_WINDOW_FEEL;};

			// menu items
			BMenuItem*		fMReopen;
			BMenu*	 		fOpenWithMenu;
			BMenuItem*		fMDeleteImage;
			BMenuItem*		fMUndo;
			BMenuItem*		fMRedo;
			BMenuItem*		fMRotate90CW;
			BMenuItem*		fMRotate90CCW;
			BMenuItem* 		fMFlipVertical;
			BMenuItem*		fMFlipHorizontal;
			BMenuItem*		fMConvertToGrayscale;
			BMenu* 			fMSwapColors;
			BMenuItem*		fMInvertColors;
			BMenuItem*		fMInformation;
			BMenuItem* 		fMAlwaysOnTop;
			BMenu*			fMShowChannel;
			BMenuItem*		fMZoomIn;
			BMenuItem*		fMZoomOut;
			BMenuItem*		fMFit;
			BMenuItem*		fMFitLarge;
			BMenuItem*		fMActualSize;
			BMenuItem* 		fMFitWidth;
			BMenuItem*		fMFitHeight;
			BMenuItem*		fMNext;
			BMenuItem*		fMPrevious;
			BMenuItem*		fMFirst;
			BMenuItem*		fMLast;
			BMenuItem*		fMRandom;
			BMenu*			fMEffects;
			BMenuItem*		fMAutoAdjust;
			BMenuItem*		fMSharpen;
};

class SavePanelView : public BView {
public:
    SavePanelView(BMenuField* field)
        : BView("save_panel_view", B_WILL_DRAW)
    {
        SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

        BLayoutBuilder::Group<>(this, B_VERTICAL)
            .SetInsets(10)
            .Add(field);
    }
};
#endif
