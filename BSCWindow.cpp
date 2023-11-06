/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
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
#include <Catalog.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <MenuBar.h>
#include <Roster.h>
#include <Screen.h>
#include <String.h>
#include <TabView.h>

#include <cstdio>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "BSCWindow"


#define USE_INFOVIEW 0

const static BRect kWindowRect(0, 0, 400, 600);

const static uint32 kGUIDockWindow = 'j90d';
const static uint32 kGUIResetSettings = 'j91d';
const static uint32 kGUIShowHelp = 'j92d';

const char* LABEL_START = B_TRANSLATE("Start");
const char* LABEL_STOP = B_TRANSLATE("Stop");
const char* LABEL_PAUSE = B_TRANSLATE("Pause");
const char* LABEL_RESUME = B_TRANSLATE("Resume");

BSCWindow::BSCWindow()
	:
	BDirectWindow(kWindowRect, B_TRANSLATE_SYSTEM_NAME("BeScreenCapture"), B_TITLED_WINDOW,
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

	fStartStopButton = new BButton("start", LABEL_START,
		new BMessage(kMsgGUIToggleCapture));

	fStartStopButton->SetTarget(be_app);
	fStartStopButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	// TODO: Trying to avoid button shrinking when label changes.
	// that won't work with translations, since the "Stop" label could be wider
	fStartStopButton->SetExplicitMinSize(fStartStopButton->PreferredSize());

	fPauseButton = new BButton("pause", LABEL_PAUSE,
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
		BString text = B_TRANSLATE("Do you really want to quit BeScreenCapture?\n");
		text.Append(" ");
		text.Append(reason);
		BAlert* alert = new BAlert(B_TRANSLATE("Really quit?"), text,
			B_TRANSLATE("Quit"), B_TRANSLATE("Continue"));
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
		case kGUIDockWindow:
			_LayoutWindow();
			break;
		case kGUIResetSettings:
			app->ResetSettings();
			break;
		case kGUIShowHelp:
			app->ShowHelp();
			break;
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
						errorString.SetToFormat(B_TRANSLATE("Could not record clip:\n%s"),
							strerror(status));
						(new BAlert(B_TRANSLATE("Capture failed"), errorString,
							B_TRANSLATE("OK")))->Go();
						fStartStopButton->SetEnabled(true);
					}
					break;
				}
				case kMsgControllerCapturePaused:
				{
					fPauseButton->SetLabel(LABEL_RESUME);
					break;
				}
				case kMsgControllerCaptureResumed:
				{
					fPauseButton->SetLabel(LABEL_PAUSE);
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
						errorString.SetToFormat(B_TRANSLATE("Could not create clip:\n%s"),
							strerror(status));
						(new BAlert(B_TRANSLATE("Encoding failed"), errorString,
							B_TRANSLATE("OK")))->Go();
					} else {
						// TODO: Should be asynchronous
						const char* destName = NULL;
						message->FindString("file_name", &destName);
						BEntry entry(destName);
						if (entry.Exists()) {
							BString buttonName;
							BString successString;
							if (entry.IsDirectory()) {
								buttonName.SetTo(B_TRANSLATE("Open folder"));
								successString.SetTo(B_TRANSLATE("Finished recording"));
							} else {
								buttonName.SetTo(B_TRANSLATE("Play"));
								successString.Append(B_TRANSLATE("Finished recording '%filename%'."));
								successString.ReplaceFirst("%filename%", entry.Name());
							}
							BAlert* alert = new BAlert(B_TRANSLATE("Success"), successString,
								B_TRANSLATE("OK"), buttonName.String());
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
	BDirectWindow::ScreenChanged(screen_size, depth);
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


/* virtual */
void
BSCWindow::MenusBeginning()
{
	BWindow::MenusBeginning();

	BMenuItem* recordingItem = fMenuBar->FindItem(B_TRANSLATE("Recording"));
	if (recordingItem == NULL)
		return;
	BMenu* menu = recordingItem->Menu();
	if (menu == NULL)
		return;

	BSCApp* app = dynamic_cast<BSCApp*>(be_app);
	if (app == NULL)
		return;
	
	BMenuItem* start = menu->FindItem(LABEL_START);
	BMenuItem* stop = menu->FindItem(LABEL_STOP);
	BMenuItem* pause = menu->FindItem(LABEL_PAUSE);
	int state = app->State();
	if (start != NULL)
		start->SetEnabled(state == BSCApp::STATE_IDLE);
	if (stop != NULL)
		stop->SetEnabled(state == BSCApp::STATE_RECORDING);
	if (pause != NULL) {
		pause->SetEnabled(state == BSCApp::STATE_RECORDING);
		pause->SetMarked(app->Paused());
	}
}


void
BSCWindow::_BuildMenu()
{
	BMenu* menu = new BMenu(B_TRANSLATE("App"));
	BMenuItem* helpItem = new BMenuItem(B_TRANSLATE("Help" B_UTF8_ELLIPSIS),
		new BMessage(kGUIShowHelp), 'H');
	BMenuItem* aboutItem = new BMenuItem(B_TRANSLATE("About BeScreenCapture"),
		new BMessage(B_ABOUT_REQUESTED));
	BMenuItem* quitItem = new BMenuItem(B_TRANSLATE("Quit"), new BMessage(B_QUIT_REQUESTED), 'Q');
	menu->AddItem(helpItem);
	menu->AddItem(aboutItem);
	menu->AddSeparatorItem();
	menu->AddItem(quitItem);
	fMenuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Recording"));
	BMenuItem* startRecording = new BMenuItem(LABEL_START,
		new BMessage(kMsgGUIToggleCapture), 'S');
	startRecording->SetTarget(be_app);
	BMenuItem* stopRecording = new BMenuItem(LABEL_STOP,
		new BMessage(kMsgGUIToggleCapture), 'T');
	stopRecording->SetTarget(be_app);
	BMenuItem* pauseRecording = new BMenuItem(LABEL_PAUSE,
		new BMessage(kMsgGUITogglePause), 'P');
	pauseRecording->SetTarget(be_app);
	menu->AddItem(startRecording);
	menu->AddItem(stopRecording);
	menu->AddSeparatorItem();
	menu->AddItem(pauseRecording);
	fMenuBar->AddItem(menu);

	menu = new BMenu(B_TRANSLATE("Settings"));
	BMenuItem* resetSettings = new BMenuItem(B_TRANSLATE("Reset to defaults"),
		new BMessage(kGUIResetSettings));
	menu->AddItem(resetSettings);
	fMenuBar->AddItem(menu);
}


void
BSCWindow::_LayoutWindow()
{
	SetFlags((Flags() | B_AUTO_UPDATE_SIZE_LIMITS) & ~(B_NOT_MOVABLE));

	BTabView* tabView = new BTabView("tabview", B_WIDTH_FROM_LABEL);

	BLayoutBuilder::Group<>(this, B_VERTICAL, 0)
		.Add(fMenuBar)
		.AddGroup(B_VERTICAL)
			.SetInsets(-2, B_USE_SMALL_SPACING, -2, 0)
			.Add(tabView)
		.End()
		.AddGroup(B_HORIZONTAL)
			.Add(fCamStatus)
			.Add(fPauseButton)
			.Add(fStartStopButton)
			.SetInsets(B_USE_DEFAULT_SPACING, 0)
		.End();

	BGroupView* outputGroup = new BGroupView(B_HORIZONTAL);
	outputGroup->SetName(B_TRANSLATE("Capture"));
	outputGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING);
	tabView->AddTab(outputGroup);
	BLayoutBuilder::Group<>(outputGroup)
		.Add(fOutputView);

	BGroupView* advancedGroup = new BGroupView(B_HORIZONTAL);
	advancedGroup->SetName(B_TRANSLATE("Options"));
	advancedGroup->GroupLayout()->SetInsets(B_USE_DEFAULT_SPACING);
	tabView->AddTab(advancedGroup);
	BLayoutBuilder::Group<>(advancedGroup)
		.Add(fAdvancedOptionsView);

	if (fInfoView != NULL) {
		BGroupView* infoGroup = new BGroupView(B_HORIZONTAL);
		infoGroup->SetName(B_TRANSLATE("Info"));
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

	fStartStopButton->SetLabel(LABEL_STOP);
	fPauseButton->SetLabel(LABEL_PAUSE);
	fPauseButton->SetEnabled(true);

	return B_OK;
}


status_t
BSCWindow::_CaptureFinished()
{
	fStartStopButton->SetLabel(LABEL_START);
	fPauseButton->SetLabel(LABEL_PAUSE);
	fPauseButton->SetEnabled(false);

	// TODO: maybe don't show window if launched with
	// the Shift-Alt-Control-R combo
	if (IsHidden())
		Show();
	if (IsMinimized())
		Minimize(false);

	return B_OK;
}
