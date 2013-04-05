#include "BSCApp.h"
#include "BSCWindow.h"
#include "Controller.h"
#include "DeskbarControlView.h"
#include "Settings.h"

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
	BApplication(kAppSignature)
{
	gControllerLooper = new Controller();
	Settings::Load();
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
	
	fWindow->Show();
	
	BDeskbar deskbar;
	if (deskbar.IsRunning()) { 
		while (deskbar.HasItem("BSC Control"))
			deskbar.RemoveItem("BSC Control");
		
		deskbar.AddItem(new DeskbarControlView(BRect(0, 0, 15, 15),
			"BSC Control"));
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
		default:
			BApplication::MessageReceived(message);
			break;
	}
}
