/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "BSCApp.h"

#include "Arguments.h"
#include "BSCWindow.h"
#include "Constants.h"
#include "Controller.h"
#include "DeskbarControlView.h"
#include "PublicMessages.h"
#include "Settings.h"

#include <private/interface/AboutWindow.h>
#include <Deskbar.h>
#include <NodeInfo.h>
#include <PropertyInfo.h>
#include <Roster.h>
#include <StringList.h>

#include <cstdio>
#include <string>

const char kChangeLog[] = {
#include "Changelog.h"
};

const char* kAuthors[] = {
	"Stefano Ceccherini (stefano.ceccherini@gmail.com)",
	NULL
};


#define BSC_SUITES "suites/vnd.BSC-application"
#define kPropertyStartRecording "Record"
#define kPropertyStopRecording "Stop"
#define kPropertyCaptureRect "CaptureRect"
#define kPropertyScaleFactor "Scale"
#define kPropertyRecordingTime "RecordingTime"
#define kPropertyQuitWhenFinished "QuitWhenFinished"

const property_info kPropList[] = {
	{
		kPropertyCaptureRect,
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_NO_SPECIFIER },
		"set/get capture rect # Set/Get capture rect",
		0,
		{ B_RECT_TYPE },
		{},
		{}
	},
	{
		kPropertyScaleFactor,
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_NO_SPECIFIER },
		"set/get scaling factor # Set/Get scaling factor",
		0,
		{ B_FLOAT_TYPE },
		{},
		{}
	},
	{
		kPropertyStartRecording,
		{ B_EXECUTE_PROPERTY },
		{ B_NO_SPECIFIER },
		"start recording # Start recording",
		0,
		{},
		{},
		{}
	},
	{
		kPropertyStopRecording,
		{ B_EXECUTE_PROPERTY },
		{ B_NO_SPECIFIER },
		"stop recording # Stop recording",
		0,
		{},
		{},
		{}
	},
	{
		kPropertyRecordingTime,
		{ B_SET_PROPERTY },
		{ B_NO_SPECIFIER },
		"set recording time # Set recording time",
		0,
		{ B_UINT32_TYPE },
		{},
		{}
	},
		{
		kPropertyQuitWhenFinished,
		{ B_SET_PROPERTY },
		{ B_NO_SPECIFIER },
		"set quit when finished # Set quit when finished",
		0,
		{ B_BOOL_TYPE },
		{},
		{}
	},
	{ 0 }
};

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
	fWindow(NULL),
	fArgs(NULL)
{
	fArgs = new Arguments(0, NULL);
	
	Settings::Initialize();

	fShouldStartRecording = false;
	gControllerLooper = new Controller();
}


BSCApp::~BSCApp()
{
	RemoveDeskbarReplicant();

	Settings::Current().Save();
	Settings::Destroy();

	gControllerLooper->Lock();
	gControllerLooper->Quit();
	
	delete fArgs;
}


void
BSCApp::ArgvReceived(int32 argc, char** argv)
{
	LaunchedFromCommandline();
	fArgs->Parse(argc, argv);
	if (fArgs->UsageRequested()) {
		_UsageRequested();
		PostMessage(B_QUIT_REQUESTED);
		return;
	}
	
	fShouldStartRecording = fArgs->RecordNow();
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
		BMessenger(gControllerLooper).SendMessage(kMsgGUIToggleCapture);
	} else {
		fWindow->Show();
		InstallDeskbarReplicant();
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
	if (_HandleScripting(message))
		return;
		
	switch (message->what) {
		case kMsgGetControllerMessenger:
		{
			// the deskbar view needs the controller looper messenger
			BMessage reply(kMsgGetControllerMessenger);
			BMessenger controllerMessenger(gControllerLooper);
			reply.AddMessenger("ControllerMessenger", controllerMessenger);
			message->SendReply(&reply);
			break;
		}
		case kMsgGUIToggleCapture:
		case kMsgGUITogglePause:
			if (gControllerLooper != NULL)
				BMessenger(gControllerLooper).SendMessage(message);
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


BStringList
SplitChangeLog(const char* changeLog)
{
	BStringList list;
	char* stringStart = (char*)changeLog;
	int i = 0;
	char c;
	while ((c = stringStart[i]) != '\0') {
		if (c == '-'  && i > 2 && stringStart[i - 1] == '-' && stringStart[i - 2] == '-') {
			BString string;
			string.Append(stringStart, i - 2);
			string.RemoveAll("\t");
			//string.ReplaceFirst("-", "\n\t-");
			string.ReplaceAll("-", "\n\t-");
			list.Add(string);
			stringStart = stringStart + i + 1;
			i = 0;
		} else
			i++;
	}
	return list;		
}


/* virtual */
status_t
BSCApp::GetSupportedSuites(BMessage* message)
{
	status_t status = message->AddString("suites", BSC_SUITES);
	if (status != B_OK)
		return status;
		
	BPropertyInfo info(const_cast<property_info*>(kPropList));
	status = message->AddFlat("messages", &info);
	if (status == B_OK)
		status = BApplication::GetSupportedSuites(message);
	return status;
}


/* virtual */
BHandler*
BSCApp::ResolveSpecifier(BMessage* message,
								int32 index,
								BMessage* specifier,
								int32 what,
								const char* property)
{
	BPropertyInfo info(const_cast<property_info*>(kPropList));
	int32 result = info.FindMatch(message, index, specifier, what, property);
	if (result < 0) {
		return BApplication::ResolveSpecifier(message, index,
				specifier, what, property);
	}
	return this;
}


void
BSCApp::AboutRequested()
{
	BAboutWindow* aboutWindow = new BAboutWindow("BeScreenCapture", kAppSignature);
	aboutWindow->AddDescription("BeScreenCapture is a screen recording application for Haiku");
	aboutWindow->AddAuthors(kAuthors);
	aboutWindow->AddCopyright(2013, "Stefano Ceccherini");
	BStringList list = SplitChangeLog(kChangeLog);
	int32 stringCount = list.CountStrings();
	char** charArray = new char* [stringCount + 1];
	for (int32 i = 0; i < stringCount; i++) {
		charArray[i] = (char*)list.StringAt(i).String();
	}
	charArray[stringCount] = NULL;
	
	aboutWindow->AddVersionHistory((const char**)charArray);
	delete[] charArray;
	
	aboutWindow->Show();
}


bool
BSCApp::_HandleScripting(BMessage* message)
{
	uint32 what = message->what;
	if (what != B_EXECUTE_PROPERTY &&
		what != B_GET_PROPERTY &&
		what != B_SET_PROPERTY)
	return false;
	
	BMessage reply(B_REPLY);
	const char* property = NULL;
	int32 index = 0;
	int32 form = 0;
	BMessage specifier;
	status_t status = message->GetCurrentSpecifier(&index,
		&specifier, &form, &property);
	if (status != B_OK || index == -1)
		return false;

	Controller* controller = dynamic_cast<Controller*>(gControllerLooper);
	if (controller == NULL)
		return false;

	status_t result = B_OK;
	switch (what) {
		case B_GET_PROPERTY:
		case B_SET_PROPERTY:
		{
			if (strcmp(property, kPropertyCaptureRect) == 0) {
				if (form == B_DIRECT_SPECIFIER) {
					if (what == B_GET_PROPERTY) {
						Settings& settings = Settings::Current();
						reply.AddRect("result", settings.CaptureArea());
					} else if (what == B_SET_PROPERTY) {
						BRect rect;
						if (message->FindRect("data", &rect) != B_OK) {
							result = B_ERROR;
							break;
						}
						controller->SetCaptureArea(rect);
					}
					reply.AddInt32("error", result);
					message->SendReply(&reply);
				}
			} else if (strcmp(property, kPropertyScaleFactor) == 0) {
				if (form == B_DIRECT_SPECIFIER) {
					if (what == B_GET_PROPERTY) {
						Settings& settings = Settings::Current();
						reply.AddFloat("result", settings.Scale());
					} else if (what == B_SET_PROPERTY) {
						float scale;
						if (message->FindFloat("data", &scale) != B_OK) {
							result = B_ERROR;
							break;
						}
						controller->SetScale(scale);
					}
					reply.AddInt32("error", result);
					message->SendReply(&reply);
				}
			} else if (strcmp(property, kPropertyRecordingTime) == 0) {
				if (form == B_DIRECT_SPECIFIER) {
					if (what == B_SET_PROPERTY) {
						// TODO: bigtime_t is uint64, but apparently hey
						// doesn't support uint64s
						int32 msecs = 0;
						if (message->FindInt32("data", &msecs) != B_OK) {
							result = B_ERROR;
							break;
						}
						controller->SetRecordingTime(bigtime_t(msecs));
					}
					reply.AddInt32("error", result);
					message->SendReply(&reply);
				}
			}
			break;
		}
		case B_EXECUTE_PROPERTY:
		{
			if (strcmp(property, kPropertyStartRecording) == 0) {
				if (form == B_DIRECT_SPECIFIER) {
					if (controller->State() == Controller::STATE_IDLE) {
						BMessage toggleMessage(kMsgGUIToggleCapture);
						if (gControllerLooper != NULL)
							BMessenger(gControllerLooper).SendMessage(&toggleMessage);
					} else
						result = B_ERROR;
					reply.AddInt32("error", result);
					message->SendReply(&reply);
				}
			} else if (strcmp(property, kPropertyStopRecording) == 0) {
				if (form == B_DIRECT_SPECIFIER) {
					if (controller->State() == Controller::STATE_RECORDING) {
						BMessage toggleMessage(kMsgGUIToggleCapture);
						if (gControllerLooper != NULL)
							BMessenger(gControllerLooper).SendMessage(&toggleMessage);
					} else
						result = B_ERROR;
					reply.AddInt32("error", result);
					message->SendReply(&reply);
				}
			}
			break;
		}
		default:
			break;
	}
	return true;
}


void
BSCApp::InstallDeskbarReplicant()
{
	BDeskbar deskbar;
	if (deskbar.IsRunning()) {
		if (!deskbar.HasItem(BSC_DESKBAR_VIEW)) {
			app_info info;
			be_roster->GetAppInfo(kAppSignature, &info);
			deskbar.AddItem(&info.ref);
		}
	}
}


void
BSCApp::RemoveDeskbarReplicant()
{
	BDeskbar deskbar;
	if (deskbar.IsRunning()) {
		while (deskbar.HasItem(BSC_DESKBAR_VIEW))
			deskbar.RemoveItem(BSC_DESKBAR_VIEW);
	}
}


bool
BSCApp::WasLaunchedSilently() const
{
	return fShouldStartRecording;
}


bool
BSCApp::LaunchedFromCommandline() const
{
	team_info teamInfo;
	get_team_info(getppid(), &teamInfo);

	// TODO: Not correct, since someone could use another shell
	std::string args = teamInfo.args;
	return args.find("/bin/bash") != std::string::npos;
}


void
BSCApp::_UsageRequested()
{
}
