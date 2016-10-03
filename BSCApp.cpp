#include "BSCApp.h"
#include "BSCWindow.h"
#include "Controller.h"
#include "DeskbarControlView.h"
#include "Settings.h"

#include <private/interface/AboutWindow.h>
#include <Deskbar.h>

#include <stdio.h>


int
main()
{
	BSCApp app;
	app.Run();
	
	return 0;
}


BSCApp::BSCApp()
	:
	BApplication(kAppSignature),
	fWindow(NULL)
{
	Settings::Load();

	fShouldStartRecording = false;
	gControllerLooper = new Controller();
}


BSCApp::~BSCApp()
{
	BDeskbar().RemoveItem("BSC Control");
	Settings::Save();
	
	gControllerLooper->Lock();
	gControllerLooper->Quit();
}


void
BSCApp::ReadyToRun()
{
	try {
		fWindow = new BSCWindow();
	} catch (...) {
		PostMessage(B_QUIT_REQUESTED);
		return;	
	}
	
	if (fShouldStartRecording) {
		fWindow->Run();
		BMessenger(fWindow).SendMessage(kCmdToggleRecording);
	} else
		fWindow->Show();

	
	BDeskbar deskbar;
	if (deskbar.IsRunning()) { 
		while (deskbar.HasItem("BSC Control"))
			deskbar.RemoveItem("BSC Control");
		if (!Settings().HideDeskbarIcon()) {
			deskbar.AddItem(new DeskbarControlView(BRect(0, 0, 15, 15),
				"BSC Control"));
		}
	}
}


bool
BSCApp::QuitRequested()
{
	return BApplication::QuitRequested();
}


void
BSCApp::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kCmdToggleRecording:
			if (fWindow != NULL)
				BMessenger(fWindow).SendMessage(message);
			else {
				// Start recording as soon as a window is created
				fShouldStartRecording = true;
			}
			break;
		default:
			BApplication::MessageReceived(message);
			break;
	}
}


void
BSCApp::AboutRequested()
{
	BAboutWindow* aboutWindow = new BAboutWindow("BeScreenCapture", kAppSignature);
	aboutWindow->AddDescription("BeScreenCapture lets you record what happens on your screen and save it to a clip in any media format supported by Haiku.");
	
	aboutWindow->Show();
}

