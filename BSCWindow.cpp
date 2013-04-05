#include "AdvancedOptionsView.h"
#include "BSCWindow.h"
#include "CamStatusView.h"
#include "Controller.h"
#include "ControllerObserver.h"
#include "messages.h"
#include "OutputView.h"
#include "PostProcessingView.h"
#include "SelectionWindow.h"
#include "Settings.h"
#include "Utils.h"

#include <Alert.h>
#include <Application.h>
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

const static BRect kWindowRect(0, 0, 600, 500);

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
	PostProcessingView *ppView 
		= new PostProcessingView("Post Processing");
	AdvancedOptionsView *advancedView 
		= new AdvancedOptionsView(fController);
	
	fStartStopButton = new BButton("Start", "Start Recording",
		new BMessage(kMsgGUIStartCapture)); 
	
	fStartStopButton->SetTarget(fController);
	
	const char *kString = "Encoding movie...";			
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fTabView = new BTabView("Tab View", B_WIDTH_FROM_LABEL))
			.AddGroup(B_HORIZONTAL)
				.Add(fCamStatus = new CamStatusView("CamStatusView"))
				.Add(fStringView = new BStringView("stringview", kString))
				.Add(fStatusBar = new BStatusBar("", ""))
				.AddGlue(1)
				.Add(fStartStopButton)
			.End()
		.End();
				
	BGroupView* outputGroup = new BGroupView(B_HORIZONTAL);
	outputGroup->SetName("Output");
	outputGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	fTabView->AddTab(outputGroup);
	BLayoutBuilder::Group<>(outputGroup)
		.Add(outputView);
					
	BGroupView* postGroup = new BGroupView(B_HORIZONTAL);
	postGroup->SetName("Post Processing");
	postGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	fTabView->AddTab(postGroup);
	BLayoutBuilder::Group<>(postGroup)
		.Add(ppView);
		
	BGroupView* advancedGroup = new BGroupView(B_HORIZONTAL);
	advancedGroup->SetName("Advanced Options");
	advancedGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	fTabView->AddTab(advancedGroup);
	BLayoutBuilder::Group<>(advancedGroup)
		.Add(advancedView);
		
	fStatusBar->Hide();
	fStringView->Hide();
		
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
				/*case kMsgControllerAreaSelectionChanged:
					if (IsMinimized())
						Minimize(false);
					break;
				*/	
				case kMsgControllerCaptureStarted:
					_CaptureStarted();
					break;
	
				case kMsgControllerCaptureStopped:
					_CaptureFinished();
					break;
	
				case kMsgControllerEncodeStarted:
					fStringView->Show();
					fStatusBar->Show();
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
					fStringView->Hide();
					fStatusBar->Hide();
					
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
	printf("finished\n");
	fCapturing = false;
	
	if (IsMinimized())
		Minimize(false);
				
	fStartStopButton->SetEnabled(false);
	fStartStopButton->SetLabel("Start Recording");

	SendNotices(kMsgGUIStopCapture);	

	return B_OK;
}
