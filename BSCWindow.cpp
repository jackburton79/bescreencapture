/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "BSCWindow.h"

#include "AdvancedOptionsView.h"
#include "BSCApp.h"
#include "CamStatusView.h"
#include "ControllerObserver.h"
#include "Constants.h"
#include "InfoView.h"
#include "OptionsWindow.h"
#include "OutputView.h"
#include "SelectionWindow.h"
#include "Settings.h"

#include <Alert.h>
#include <Button.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <TabView.h>

#include <cstdio>


#define USE_INFOVIEW 0

const static BRect kWindowRect(0, 0, 400, 600);

const static uint32 kGUIOpenMediaWindow = 'j89d';
const static uint32 kGUIDockWindow = 'j90d';
const static uint32 kGUIResetSettings = 'j91d';

const static char* LABEL_BUTTON_START = "Start recording";
const static char* LABEL_BUTTON_STOP = "Stop recording";
const static char* LABEL_BUTTON_PAUSE = "Pause";
const static char* LABEL_BUTTON_RESUME = "Resume";

BSCWindow::BSCWindow()
	:
	BDirectWindow(kWindowRect, "BeScreenCapture", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS|B_NOT_RESIZABLE|B_NOT_ZOOMABLE),
	fMenuBar(NULL),
	fStartStopButton(NULL),
	fCamStatus(NULL),
	fOutputView(NULL),
	fAdvancedOptionsView(NULL),
	fInfoView(NULL)
{
	fOutputView = new OutputView();
	fAdvancedOptionsView = new AdvancedOptionsView();
#if USE_INFOVIEW
	fInfoView = new InfoView();
#endif
	fMenuBar = new BMenuBar("menubar");
	_BuildMenu();
	
	fStartStopButton = new BButton("Start", LABEL_BUTTON_START,
		new BMessage(kMsgGUIToggleCapture));
	
	fStartStopButton->SetTarget(be_app);
	fStartStopButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	// TODO: Trying to avoid button shrinking when label changes.
	// that won't work with translations, since the "Stop" label could be wider
	fStartStopButton->SetExplicitMinSize(fStartStopButton->PreferredSize());
	
	fPauseButton = new BButton("Pause", LABEL_BUTTON_PAUSE,
		new BMessage(kMsgGUITogglePause));
	fPauseButton->SetTarget(be_app);
	fPauseButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	fPauseButton->SetEnabled(false);
	
	fCamStatus = new CamStatusView();
	fCamStatus->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
	
	_LayoutWindow();

	if (be_app->LockLooper()) {
		be_app->StartWatching(this, kMsgControllerEncodeStarted);
		be_app->StartWatching(this, kMsgControllerEncodeProgress);
		be_app->StartWatching(this, kMsgControllerEncodeFinished);
		be_app->StartWatching(this, kMsgControllerTargetFrameChanged);
		be_app->StartWatching(this, kMsgControllerCaptureStarted);
		be_app->StartWatching(this, kMsgControllerCaptureStopped);
		be_app->StartWatching(this, kMsgControllerCapturePaused);
		be_app->StartWatching(this, kMsgControllerCaptureResumed);
		be_app->StartWatching(this, kMsgControllerSelectionWindowClosed);
		be_app->UnlockLooper();
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
	bool canQuit = false;
	BSCApp* app = dynamic_cast<BSCApp*>(be_app);
	if (!Settings::Current().WarnOnQuit()) {
		app->StopThreads();
		canQuit = true;
	} else if ((canQuit = app->CanQuit(reason)) != true) {
		BString text = "Do you really want to quit?\n";
		text.Append(" ");
		text.Append(reason);
		BAlert* alert = new BAlert("Really quit?", text, "Quit", "Continue");
		alert->SetShortcut(1, B_ESCAPE);
		int32 result = alert->Go();
		if (result == 0) {
			app->StopThreads();
			canQuit = true;
		}
	}

	if (canQuit)
		be_app->PostMessage(B_QUIT_REQUESTED);
	
	return canQuit && BWindow::QuitRequested();
}


void
BSCWindow::MessageReceived(BMessage *message)
{
	BSCApp* app = dynamic_cast<BSCApp*>(be_app);
	switch (message->what) {
		case B_ABOUT_REQUESTED:
			be_app->PostMessage(B_ABOUT_REQUESTED);
			break;
		case kGUIOpenMediaWindow:
			(new OptionsWindow())->Show();
			break;
		case kGUIDockWindow:
			_LayoutWindow();
			break;
		case kGUIResetSettings:
			app->ResetSettings();
			break;
		case kSelectArea:
		case kSelectWindow:
		{
			Hide();
			while (!IsHidden())
				snooze(500);
			snooze(2000);
			BMessenger messenger(app);
			int mode = message->what == kSelectArea ? SelectionWindow::REGION : SelectionWindow::WINDOW;
			SelectionWindow *window = new SelectionWindow(mode, messenger, kSelectionWindowClosed);			
			window->Show();
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
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
						BString errorString;
						errorString.SetToFormat("Could not record clip:\n"
							"%s", strerror(status));
						(new BAlert("Capture failed", errorString, "OK"))->Go();
						fStartStopButton->SetEnabled(true);
					}
					break;
				}
				case kMsgControllerCapturePaused:
				{
					fPauseButton->SetLabel(LABEL_BUTTON_RESUME);
					break;
				}
				case kMsgControllerCaptureResumed:
				{
					fPauseButton->SetLabel(LABEL_BUTTON_PAUSE);
					break;
				}
				case kMsgControllerEncodeStarted:
					fStartStopButton->SetEnabled(false);			
					break;
				case kMsgControllerEncodeFinished:
				{
					// We're set to quit, bail out
					if (Settings::Current().QuitWhenFinished())
						break;

					fStartStopButton->SetEnabled(true);
					status_t status = B_OK;
					if (message->FindInt32("status", (int32*)&status) == B_OK
						&& status != B_OK) {
						BString errorString;
						errorString.SetToFormat("Could not create clip: "
							"%s", strerror(status));
						(new BAlert("Encoding failed", errorString, "OK"))->Go();
					} else {
						// TODO: Should be asynchronous
						const char* destName = NULL;
						message->FindString("file_name", &destName);
						BEntry entry(destName);
						if (entry.Exists()) {
							BString buttonName;
							BString successString;
							if (entry.IsDirectory()) {
								buttonName.SetTo("Open folder");
								successString.SetTo("Finished recording");
							} else {
								buttonName.SetTo("Play");
								successString.Append("Finished recording ");
								successString.Append(entry.Name());
							}
							BAlert* alert = new BAlert("Success", successString,
								"OK", buttonName.String());
							alert->SetShortcut(0, B_ESCAPE);
							int32 choice = alert->Go();
							if (choice == 1) {
								entry_ref ref;
								if (entry.GetRef(&ref) == B_OK) {
									entry_ref app;
									be_roster->Launch(&ref);
								}
							}	
						}
					}
					if (dynamic_cast<BSCApp*>(be_app)->WasLaunchedSilently())
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
	BDirectWindow::DirectConnected(info);

	BSCApp* app = dynamic_cast<BSCApp*>(be_app);
	if (app == NULL)
		return;

	switch (info->buffer_state & B_DIRECT_MODE_MASK) {
		case B_DIRECT_START:
		case B_DIRECT_MODIFY:
			app->UpdateDirectInfo(info);
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
	BMenu* menu = new BMenu("App");
	BMenuItem* aboutItem = new BMenuItem("About", new BMessage(B_ABOUT_REQUESTED));
	BMenuItem* quitItem = new BMenuItem("Quit", new BMessage(B_QUIT_REQUESTED));
	menu->AddItem(aboutItem);
	menu->AddItem(quitItem);
	fMenuBar->AddItem(menu);
	
	menu = new BMenu("Settings");
	BMenuItem* resetSettings = new BMenuItem("Default", new BMessage(kGUIResetSettings));
	menu->AddItem(resetSettings);

	fMenuBar->AddItem(menu);
}


void
BSCWindow::_LayoutWindow()
{
	SetFlags((Flags() | B_AUTO_UPDATE_SIZE_LIMITS) & ~(B_NOT_MOVABLE));

	BTabView* tabView = new BTabView("Tab View", B_WIDTH_FROM_LABEL);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fMenuBar)
		.Add(tabView)
		.AddGroup(B_HORIZONTAL)
			.Add(fCamStatus)
			.Add(fPauseButton)
			.Add(fStartStopButton)
			.SetInsets(B_USE_DEFAULT_SPACING)
		.End();

	BGroupView* outputGroup = new BGroupView(B_HORIZONTAL);
	outputGroup->SetName("Capture");
	outputGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING);
	tabView->AddTab(outputGroup);
	BLayoutBuilder::Group<>(outputGroup)
		.Add(fOutputView);

	BGroupView* advancedGroup = new BGroupView(B_HORIZONTAL);
	advancedGroup->SetName("Options");
	advancedGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING);
	tabView->AddTab(advancedGroup);
	BLayoutBuilder::Group<>(advancedGroup)
		.Add(fAdvancedOptionsView);

	if (fInfoView != NULL) {
		BGroupView* infoGroup = new BGroupView(B_HORIZONTAL);
		infoGroup->SetName("Info");
		infoGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING);
		tabView->AddTab(infoGroup);
		BLayoutBuilder::Group<>(infoGroup)
			.Add(fInfoView);
	}
	CenterOnScreen();
}


status_t
BSCWindow::_CaptureStarted()
{
	const Settings& settings = Settings::Current();
	if (settings.MinimizeOnRecording())
		Hide();
	
	fStartStopButton->SetLabel(LABEL_BUTTON_STOP);
	fPauseButton->SetLabel(LABEL_BUTTON_PAUSE);
	fPauseButton->SetEnabled(true);
	
	return B_OK;
}


status_t
BSCWindow::_CaptureFinished()
{
	fStartStopButton->SetLabel(LABEL_BUTTON_START);
	fPauseButton->SetLabel(LABEL_BUTTON_PAUSE);
	fPauseButton->SetEnabled(false);
	
	// TODO: maybe don't show window if launched with
	// the Shift-Alt-Control-R combo
	if (IsHidden())
		Show();
	if (IsMinimized())
		Minimize(false);

	return B_OK;
}
