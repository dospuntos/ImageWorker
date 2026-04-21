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
#include <MenuBar.h>
#include <MenuItem.h>
#include <StringView.h>
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

private:
			BMenuBar*		_BuildMenu();
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

			std::vector<entry_ref> fFileList;
			int32			fCurrentIndex = -1;
			entry_ref		fCurrentRef;
			entry_ref		fLastSaveDir;
			bool			fHasImage;
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
};

#endif
