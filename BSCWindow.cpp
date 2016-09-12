#include "AdvancedOptionsView.h"
#include "BSCApp.h"
#include "BSCWindow.h"
#include "CamStatusView.h"
#include "Controller.h"
#include "ControllerObserver.h"
#include "InfoView.h"
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

const static BRect kWindowRect(0, 0, 400, 900);

const char* kEncodingString = "Encoding movie...";
const char* kDoneString = "Done!";

BSCWindow::BSCWindow()
	:
	BDirectWindow(kWindowRect, "BeScreenCapture", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS|B_AUTO_UPDATE_SIZE_LIMITS),
	fController(dynamic_cast<Controller*>(gControllerLooper)),
	fCapturing(false)
{
	OutputView* outputView = new OutputView(fController);
	AdvancedOptionsView* advancedView = new AdvancedOptionsView(fController);
	InfoView* infoView = new InfoView(fController);
	
	fStartStopButton = new BButton("Start", "Start Recording",
		new BMessage(kMsgGUIStartCapture)); 
	
	fStartStopButton->SetTarget(fController);
	fStartStopButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	
	fCardLayout = new BCardLayout();
	BView* cardsView = new BView("status", 0, fCardLayout);
	cardsView->SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fCardLayout->AddView(fCamStatus = new CamStatusView(fController));
	fCamStatus->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
		
	BView* statusView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fStringView = new BStringView("stringview", kEncodingString))
			.Add(fStatusBar = new BStatusBar("", ""))
		.End()
		.View();
	
	statusView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));	
	fStatusBar->SetExplicitMinSize(BSize(100, 20));
	fCardLayout->AddView(statusView);
	
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.AddGroup(B_VERTICAL, 1)
		.SetInsets(B_USE_DEFAULT_SPACING, 1,
			B_USE_DEFAULT_SPACING, 1)
			.Add(fTabView = new BTabView("Tab View", B_WIDTH_FROM_LABEL))
			.AddGroup(B_HORIZONTAL)
				.Add(cardsView)
				.Add(fStartStopButton)
			.End()
		.End();
	
	fCardLayout->SetVisibleItem((int32)0);
				
	BGroupView* outputGroup = new BGroupView(B_HORIZONTAL);
	outputGroup->SetName("Capture");
	outputGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	fTabView->AddTab(outputGroup);
	BLayoutBuilder::Group<>(outputGroup)
		.Add(outputView);
								
	BGroupView* advancedGroup = new BGroupView(B_HORIZONTAL);
	advancedGroup->SetName("Options");
	advancedGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	fTabView->AddTab(advancedGroup);
	BLayoutBuilder::Group<>(advancedGroup)
		.Add(advancedView);

	BGroupView* infoGroup = new BGroupView(B_HORIZONTAL);
	infoGroup->SetName("Info");
	infoGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	fTabView->AddTab(infoGroup);
	BLayoutBuilder::Group<>(infoGroup)
		.Add(infoView);
						
	if (fController->LockLooper()) {
		// watch Controller for these
		fController->StartWatching(this, B_UPDATE_STATUS_BAR);
		fController->StartWatching(this, B_RESET_STATUS_BAR);
		fController->StartWatching(this, kMsgControllerEncodeStarted);
		fController->StartWatching(this, kMsgControllerEncodeProgress);
		fController->StartWatching(this, kMsgControllerEncodeFinished);
		fController->StartWatching(this, kMsgControllerTargetFrameChanged);
		fController->StartWatching(this, kMsgControllerCaptureStarted);
		fController->StartWatching(this, kMsgControllerCaptureStopped);
		fController->StartWatching(this, kMsgControllerCaptureFailed);
		fController->StartWatching(this, kMsgControllerSelectionWindowClosed);
				
		fController->UnlockLooper();
	}
	
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
	bool canQuit = fController->CanQuit();
	if (canQuit)
		be_app->PostMessage(B_QUIT_REQUESTED);
	
	return canQuit && BWindow::QuitRequested();
}


bool
BSCWindow::IsRecording()
{
	return fCapturing;
}


void
BSCWindow::MessageReceived(BMessage *message)
{
	switch (message->what) {				
		case kSelectArea:
		case kSelectWindow:
		{
			Hide();
			while (!IsHidden())
				snooze(500);
			snooze(2000);
			BMessenger messenger(fController);
			int mode = message->what == kSelectArea ? SelectionWindow::REGION : SelectionWindow::WINDOW;
			SelectionWindow *window = new SelectionWindow(mode, messenger, kSelectionWindowClosed);			
			window->Show();
			break;
		}
				
		case kCmdToggleRecording:
			fStartStopButton->Invoke();
			break;
		
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
	
				case kMsgControllerSelectionWindowClosed:
				{
					if (IsHidden())
						Show();
			
					break;
				}
				case kMsgControllerCaptureFailed:
				{
					status_t status;
					message->FindInt32("status", &status);
					char errorString[128];
					snprintf(errorString, 128, "A problem has occurred while starting capture:\n"
						"%s", strerror(status));
					(new BAlert("Capture Failed", errorString, "Ok"))->Go();
					break;
				}
				case kMsgControllerCaptureStarted:
					_CaptureStarted();
					break;
	
				case kMsgControllerCaptureStopped:
					_CaptureFinished();
					break;
	
				case kMsgControllerEncodeStarted:
					fStringView->SetText(kEncodingString);
					fCardLayout->SetVisibleItem(1);
					break;
				case kMsgControllerEncodeProgress:
				{
					int32 numFiles = 0;
					message->FindInt32("num_files", &numFiles);					
					fStatusBar->SetMaxValue(float(numFiles));
					
					BString string = kEncodingString;
					string << " (" << numFiles << " frames)";
					fStringView->SetText(string);
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
			break;
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
		Hide();
	
	fCardLayout->SetVisibleItem((int32)0);
	fStatusBar->Reset();
	
	fStartStopButton->SetLabel("Stop Recording");

	SendNotices(kMsgGUIStartCapture);
	
	return B_OK;
}


status_t
BSCWindow::_CaptureFinished()
{
	fCapturing = false;

	fStartStopButton->SetEnabled(false);
	fStartStopButton->SetLabel("Start Recording");

	SendNotices(kMsgGUIStopCapture);	

	if (IsHidden())
		Show();
	if (IsMinimized())
		Minimize(false);

	return B_OK;
}
