/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <FilePanel.h>
#include <vector>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Window.h>
#include "ImageView.h"
#include "StatusView.h"
#include "Toolbar.h"
#include <StringView.h>

class MainWindow : public BWindow
{
public:
							MainWindow();
	virtual					~MainWindow();

	virtual void			MessageReceived(BMessage* msg);
	void					_ToggleScaleMode();

private:
			BMenuBar*		_BuildMenu();
			ImageView*		fImageView;
			StatusView*		fStatusView;
			BToolBar*		fToolBar;

			status_t		_LoadSettings(BMessage& settings);
			status_t		_SaveSettings();
			void			_LoadImage(const entry_ref& ref);
			void			_UpdateStatus();
			void			_ShowImageInfo();

			BMenuItem*		fSaveMenuItem;
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
			void			DeleteCurrentImage();
};

#endif
