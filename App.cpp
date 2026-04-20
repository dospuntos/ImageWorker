/*
 * Copyright 2026, Johan Wagenheim
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "App.h"
#include "Constants.h"

#include <AboutWindow.h>
#include <Catalog.h>
#include <time.h>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Application"


App::App()
	:
	BApplication(kApplicationSignature)
{
	fMainWindow = new MainWindow();
	fMainWindow->Show();
	srand(time(NULL)); // For random generator
}


App::~App()
{
}


void
App::AboutRequested()
{
	BAboutWindow* about
		= new BAboutWindow(B_TRANSLATE_SYSTEM_NAME("ImageWorker"), kApplicationSignature);
	about->AddDescription(B_TRANSLATE("A simple but powerfull image viewer and editor. Inspired by IrfanView."));
	about->AddCopyright(2026, "Johan Wagenheim");
	about->Show();
}


int
main()
{
	App* app = new App();
	app->Run();
	delete app;
	return 0;
}
