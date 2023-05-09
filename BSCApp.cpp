/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "BSCApp.h"

#include "Arguments.h"
#include "BSCWindow.h"
#include "Constants.h"
#include "ControllerObserver.h"
#include "DeskbarControlView.h"
#include "FramesList.h"
#include "FunctionObject.h"
#include "MovieEncoder.h"
#include "PublicMessages.h"
#include "Settings.h"
#include "Utils.h"

#include <private/interface/AboutWindow.h>
#include <Autolock.h>
#include <Bitmap.h>
#include <Deskbar.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <MessageRunner.h>
#include <NodeInfo.h>
#include <PropertyInfo.h>
#include <Roster.h>
#include <Screen.h>
#include <StopWatch.h>
#include <String.h>
#include <StringList.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <string>

#include <iostream>

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
		"Set/Get capture rect",
		0,
		{ B_RECT_TYPE },
		{},
		{}
	},
	{
		kPropertyScaleFactor,
		{ B_GET_PROPERTY, B_SET_PROPERTY },
		{ B_NO_SPECIFIER },
		"Set/Get scaling factor",
		0,
		{ B_INT32_TYPE },
		{},
		{}
	},
	{
		kPropertyStartRecording,
		{ B_EXECUTE_PROPERTY },
		{ B_NO_SPECIFIER },
		"Start recording",
		0,
		{},
		{},
		{}
	},
	{
		kPropertyStopRecording,
		{ B_EXECUTE_PROPERTY },
		{ B_NO_SPECIFIER },
		"Stop recording",
		0,
		{},
		{},
		{}
	},
	{
		kPropertyRecordingTime,
		{ B_SET_PROPERTY },
		{ B_NO_SPECIFIER },
		"Set recording time (in seconds)",
		0,
		{ B_INT32_TYPE },
		{},
		{}
	},
		{
		kPropertyQuitWhenFinished,
		{ B_SET_PROPERTY },
		{ B_NO_SPECIFIER },
		"Set quit when finished",
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
	BSCApp* app = new BSCApp();
	app->Run();
	delete app;

	return 0;
}


BSCApp::BSCApp()
	:
	BApplication(kAppSignature),
	fWindow(NULL),
	fArgs(NULL),
	fShouldStartRecording(false),
	fCaptureThread(-1),
	fNumFrames(0),
	fRecordWatch(NULL),
	fKillCaptureThread(true),
	fPaused(false),
	fDirectWindowAvailable(false),
	fEncoder(NULL),
	fEncoderThread(-1),
	fFileList(NULL),
	fCodecList(NULL),
	fStopRunner(NULL),
	fRequestedRecordTime(0),
	fSupportsWaitForRetrace(false)
{
	fArgs = new Arguments(0, NULL);
	
	Settings::Initialize();

	memset(&fDirectInfo, 0, sizeof(fDirectInfo));

	fEncoder = new MovieEncoder;

	_UpdateFromSettings();
}


BSCApp::~BSCApp()
{
	RemoveDeskbarReplicant();
	
	StopThreads();
	delete fRecordWatch;
	delete fEncoder;
	delete fCodecList;
	delete fFileList;

	Settings::Current().Save();
	Settings::Destroy();
	
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
		BMessenger(be_app).SendMessage(kMsgGUIToggleCapture);
	} else {
		// TODO: InstallDeskbarReplicant creates a deadlock
		// if called AFTER the window is created
		// I guess it's the interaction between the direct_info thread calling
		// BDirectWindow::DirectConnected() (and our inherited version which ends up
		// locking the BApplication) and BRoster::GetAppInfo() which also locks the BApplication
		// Looks like doing it before Show() is safe.
		InstallDeskbarReplicant();
		fWindow->Show();
	}
}


bool
BSCApp::QuitRequested()
{
	if (State() == STATE_IDLE)
		return BApplication::QuitRequested();

	return false;
}


void
BSCApp::MessageReceived(BMessage *message)
{
	if (_HandleScripting(message))
		return;
		
	switch (message->what) {
		case kSelectionWindowClosed:
		{
			SendNotices(kMsgControllerSelectionWindowClosed, message);
			BRect rect;
			if (message->FindRect("selection", &rect) == B_OK) {
				SetCaptureArea(rect);
			}
			break;
		}

		case kMsgGUIToggleCapture:
			if (fWindow != NULL)
				ToggleCapture();
			else
				fShouldStartRecording = true;
			break;

		case kMsgGUITogglePause:
			TogglePause();
			break;

		case kEncodingFinished:
		{
			status_t error;
			message->FindInt32("status", (int32*)&error);
			const char* fileName = NULL;
			message->FindString("file_name", &fileName);
			_EncodingFinished(error, fileName);
			break;
		}
		case kEncodingProgress:
		{
			BMessage progressMessage(kMsgControllerEncodeProgress);
			int32 numFrames = 0;
			if (message->FindInt32("frames_remaining", &numFrames) == B_OK)
				progressMessage.AddInt32("frames_remaining", numFrames);
			int32 framesTotal = 0;
			if (message->FindInt32("frames_total", &framesTotal) == B_OK)
				progressMessage.AddInt32("frames_total", framesTotal);
			const char* text = NULL;
			if (message->FindString("text", &text) == B_OK)
				progressMessage.AddString("text", text);
			bool reset = false;
			if (message->FindBool("reset", &reset) == B_OK)
				progressMessage.AddBool("reset", reset);

			SendNotices(kMsgControllerEncodeProgress, &progressMessage);
			break;
		}
		
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
			string.ReplaceAll("- ", "\n\t- ");
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
						if (message->FindRect("data", &rect) == B_OK) {
							if (!SetCaptureArea(rect))
								result = B_BAD_VALUE;
						} else
							result = B_ERROR;
					}
					reply.AddInt32("error", result);
					message->SendReply(&reply);
				}
			} else if (strcmp(property, kPropertyScaleFactor) == 0) {
				if (form == B_DIRECT_SPECIFIER) {
					if (what == B_GET_PROPERTY) {
						Settings& settings = Settings::Current();
						reply.AddInt32("result", int32(settings.Scale()));
					} else if (what == B_SET_PROPERTY) {
						int32 scale;
						if (message->FindInt32("data", &scale) == B_OK)
							SetScale(float(scale));
						else
							result = B_ERROR;
					}
					reply.AddInt32("error", result);
					message->SendReply(&reply);
				}
			} else if (strcmp(property, kPropertyRecordingTime) == 0) {
				if (form == B_DIRECT_SPECIFIER) {
					if (what == B_SET_PROPERTY) {
						int32 seconds = 0;
						if (message->FindInt32("data", &seconds) == B_OK)
							SetRecordingTime(bigtime_t(seconds * 1000 * 1000));
						else
							result = B_ERROR;
					}
					reply.AddInt32("error", result);
					message->SendReply(&reply);
				}
			} else if (strcmp(property, kPropertyQuitWhenFinished) == 0) {
				if (form == B_DIRECT_SPECIFIER) {
					if (what == B_SET_PROPERTY) {
						// No need to check for error, just assume "false" in that case
						bool quit = false;
						message->FindBool("data", &quit);
						Settings::Current().SetQuitWhenFinished(quit);
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
					if (State() == BSCApp::STATE_IDLE) {
						BMessage toggleMessage(kMsgGUIToggleCapture);
						be_app->PostMessage(&toggleMessage);
					} else
						result = B_ERROR;
					reply.AddInt32("error", result);
					message->SendReply(&reply);
				}
			} else if (strcmp(property, kPropertyStopRecording) == 0) {
				if (form == B_DIRECT_SPECIFIER) {
					if (State() == BSCApp::STATE_RECORDING) {
						BMessage toggleMessage(kMsgGUIToggleCapture);
						be_app->PostMessage(&toggleMessage);
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
	if (deskbar.IsRunning() && !deskbar.HasItem(BSC_DESKBAR_VIEW)) {
		app_info info;
		if (be_roster->GetAppInfo(kAppSignature, &info) == B_OK)
			deskbar.AddItem(&info.ref);
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


bool
BSCApp::CanQuit(BString& reason) const
{
	BAutolock _(const_cast<BSCApp*>(this));
	switch (State()) {
		case STATE_RECORDING:
			reason = "Recording in progress.";
			break;
		case STATE_ENCODING:
			reason = "Encoding in progress.";
			break;
		case STATE_IDLE:
			return true;
		default:
			break;
	}

	return false;
}


void
BSCApp::StopThreads()
{
	BAutolock _(this);
	switch (State()) {
		case STATE_RECORDING:
		{
			status_t status;
			fKillCaptureThread = true;
			wait_for_thread(fCaptureThread, &status);
			fCaptureThread = -1;
			break;
		}
		case STATE_ENCODING:
			fEncoder->Cancel();
			fEncoderThread = -1;
			break;
		case STATE_IDLE:
		default:
			break;
	}
}


int
BSCApp::State() const
{
	BAutolock _(const_cast<BSCApp*>(this));
	if (fCaptureThread > 0)
		return STATE_RECORDING;

	if (fEncoderThread > 0 || fFileList != NULL)
		return STATE_ENCODING;

	return STATE_IDLE;
}


void
BSCApp::ToggleCapture()
{
	BAutolock _(this);
	int state = State();
	switch (state) {
		case STATE_IDLE:
			StartCapture();
			break;
		case STATE_RECORDING:
			EndCapture();
			break;
		case STATE_ENCODING:
			// DO NOTHING
			break;
	}
}


void
BSCApp::TogglePause()
{
	BAutolock _(this);
	if (fCaptureThread < 0)
		return;

	if (fPaused)
		_ResumeCapture();
	else
		_PauseCapture();
}


int32
BSCApp::RecordedFrames() const
{
	return atomic_get((int32*)&fNumFrames);
}


bigtime_t
BSCApp::RecordTime() const
{
	return fRecordWatch != NULL ? fRecordWatch->ElapsedTime() : 0;
}


void
BSCApp::SetRecordingTime(const bigtime_t msecs)
{
	BAutolock _(this);
	// Bail out if recording is already started
	if (fCaptureThread > 0)
		return;

	fRequestedRecordTime = msecs;
}


float
BSCApp::AverageFPS() const
{
	if (RecordTime() == 0)
		return 0;
	return CalculateFPS(RecordedFrames(), RecordTime());
}


void
BSCApp::EncodeMovie()
{
	BAutolock _(this);

	int32 numFrames = fFileList->CountItems();
	if (numFrames <= 0) {
		_EncodingFinished(B_ERROR, NULL);
		delete fFileList;
		fFileList = NULL;
		return;
	}

	// Write to a temp file
	BPath path;
	status_t status = find_directory(B_SYSTEM_TEMP_DIRECTORY, &path);
	if (status != B_OK)
		throw status;
	char tempFileName[B_PATH_NAME_LENGTH];
	snprintf(tempFileName, sizeof(tempFileName), "%s/BSC_clip_XXXXXXX", path.Path());
	// mkstemp creates a fd with an unique file name.
	// We then close the fd immediately, because we only need an unique file name.
	// In theory, it's possible that between this and WriteFrame() someone
	// creates a file with this exact name, but it's not likely to happen.
	int tempFile = ::mkstemp(tempFileName);
	if (tempFile < 0)
		throw errno;

	BString fileName = tempFileName;
	::close(tempFile);
	// Remove the temporary file, will be created
	// later with the same name by MovieEncoder
	// TODO: Not nice
	BEntry(fileName).Remove();

	// Tell the encoder where to write
	fEncoder->SetOutputFile(fileName);

	BMessage message(kMsgControllerEncodeStarted);
	message.AddInt32("frames_total", numFrames);
	SendNotices(kMsgControllerEncodeStarted, &message);

	fEncoder->SetSource(fFileList);
	fFileList = NULL;

	BMessenger messenger(this);
	fEncoder->SetMessenger(messenger);

	fEncoderThread = fEncoder->EncodeThreaded();
}


void
BSCApp::SetUseDirectWindow(const bool& use)
{
	BAutolock _(this);
	Settings::Current().SetUseDirectWindow(use);
}


bool
BSCApp::SetCaptureArea(const BRect& rect)
{
	BAutolock _(this);

	// Rect is bigger than screen:
	// bail out
	BRect screenRect = BScreen().Frame();
	if (rect.Width() > screenRect.Width() ||
		rect.Height() > screenRect.Height() ||
		rect.left < 0 || rect.top < 0)
		return false;

	Settings& settings = Settings::Current();
	settings.SetCaptureArea(rect);
	BRect targetRect = settings.TargetRect();
	fEncoder->SetDestFrame(targetRect);

	BMessage message(kMsgControllerSourceFrameChanged);
	message.AddRect("frame", rect);
	SendNotices(kMsgControllerSourceFrameChanged, &message);

	_HandleTargetFrameChanged(targetRect);

	return true;
}


void
BSCApp::SetCaptureFrameRate(const int fps)
{
	BAutolock _(this);
	Settings::Current().SetCaptureFrameRate(fps);

	BMessage message(kMsgControllerCaptureFrameRateChanged);
	message.AddInt32("frame_rate", fps);
	SendNotices(kMsgControllerCaptureFrameRateChanged, &message);
}


void
BSCApp::SetPlaybackFrameRate(const int rate)
{
	BAutolock _(this);
}


void
BSCApp::SetScale(const float &scale)
{
	BAutolock _(this);
	Settings& settings = Settings::Current();
	settings.SetScale(scale);
	BRect rect(settings.TargetRect());
	fEncoder->SetDestFrame(rect);
	BMessage message(kMsgControllerTargetFrameChanged);
	message.AddRect("frame", rect);
	message.AddFloat("scale", scale);
	SendNotices(kMsgControllerTargetFrameChanged, &message);
}


void
BSCApp::SetVideoDepth(const color_space &space)
{
	BAutolock _(this);
	fEncoder->SetColorSpace(space);
	Settings::Current().SetClipDepth(space);
	SendNotices(kMsgControllerVideoDepthChanged);
}


void
BSCApp::SetOutputFileName(const char *fileName)
{
	BAutolock _(this);
	Settings::Current().SetOutputFileName(fileName);
	fEncoder->SetOutputFile(fileName);
}


media_format_family
BSCApp::MediaFormatFamily() const
{
	BAutolock _(const_cast<BSCApp*>(this));
	return fEncoder->MediaFormatFamily();
}


void
BSCApp::SetMediaFormatFamily(const media_format_family& family)
{
	BAutolock _(this);
	fEncoder->SetMediaFormatFamily(family);
	UpdateMediaFormatAndCodecsForCurrentFamily();
}


media_file_format
BSCApp::MediaFileFormat() const
{
	BAutolock _(const_cast<BSCApp*>(this));
	return fEncoder->MediaFileFormat();
}


BString
BSCApp::MediaFileFormatName() const
{
	BAutolock _(const_cast<BSCApp*>(this));
	return fEncoder->MediaFileFormat().pretty_name;
}


void
BSCApp::SetMediaFileFormat(const media_file_format& fileFormat)
{
	BAutolock _(this);
	fEncoder->SetMediaFileFormat(fileFormat);

	Settings::Current().SetOutputFileFormat(fileFormat.pretty_name);

	BMessage message(kMsgControllerMediaFileFormatChanged);
	message.AddString("format_name", fileFormat.pretty_name);
	SendNotices(kMsgControllerMediaFileFormatChanged, &message);

	UpdateMediaFormatAndCodecsForCurrentFamily();
}


void
BSCApp::SetMediaCodec(const char* codecName)
{
	BAutolock _(this);
	for (int32 i = 0; i < fCodecList->CountItems(); i++) {
		media_codec_info* codec = fCodecList->ItemAt(i);
		if (!strcmp(codec->pretty_name, codecName)) {
			fEncoder->SetMediaCodecInfo(*codec);
			Settings::Current().SetOutputCodec(codec->pretty_name);
			BMessage message(kMsgControllerCodecChanged);
			message.AddString("codec_name", codec->pretty_name);
			SendNotices(kMsgControllerCodecChanged, &message);
			break;
		}
	}
}


BString
BSCApp::MediaCodecName() const
{
	BAutolock _(const_cast<BSCApp*>(this));
	return fEncoder->MediaCodecInfo().pretty_name;
}


status_t
BSCApp::GetCodecsList(BObjectList<media_codec_info>& codecList) const
{
	BAutolock _(const_cast<BSCApp*>(this));
	codecList = *fCodecList;
	return B_OK;
}


media_format
BSCApp::_ComputeMediaFormat(const int32 &width, const int32 &height,
	const color_space &colorSpace, const float &fieldRate)
{
	media_format format;
	format.type = B_MEDIA_RAW_VIDEO;
	format.u.raw_video.display.line_width = width;
	format.u.raw_video.display.line_count = height;
	format.u.raw_video.last_active = format.u.raw_video.display.line_count - 1;
	
	size_t pixelChunk;
	size_t rowAlign;
	size_t pixelPerChunk;
	status_t status = get_pixel_size_for(colorSpace, &pixelChunk,
			&rowAlign, &pixelPerChunk);
	if (status != B_OK)
		throw status;

	format.u.raw_video.display.bytes_per_row = width * rowAlign;
	format.u.raw_video.display.format = colorSpace;
	format.u.raw_video.interlace = 1;
	format.u.raw_video.field_rate = fieldRate; // Frames per second, overwritten later
	format.u.raw_video.pixel_width_aspect = 1;	// square pixels
	format.u.raw_video.pixel_height_aspect = 1;
	
	return format;
}


// Should be called every time the media_format_family is changed
status_t
BSCApp::UpdateMediaFormatAndCodecsForCurrentFamily()
{
	BAutolock _(this);
	
	const Settings& settings = Settings::Current();
	BRect targetRect = settings.TargetRect();
	targetRect.right++;
	targetRect.bottom++;
	
	const float frameRate = float(settings.CaptureFrameRate());
	const media_format mediaFormat = _ComputeMediaFormat(targetRect.IntegerWidth(),
									targetRect.IntegerHeight(),
									settings.ClipDepth(), frameRate);
	
	fEncoder->SetMediaFormat(mediaFormat);
	
	delete fCodecList;
	fCodecList = new BObjectList<media_codec_info> (1, true);
	
	// Handle the NULL/GIF media_file_formats
	media_file_format fileFormat = fEncoder->MediaFileFormat();
	if ((::strcmp(fileFormat.short_name, NULL_FORMAT_SHORT_NAME) != 0) &&
		(::strcmp(fileFormat.short_name, GIF_FORMAT_SHORT_NAME) != 0)) {
		int32 cookie = 0;
		media_codec_info codec;
		media_format dummyFormat;
		while (get_next_encoder(&cookie, &fileFormat, &mediaFormat,
				&dummyFormat, &codec) == B_OK) {
			media_codec_info* newCodec = new media_codec_info;
			*newCodec = codec;
			fCodecList->AddItem(newCodec);
		}
	}

	SendNotices(kMsgControllerCodecListUpdated);
	return B_OK;
}


void
BSCApp::UpdateDirectInfo(direct_buffer_info* info)
{
	BAutolock _(this);
	if (!fDirectWindowAvailable)
		fDirectWindowAvailable = true;
	fDirectInfo = *info;
}


status_t
BSCApp::ReadBitmap(BBitmap* bitmap, bool includeCursor, BRect bounds)
{
	const bool &useDirectWindow = fDirectWindowAvailable && Settings::Current().UseDirectWindow();
	
	if (!useDirectWindow)
		return BScreen().ReadBitmap(bitmap, includeCursor, &bounds);
		
	const int32 bytesPerPixel = fDirectInfo.bits_per_pixel >> 3;
	if (bytesPerPixel <= 0)
		return B_ERROR;

	const uint32 rowBytes = fDirectInfo.bytes_per_row / bytesPerPixel;
	const int32 offset = ((uint32)bounds.left +
		 ((uint32)bounds.top * rowBytes)) * bytesPerPixel;

	const int32 height = bounds.IntegerHeight() + 1;
	void* from = (void*)((uint8*)fDirectInfo.bits + offset);    		
	void* to = bitmap->Bits();
	const int32 bytesPerRow = bitmap->BytesPerRow();
	const int32 areaSize = (bounds.IntegerWidth() + 1) * bytesPerPixel;
	for (int32 y = 0; y < height; y++) {  
		::memcpy(to, from, areaSize);
	   	to = (void*)((uint8*)to + bytesPerRow);
		from = (void*)((uint8*)from + fDirectInfo.bytes_per_row);
	}

	return B_OK;
}


void
BSCApp::StartCapture()
{
	fNumFrames = 0;
	try {
		if (fFileList == NULL)
			fFileList = new FramesList();
	} catch (status_t& error) { 
		BMessage message(kMsgControllerCaptureStopped);
		message.AddInt32("status", error);
		SendNotices(kMsgControllerCaptureStopped, &message);
		return;
	} catch (...) {
		BMessage message(kMsgControllerCaptureStopped);
		message.AddInt32("status", int32(B_ERROR));
		SendNotices(kMsgControllerCaptureStopped, &message);
		return;
	}

	fKillCaptureThread = false;
	fPaused = false;

	fCaptureThread = spawn_thread((thread_entry)CaptureStarter,
		"Capture Thread", B_DISPLAY_PRIORITY, this);

	if (fCaptureThread < 0) {
		BMessage message(kMsgControllerCaptureStopped);
		message.AddInt32("status", fCaptureThread);
		SendNotices(kMsgControllerCaptureStopped, &message);	
		return;
	}

	status_t status = resume_thread(fCaptureThread);
	if (status < B_OK) {
		kill_thread(fCaptureThread);
		BMessage message(kMsgControllerCaptureStopped);
		message.AddInt32("status", status);
		SendNotices(kMsgControllerCaptureStopped, &message);
		return;
	}

	delete fRecordWatch;
	fRecordWatch = new BStopWatch("record_time", true);

	delete fStopRunner;
	fStopRunner = NULL;

	if (fRequestedRecordTime != 0) {
		BMessenger messenger(NULL, this);
		// TODO: Use a specific message instead of kMsgGUIToggleCapture
		// since this could trigger start instead of stopping
		fStopRunner = new BMessageRunner(messenger,
							new BMessage(kMsgGUIToggleCapture),
							fRequestedRecordTime, 1);
		// reset requested record time, so
		// it won't be used for the next time
		// TODO: rework this
		fRequestedRecordTime = 0;
	}
	SendNotices(kMsgControllerCaptureStarted);
}


void
BSCApp::EndCapture()
{
	BAutolock _(this);
	if (fCaptureThread > 0) {
		fPaused = false;
		fKillCaptureThread = true;
		status_t unused;
		wait_for_thread(fCaptureThread, &unused);
	}

	fRecordWatch->Suspend();
	SendNotices(kMsgControllerCaptureStopped);

	EncodeMovie();
}


void
BSCApp::ResetSettings()
{
	BAutolock _(this);
	Settings::ResetToDefaults();

	_UpdateFromSettings();

	BMessage message(kMsgControllerResetSettings);
	SendNotices(kMsgControllerResetSettings, &message);
}


void
BSCApp::TestSystem()
{
	std::cout << "Testing system speed:" << std::endl;
	int32 numFrames = 500;
	FramesList* list = new FramesList(true);

	BRect rect = BScreen().Frame();
	color_space colorSpace = BScreen().ColorSpace();
	bigtime_t startTime = system_time();

	for (int32 i = 0; i < numFrames; i++)
		list->AddItem(new BBitmap(rect, colorSpace), system_time());

	bigtime_t elapsedTime = system_time() - startTime;

	delete list;

	std::cout << "Took " << (elapsedTime / 1000) << " msec to write ";
	std::cout << numFrames << " frames: " << (numFrames / (elapsedTime / 1000000)) << " fps.";
	std::cout << std::endl;
}


void
BSCApp::_PauseCapture()
{
	SendNotices(kMsgControllerCapturePaused);
	fRecordWatch->Suspend();

	BAutolock _(this);
	fPaused = true;
	suspend_thread(fCaptureThread);
}


void
BSCApp::_ResumeCapture()
{
	BAutolock _(this);
	resume_thread(fCaptureThread);
	fPaused = false;

	fRecordWatch->Resume();
	SendNotices(kMsgControllerCaptureResumed);
}


void
BSCApp::_EncodingFinished(const status_t status, const char* fileName)
{
	// Reminder: fFilelist is already NULL here, since it's deleted in MovieEncoder,
	// except when Encoder didn't start at all, where it's deleted in EncodeMovie()
	// TODO: Review fFileList ownership policy
	fEncoderThread = -1;
	fNumFrames = 0;

	const Settings& settings = Settings::Current();
	// TODO: Remove special case handling
	media_file_format fileFormat = fEncoder->MediaFileFormat();
	BPath destFile = fileName;
	if (::strcmp(fileFormat.short_name, NULL_FORMAT_SHORT_NAME) != 0) {
		// Move temporary file to the correct destination
		BEntry sourceFile(fileName);
		destFile = GetUniqueFileName(settings.OutputFileName().String());
		BPath parent;
		destFile.GetParent(&parent);
		BDirectory dir(parent.Path());
		sourceFile.MoveTo(&dir, destFile.Path());
	}
	BMessage message(kMsgControllerEncodeFinished);
	message.AddInt32("status", (int32)status);
	if (fileName != NULL)
		message.AddString("file_name", destFile.Path());
	SendNotices(kMsgControllerEncodeFinished, &message);

	if (settings.QuitWhenFinished())
		be_app->PostMessage(B_QUIT_REQUESTED);
}


void
BSCApp::_HandleTargetFrameChanged(const BRect& targetRect)
{
	UpdateMediaFormatAndCodecsForCurrentFamily();

	BMessage targetFrameMessage(kMsgControllerTargetFrameChanged);
	targetFrameMessage.AddRect("frame", targetRect);
	SendNotices(kMsgControllerTargetFrameChanged, &targetFrameMessage);
}


void
BSCApp::_TestWaitForRetrace()
{
	fSupportsWaitForRetrace = BScreen().WaitForRetrace() == B_OK;
}


void
BSCApp::_WaitForRetrace(bigtime_t time)
{
	if (fSupportsWaitForRetrace)
		BScreen().WaitForRetrace(time);
	else
		snooze(time);
}


void
BSCApp::_UpdateFromSettings()
{
	const Settings& settings = Settings::Current();
	BRect rect = settings.CaptureArea();
	SetCaptureArea(rect);

	BString fileFormatName = settings.OutputFileFormat();
	media_file_format fileFormat;
	if (!GetMediaFileFormat(fileFormatName, &fileFormat)) {
		if (!GetMediaFileFormat("", &fileFormat))
			throw "Unable to find a suitable media_file_format!";
	}

	SetMediaFileFormat(fileFormat);

	const BString codecName = settings.OutputCodec();
	if (codecName != "")
		SetMediaCodec(codecName);
	else if (fCodecList != NULL && fCodecList->ItemAt(0) != NULL)
		SetMediaCodec(fCodecList->ItemAt(0)->pretty_name);
}


void
BSCApp::_DumpSettings() const
{
	Settings::Current().PrintToStream();
}


int32
BSCApp::CaptureThread()
{
	const Settings& settings = Settings::Current();
	BRect bounds = settings.CaptureArea();
	int32 frameRate = settings.CaptureFrameRate();
	if (frameRate <= 0)
		frameRate = 10;
	bigtime_t captureDelay = 1000 * (1000 / frameRate);

#if 0
	TestSystem();
#endif

	_TestWaitForRetrace();

	const int32 windowEdge = settings.WindowFrameEdgeSize();
	int32 token = GetWindowTokenForFrame(bounds, windowEdge);
	status_t error = B_ERROR;
	BBitmap *bitmap = NULL;
	const color_space colorSpace = BScreen().ColorSpace();
	while (!fKillCaptureThread) {
		if (!fPaused) {		
			if (token != -1) {
				BRect windowBounds = GetWindowFrameForToken(token, windowEdge);
				if (windowBounds.IsValid())
					bounds.OffsetTo(windowBounds.LeftTop());
			}
				
			bitmap = new (std::nothrow) BBitmap(bounds, colorSpace);
			if (bitmap == NULL) {
				error = B_NO_MEMORY;
				std::cerr << "BSCApp::CaptureThread(): error creating bitmap: " << ::strerror(error) << std::endl;
				break;
			}

			error = bitmap->InitCheck();
			if (error != B_OK) {
				std::cerr << "BSCApp::CaptureThread(): error initializing bitmap: " << ::strerror(error) << std::endl;
				break;
			}

			error = ReadBitmap(bitmap, true, bounds);
			if (error != B_OK) {
				std::cerr << "BSCApp::CaptureThread(): error reading bitmap" << ::strerror(error) << std::endl;
				break;
			}

			bigtime_t lastFrameTime = system_time();

			// Takes ownership of the bitmap
			if (!fFileList->AddItem(bitmap, lastFrameTime)) {
				error = B_NO_MEMORY;
				std::cerr << "BSCApp::CaptureThread(): error adding bitmap to frame list: " << ::strerror(error) << std::endl;
				break;
			}

			atomic_add(&fNumFrames, 1);

			bigtime_t toWait = (lastFrameTime + captureDelay) - system_time();
			if (toWait > 0)
				_WaitForRetrace(toWait); // Wait for Vsync
		} else
			snooze(500000);
	}
	
	fCaptureThread = -1;
	fKillCaptureThread = true;

	if (error != B_OK) {
		delete bitmap;
		BMessage message(kMsgControllerCaptureStopped);
		message.AddInt32("status", (int32)error);
		SendNotices(kMsgControllerCaptureStopped, &message);
		
		delete fFileList;
		fFileList = NULL;
	}
	
	return B_OK;
}


/* static */
int32
BSCApp::CaptureStarter(void *arg)
{
	return static_cast<BSCApp*>(arg)->CaptureThread();
}


void
BSCApp::_UsageRequested()
{
}
