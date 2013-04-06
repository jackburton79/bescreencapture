#include "AdvancedOptionsView.h"
#include "BSCWindow.h"
#include "CamStatusView.h"
#include "Controller.h"
#include "ControllerObserver.h"
#include "messages.h"
#include "OutputView.h"
#include "SelectionWindow.h"
#include "Settings.h"
#include "Utils.h"

#include <Alert.h>
#include <Application.h>
#include <CardLayout.h>
#include <Box.h>
#include <Button.h>
#include <Debug.h>
#include <GroupLayoutBuilder.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <Screen.h>
#include <StatusBar.h>
#include <String.h>
#include <StringView.h>
#include <TabView.h>


#include <cstdio>
#include <cstdlib>

const static BRect kWindowRect(0, 0, 400, 500);

enum {
	kMsgGUIToggleCapture = 0x10000
};


BSCWindow::BSCWindow()
	:
	BDirectWindow(kWindowRect, "BeScreenCapture", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS|B_AUTO_UPDATE_SIZE_LIMITS),
	fController(dynamic_cast<Controller*>(gControllerLooper)),
	fCapturing(false)
{
	OutputView *outputView
		= new OutputView(fController);
	AdvancedOptionsView *advancedView 
		= new AdvancedOptionsView(fController);
	
	fStartStopButton = new BButton("Start", "Start Recording",
		new BMessage(kMsgGUIStartCapture)); 
	
	fStartStopButton->SetTarget(fController);
	
	fCardLayout = new BCardLayout();
	BView* cardsView = new BView("status", 0, fCardLayout);
	
	fCardLayout->AddView(fCamStatus = new CamStatusView("CamStatusView"));
	fCamStatus->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
	
	const char *kString = "Encoding movie...";
	BView* statusView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fStringView = new BStringView("stringview", kString))
			.Add(fStatusBar = new BStatusBar("", ""))
		.End()
		.View();
		
	fStatusBar->SetExplicitMinSize(BSize(60, 20));
	fCardLayout->AddView(statusView);
	
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fTabView = new BTabView("Tab View", B_WIDTH_FROM_LABEL))
			.AddGroup(B_HORIZONTAL)
				.Add(cardsView)
				.AddGlue(1)
				.Add(fStartStopButton)
			.End()
		.End();
	
	fCardLayout->SetVisibleItem((int32)0);
				
	BGroupView* outputGroup = new BGroupView(B_HORIZONTAL);
	outputGroup->SetName("Output");
	outputGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	fTabView->AddTab(outputGroup);
	BLayoutBuilder::Group<>(outputGroup)
		.Add(outputView);
							
	BGroupView* advancedGroup = new BGroupView(B_HORIZONTAL);
	advancedGroup->SetName("Advanced Options");
	advancedGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	fTabView->AddTab(advancedGroup);
	BLayoutBuilder::Group<>(advancedGroup)
		.Add(advancedView);
				
	if (fController->LockLooper()) {	
		// controller should watch for these messages
		//StartWatching(fController, kMsgGUIStartCapture);
		//StartWatching(fController, kMsgGUIStopCapture);
		StartWatching(fController, kSelectionWindowClosed);
		//StartWatching(fCamStatus, kMsgControllerCaptureResumed);	
		
		// watch Controller for these
		fController->StartWatching(this, B_UPDATE_STATUS_BAR);
		fController->StartWatching(this, B_RESET_STATUS_BAR);
		fController->StartWatching(this, kMsgControllerEncodeStarted);
		fController->StartWatching(this, kMsgControllerEncodeProgress);
		fController->StartWatching(this, kMsgControllerEncodeFinished);
		fController->StartWatching(this, kMsgControllerAreaSelectionChanged);
		fController->StartWatching(this, kMsgControllerCaptureStarted);
		fController->StartWatching(this, kMsgControllerCaptureStopped);
		
		fController->StartWatching(fCamStatus, kMsgControllerCaptureStarted);
		fController->StartWatching(fCamStatus, kMsgControllerCaptureStopped);
		fController->StartWatching(fCamStatus, kMsgControllerCapturePaused);
		fController->StartWatching(fCamStatus, kMsgControllerCaptureResumed);
		
		fController->StartWatching(outputView, kMsgControllerAreaSelectionChanged);
			
		fController->UnlockLooper();
	}
	
	StartWatching(outputView, kSelectionWindowClosed);

	CenterOnScreen();
}


BSCWindow::~BSCWindow()
{
}


void
BSCWindow::DispatchMessage(BMessage *message, BHandler *handler)
{
	BDirectWindow::DispatchMessage(message, handler);
}


bool
BSCWindow::QuitRequested()
{
	bool canQuit = !fCapturing;
	if (canQuit)
		be_app->PostMessage(B_QUIT_REQUESTED);
	
	return canQuit && BWindow::QuitRequested();
}


void
BSCWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {				
		case kSelectArea:
		{
			Minimize(true);
			BMessenger messenger(this);
			SelectionWindow *window = new SelectionWindow(messenger, kSelectionWindowClosed);			
			window->Show();
			break;
		}
		
		case kSelectionWindowClosed:
		{
			if (IsMinimized())
				Minimize(false);
				
			SendNotices(kSelectionWindowClosed, message);
			break;
		}		
		
		
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32("be:observe_change_what", &code);
			switch (code) {		
				case B_UPDATE_STATUS_BAR:
				case B_RESET_STATUS_BAR:
				{
					BMessage newMessage(*message);
					message->what = code;
					PostMessage(message, fStatusBar);
					break;
				}
	
				case kMsgControllerCaptureStarted:
					_CaptureStarted();
					break;
	
				case kMsgControllerCaptureStopped:
					_CaptureFinished();
					break;
	
				case kMsgControllerEncodeStarted:
					fCardLayout->SetVisibleItem(1);

					break;
					
				case kMsgControllerEncodeProgress:
				{
					int32 numFiles = 0;
					message->FindInt32("num_files", &numFiles);
					
					fStatusBar->SetMaxValue(float(numFiles));
					
					break;
				}
		
				case kMsgControllerEncodeFinished:
				{
					fStartStopButton->SetEnabled(true);
					fCardLayout->SetVisibleItem((int32)0);
					
					status_t status = B_OK;
					if (message->FindInt32("status", (int32*)&status) == B_OK
						&& status != B_OK) {
						char errorString[128];
						snprintf(errorString, 128, "A problem has occurred:\n"
							"%s", strerror(status));
						(new BAlert("yo", errorString, "Ok"))->Go();
					}
					
					break;
				}
				default:
					break;
			}
		}
		

		default:
			BWindow::MessageReceived(message);
			break;
	}
}


void
BSCWindow::ScreenChanged(BRect screen_size, color_space depth)
{
	
}


void
BSCWindow::DirectConnected(direct_buffer_info *info)
{
	switch (info->buffer_state & B_DIRECT_MODE_MASK) {
		case B_DIRECT_START:
		case B_DIRECT_MODIFY:
			fController->UpdateDirectInfo(info);
			break;
		case B_DIRECT_STOP:
			break;
		default:
			break;
	}
}


status_t
BSCWindow::_CaptureStarted()
{
	fCapturing = true;
	
	Settings settings;
						
	if (settings.MinimizeOnRecording())
		Minimize(true);
	
	fStatusBar->Reset();
	
	fStartStopButton->SetLabel("Stop Recording");

	SendNotices(kMsgGUIStartCapture);
	
	return B_OK;
}


status_t
BSCWindow::_CaptureFinished()
{
	fCapturing = false;
	
	if (IsMinimized())
		Minimize(false);
				
	fStartStopButton->SetEnabled(false);
	fStartStopButton->SetLabel("Start Recording");

	SendNotices(kMsgGUIStopCapture);	

	return B_OK;
}
