#include "AdvancedOptionsView.h"
#include "BSCApp.h"
#include "BSCWindow.h"
#include "CamStatusView.h"
#include "Controller.h"
#include "ControllerObserver.h"
#include "InfoView.h"
#include "Messages.h"
#include "OutputView.h"
#include "OptionsWindow.h"
#include "SelectionWindow.h"
#include "Settings.h"
#include "Utils.h"

#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Debug.h>
#include <Entry.h>
#include <GroupLayoutBuilder.h>
#include <GroupView.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <StringView.h>
#include <TabView.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>

const static BRect kWindowRect(0, 0, 400, 600);

const static uint32 kGUIOpenMediaWindow = 'j89d';
const static uint32 kGUIDockWindow = 'j90d';
const static uint32 kGUIResetSettings = 'j91d';

BSCWindow::BSCWindow()
	:
	BDirectWindow(kWindowRect, "BeScreenCapture", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS),
	fController(dynamic_cast<Controller*>(gControllerLooper)),
	fMenuBar(NULL),
	fStartStopButton(NULL),
	fCamStatus(NULL),
	fOutputView(NULL),
	fAdvancedOptionsView(NULL),
	fInfoView(NULL),
	fCapturing(false)
{
	fOutputView = new OutputView(fController);
	fAdvancedOptionsView = new AdvancedOptionsView(fController);
	fInfoView = new InfoView(fController);
	
	fMenuBar = new BMenuBar("menubar");
	_BuildMenu();
	
	fStartStopButton = new BButton("Start", "Start Recording",
		new BMessage(kMsgGUIToggleCapture));
	
	fStartStopButton->SetTarget(fController);
	fStartStopButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	
	fCamStatus = new CamStatusView(fController);
	fCamStatus->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
	
	_LayoutWindow(false);

	if (fController->LockLooper()) {
		// watch Controller for these
		fController->StartWatching(this, kMsgControllerEncodeStarted);
		fController->StartWatching(this, kMsgControllerEncodeProgress);
		fController->StartWatching(this, kMsgControllerEncodeFinished);
		fController->StartWatching(this, kMsgControllerTargetFrameChanged);
		fController->StartWatching(this, kMsgControllerCaptureStarted);
		fController->StartWatching(this, kMsgControllerCaptureStopped);
		fController->StartWatching(this, kMsgControllerSelectionWindowClosed);
				
		fController->UnlockLooper();
	}
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
	BString reason;
	bool canQuit = fController->CanQuit(reason);
	if (!canQuit) {
		BString text = "Really Quit?";
		text.Append(" ");
		text.Append(reason);
		BAlert* alert = new BAlert("Really Quit?", text, "Yes", "No");
		int32 result = alert->Go();
		if (result == 0) {
			fController->Cancel();
			canQuit = true;
		}
	}
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
		case B_ABOUT_REQUESTED:
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;

		case kGUIOpenMediaWindow:
			(new OptionsWindow(fController))->Show();
			break;
			
		case kGUIDockWindow:
			_LayoutWindow(!Settings().DockingMode());
			break;

		case kGUIResetSettings:
			fController->ResetSettings();
			break;
			
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

		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32("be:observe_change_what", &code);
			switch (code) {		
				case kMsgControllerSelectionWindowClosed:
				{
					if (IsHidden())
						Show();
			
					break;
				}

				case kMsgControllerCaptureStarted:
					_CaptureStarted();
					break;
	
				case kMsgControllerCaptureStopped:
				{
					_CaptureFinished();
					
					status_t status = B_OK;
					message->FindInt32("status", &status);
					
					if (status != B_OK) {
						char errorString[128];
						snprintf(errorString, 128, "Could not record clip:\n"
							"%s", strerror(status));
						(new BAlert("Capture Failed", errorString, "Ok"))->Go();
						fStartStopButton->SetEnabled(true);
					}
										
					break;
				}
				case kMsgControllerEncodeStarted:
					fStartStopButton->SetEnabled(false);			
					break;
		
				case kMsgControllerEncodeFinished:
				{
					fStartStopButton->SetEnabled(true);
					status_t status = B_OK;
					if (message->FindInt32("status", (int32*)&status) == B_OK
						&& status != B_OK) {
						BString errorString;
						errorString.SetToFormat("Could not create clip: "
							"%s", strerror(status));
						(new BAlert("Encoding Failed", errorString.String(), "Ok"))->Go();
					} else {
						// TODO: Should be asynchronous
						BString successString;
						const char* destName = NULL;
						message->FindString("file_name", &destName);
						BEntry entry(destName);
						if (entry.Exists()) {
							if (entry.IsDirectory()) {
								successString.SetTo("Do you want to open the folder in Tracker?");
							} else {
								successString.SetTo("Do you want to open the clip?");	
							}
							int32 choice = (new BAlert("Success", successString.String(), "Yes", "No"))->Go();
							if (choice == 0) {
								entry_ref ref;
								if (entry.GetRef(&ref) == B_OK) {
									entry_ref app;
									be_roster->Launch(&ref);
								}
							}	
						}
					}
					if (((BSCApp*)be_app)->WasLaunchedSilently())
						be_app->PostMessage(B_QUIT_REQUESTED);
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


void
BSCWindow::_BuildMenu()
{
	BMenu* menu = new BMenu("File");
	BMenuItem* aboutItem = new BMenuItem("About", new BMessage(B_ABOUT_REQUESTED));
	BMenuItem* quitItem = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED));
	menu->AddItem(aboutItem);
	menu->AddItem(quitItem);
	fMenuBar->AddItem(menu);
	
	menu = new BMenu("Settings");
	BMenuItem* resetSettings = new BMenuItem("Reset Settings", new BMessage(kGUIResetSettings));
	menu->AddItem(resetSettings);
	/*BMenuItem* media = new BMenuItem("Encoding Settings"B_UTF8_ELLIPSIS, new BMessage(kGUIOpenMediaWindow));
	menu->AddItem(media);*/
	BMenuItem* dock = new BMenuItem("Dock Window", new BMessage(kGUIDockWindow));
	menu->AddItem(dock);
	fMenuBar->AddItem(menu);
}


void
BSCWindow::_LayoutWindow(bool dock)
{
	if (dock) {
		SetFlags((Flags() & ~B_AUTO_UPDATE_SIZE_LIMITS) | (B_NOT_MOVABLE|B_NOT_RESIZABLE));

		BLayoutBuilder::Group<>(this, B_VERTICAL)
			.AddGroup(B_VERTICAL)
			.SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING, 0)
				.Add(fOutputView)
				.AddGroup(B_HORIZONTAL)
					.Add(fCamStatus)
					.Add(fStartStopButton)
				.End()
			.End();
			
		fOutputView->SetLayout(new BGroupLayout(B_HORIZONTAL));
		
		BScreen screen(this);
		ResizeTo(screen.Frame().Width(), 300);
		MoveTo(0, screen.Frame().Height() - 300);
		
		return;
	}

	SetFlags((Flags() | B_AUTO_UPDATE_SIZE_LIMITS) & ~(B_NOT_MOVABLE|B_NOT_RESIZABLE));

	fOutputView->SetLayout(new BGroupLayout(B_VERTICAL));
		
	BTabView* tabView = new BTabView("Tab View", B_WIDTH_FROM_LABEL);

	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(fMenuBar)
		.AddGroup(B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, 0, B_USE_DEFAULT_SPACING, 0)
			.Add(tabView)
			.AddGroup(B_HORIZONTAL)
				.Add(fCamStatus)
				.Add(fStartStopButton)
			.End()
		.End();

	BGroupView* outputGroup = new BGroupView(B_HORIZONTAL);
	outputGroup->SetName("Capture");
	outputGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	tabView->AddTab(outputGroup);
	BLayoutBuilder::Group<>(outputGroup)
		.Add(fOutputView);

	BGroupView* advancedGroup = new BGroupView(B_HORIZONTAL);
	advancedGroup->SetName("Options");
	advancedGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	tabView->AddTab(advancedGroup);
	BLayoutBuilder::Group<>(advancedGroup)
		.Add(fAdvancedOptionsView);

	BGroupView* infoGroup = new BGroupView(B_HORIZONTAL);
	infoGroup->SetName("Info");
	infoGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING,
		B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING);
	tabView->AddTab(infoGroup);
	BLayoutBuilder::Group<>(infoGroup)
		.Add(fInfoView);
	
	CenterOnScreen();
}


status_t
BSCWindow::_CaptureStarted()
{
	fCapturing = true;
	
	Settings settings;
	if (settings.MinimizeOnRecording())
		Hide();
	
	fStartStopButton->SetLabel("Stop Recording");
	
	return B_OK;
}


status_t
BSCWindow::_CaptureFinished()
{
	fCapturing = false;
	
	fStartStopButton->SetLabel("Start Recording");
	
	if (IsHidden())
		Show();
	if (IsMinimized())
		Minimize(false);

	return B_OK;
}
