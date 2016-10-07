#include "Controller.h"
#include "ControllerObserver.h"
#include "FileList.h"
#include "FunctionObject.h"
#include "MovieEncoder.h"
#include "Settings.h"
#include "messages.h"
#include "Utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>

#include <Autolock.h>
#include <Bitmap.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Screen.h>
#include <String.h>


BLooper *gControllerLooper = NULL;

Controller::Controller()
	:
	BLooper("Controller"),
	fCaptureThread(-1),
	fNumFrames(0),
	fKillThread(true),
	fPaused(false),
	fDirectWindowAvailable(false),
	fEncoder(NULL),
	fEncoderThread(-1),
	fFileList(NULL),
	fCodecList(NULL)
{
	memset(&fDirectInfo, 0, sizeof(fDirectInfo));
	
	fEncoder = new MovieEncoder;

	Settings settings;	
	BString name;
	settings.GetOutputFileName(name);
	fEncoder->SetOutputFile(name.String());

	BRect rect;
	settings.GetCaptureArea(rect);
	SetCaptureArea(rect);
	
	Run();
}


Controller::~Controller()
{
	delete fEncoder;
	delete fCodecList;
	delete fFileList;
}


void
Controller::MessageReceived(BMessage *message)
{
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
		
		case kMsgGUIStartCapture:
		case kMsgGUIStopCapture:
			if (fEncoderThread < 0)
				ToggleCapture();
			break;
		
		case kPauseResumeCapture:
			TogglePause();
			break;
			
		case kEncodingFinished:
		{
			status_t error;
			message->FindInt32("status", (int32*)&error);
			_EncodingFinished(error);
			break;
		}
		
		case B_UPDATE_STATUS_BAR:
		case B_RESET_STATUS_BAR:
			SendNotices(message->what, message);
			break;
				
		default:	
			BLooper::MessageReceived(message);
			break;
	}
}


bool
Controller::QuitRequested()
{
	if (fCaptureThread < 0 && fFileList == NULL && fEncoderThread < 0)
		return BLooper::QuitRequested();
	
	return false;
}


bool
Controller::CanQuit(BString& reason) const
{
	BAutolock _((BLooper*)this);
	if (fCaptureThread > 0)
		reason = "Recording is in progress.";
	else if (fFileList != NULL)
		reason = "Encoding is in progress.";
		
	return fCaptureThread < 0 && fFileList == NULL && fEncoderThread < 0;
}


void
Controller::Cancel()
{
	if (fCaptureThread > 0) {
		fKillThread = true;
		status_t status;
		wait_for_thread(fCaptureThread, &status);
		fCaptureThread = -1;
	} else if (fEncoderThread > 0) {
		fEncoder->Cancel();
		fEncoderThread = -1;
	}
}


void
Controller::ToggleCapture()
{
	BAutolock _(this);
	if (fCaptureThread < 0)
		StartCapture();
	else
		EndCapture();
}


void
Controller::TogglePause()
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
Controller::RecordedFrames() const
{
	return atomic_get((int32*)&fNumFrames);
}


void
Controller::EncodeMovie()
{
	BAutolock _(this);
	
	BString fileName;
	Settings().GetOutputFileName(fileName);
	BEntry entry(fileName.String());
	if (entry.Exists()) {
		// file exists.
		fileName = GetUniqueFileName(fileName, MediaFileFormat().file_extension);
	}
	
	fEncoder->SetOutputFile(fileName);
	
	SendNotices(kMsgControllerEncodeStarted);
		 
	BMessage message(kMsgControllerEncodeProgress);
	message.AddInt32("num_files", fFileList->CountItems());
	
	SendNotices(kMsgControllerEncodeProgress, &message);
	
	fEncoder->SetSource(fFileList);
	
	BMessenger messenger(this);
	fEncoder->SetMessenger(messenger);

	fEncoderThread = fEncoder->EncodeThreaded();
}


void
Controller::SetUseDirectWindow(const bool& use)
{
	BAutolock _(this);
	Settings().SetUseDirectWindow(use);
}


void
Controller::SetCaptureArea(const BRect& rect)
{
	BAutolock _(this);
	Settings().SetCaptureArea(rect);
	
	fEncoder->SetDestFrame(Settings().TargetRect());
			
	BMessage message(kMsgControllerSourceFrameChanged);
	message.AddRect("frame", rect);
	SendNotices(kMsgControllerSourceFrameChanged, &message);

	BMessage targetFrameMessage(kMsgControllerTargetFrameChanged);
	// TODO: Move this to its own method
	BRect targetRect = Settings().TargetRect();
	targetFrameMessage.AddRect("frame", targetRect);
	SendNotices(kMsgControllerTargetFrameChanged, &targetFrameMessage);
}


void
Controller::SetScale(const float &scale)
{
	BAutolock _(this);
	Settings().SetScale(scale);
	BRect rect(Settings().TargetRect());
	fEncoder->SetDestFrame(rect);
	BMessage message(kMsgControllerTargetFrameChanged);
	message.AddRect("frame", rect);
	message.AddFloat("scale", scale);
	SendNotices(kMsgControllerTargetFrameChanged, &message);
}


void
Controller::SetVideoDepth(const color_space &space)
{
	BAutolock _(this);
	fEncoder->SetColorSpace(space);
	Settings().SetClipDepth(space);
	SendNotices(kMsgControllerVideoDepthChanged);
}


void
Controller::SetOutputFileName(const char *name)
{
	BAutolock _(this);
	Settings().SetOutputFileName(name);
	fEncoder->SetOutputFile(name);
}


media_format_family
Controller::MediaFormatFamily() const
{
	BAutolock _((BLooper*)this);
	return fEncoder->MediaFormatFamily();
}


void
Controller::SetMediaFormatFamily(const media_format_family& family)
{
	BAutolock _(this);
	fEncoder->SetMediaFormatFamily(family);
	UpdateMediaFormatAndCodecsForCurrentFamily();
}


media_file_format
Controller::MediaFileFormat() const
{
	BAutolock _((BLooper*)this);
	return fEncoder->MediaFileFormat();
}


BString
Controller::MediaFileFormatName() const
{
	BAutolock _((BLooper*)this);
	return fEncoder->MediaFileFormat().pretty_name;
}


void
Controller::SetMediaFileFormat(const media_file_format& fileFormat)
{
	BAutolock _(this);
	fEncoder->SetMediaFileFormat(fileFormat);

	Settings().SetOutputFileFormat(fileFormat.pretty_name);
	
	BMessage message(kMsgControllerMediaFileFormatChanged);
	message.AddString("format_name", fileFormat.pretty_name);
	SendNotices(kMsgControllerMediaFileFormatChanged, &message);
	
	UpdateMediaFormatAndCodecsForCurrentFamily();
}


void
Controller::SetMediaCodec(const char* codecName)
{
	BAutolock _(this);
	for (int32 i = 0; i < fCodecList->CountItems(); i++) {
		media_codec_info* codec = fCodecList->ItemAt(i);
		if (!strcmp(codec->pretty_name, codecName)) {
			fEncoder->SetMediaCodecInfo(*codec);
			BMessage message(kMsgControllerCodecChanged);
			message.AddString("codec_name", codec->pretty_name);
			SendNotices(kMsgControllerCodecChanged, &message);
			break;
		}
	}
}


BString
Controller::MediaCodecName() const
{
	BAutolock _((BLooper*)this);
	return fEncoder->MediaCodecInfo().pretty_name;
}


status_t
Controller::GetCodecsList(BObjectList<media_codec_info>& codecList) const
{
	BAutolock _((BLooper*)this);
	codecList = *fCodecList;
	return B_OK;
}
	

status_t
Controller::UpdateMediaFormatAndCodecsForCurrentFamily()
{
	BAutolock _(this);
	
	Settings settings;
	BRect targetRect = settings.TargetRect();
	targetRect.right++;
	targetRect.bottom++;
	
	media_format mediaFormat;
	status_t status;
	status = UpdateMediaFormat(targetRect.IntegerWidth(), targetRect.IntegerHeight(),
		settings.ClipDepth(), 10, mediaFormat);
	if (status != B_OK)
		return status;

	fEncoder->SetMediaFormat(mediaFormat);
	
	media_file_format fileFormat = fEncoder->MediaFileFormat();
	
	delete fCodecList;
	fCodecList = new BObjectList<media_codec_info> (1, true);
	
	int32 cookie = 0;
	media_codec_info codec;
	media_format dummyFormat;
	while (get_next_encoder(&cookie, &fileFormat, &mediaFormat,
			&dummyFormat, &codec) == B_OK) {
		media_codec_info* newCodec = new media_codec_info;
		*newCodec = codec;
		fCodecList->AddItem(newCodec);
	}
		
	SendNotices(kMsgControllerCodecListUpdated);
	return B_OK;
}


void
Controller::UpdateDirectInfo(direct_buffer_info* info)
{
	BAutolock _(this);
	if (!fDirectWindowAvailable)
		fDirectWindowAvailable = true;
	fDirectInfo = *info;
}


status_t
Controller::ReadBitmap(BBitmap* bitmap, bool includeCursor, BRect bounds)
{
	const bool &useDirectWindow = Settings().UseDirectWindow()
		&& fDirectWindowAvailable;
	
	if (!useDirectWindow)
		return BScreen().ReadBitmap(bitmap, includeCursor, &bounds);
		
	int32 bytesPerPixel = fDirectInfo.bits_per_pixel >> 3;
	if (bytesPerPixel <= 0)
		return B_ERROR;

	uint32 rowBytes = fDirectInfo.bytes_per_row / bytesPerPixel;
	const int32 offset = ((uint32)bounds.left +
		 ((uint32)bounds.top * rowBytes)) * bytesPerPixel;

	int32 height = bounds.IntegerHeight() + 1;
	void* from = (void*)((uint8*)fDirectInfo.bits + offset);    		
	void* to = bitmap->Bits();
	int32 bytesPerRow = bitmap->BytesPerRow();
	int32 areaSize = (bounds.IntegerWidth() + 1) * bytesPerPixel;
	for (int32 y = 0; y < height; y++) {  
		memcpy(to, from, areaSize);
	   	to = (void*)((uint8*)to + bytesPerRow);
		from = (void*)((uint8*)from + fDirectInfo.bytes_per_row);
	}
	 
	return B_OK;
}



void
Controller::StartCapture()
{
	fNumFrames = 0;
	try {
		if (fFileList == NULL)
			fFileList = new FileList();
	} catch (status_t error) { 
		BMessage message(kMsgControllerCaptureFailed);
		message.AddInt32("status", error);
		SendNotices(kMsgControllerCaptureFailed, &message);
		return;
	}
	
	fKillThread = false;
	fPaused = false;
	
	fCaptureThread = spawn_thread((thread_entry)CaptureStarter,
		"Capture Thread", B_DISPLAY_PRIORITY, this);
					
	if (fCaptureThread < 0) {
		BMessage message(kMsgControllerCaptureFailed);
		message.AddInt32("status", fCaptureThread);
		SendNotices(kMsgControllerCaptureFailed, &message);	
		return;
	}
		
	status_t status = resume_thread(fCaptureThread);
	if (status < B_OK) {
		kill_thread(fCaptureThread);
		BMessage message(kMsgControllerCaptureFailed);
		message.AddInt32("status", status);
		SendNotices(kMsgControllerCaptureFailed, &message);
		return;
	}

	SendNotices(kMsgControllerCaptureStarted);
}


void
Controller::EndCapture()
{
	BAutolock _(this);
	if (fCaptureThread > 0) {
		fPaused = false;
		fKillThread = true;
		status_t unused;
		wait_for_thread(fCaptureThread, &unused);
	}
	
	SendNotices(kMsgControllerCaptureStopped);
	
	EncodeMovie();
}


void
Controller::_PauseCapture()
{
	SendNotices(kMsgControllerCapturePaused);
	
	BAutolock _(this);
	fPaused = true;
	suspend_thread(fCaptureThread);
}


void
Controller::_ResumeCapture()
{
	BAutolock _(this);
	resume_thread(fCaptureThread);
	fPaused = false;
	
	SendNotices(kMsgControllerCaptureResumed);
}


void
Controller::_EncodingFinished(const status_t status)
{
	// Deleting the filelist deletes the files referenced by it
	// and also the temporary folder
	delete fFileList;
	fFileList = NULL;
	
	fEncoderThread = -1;
	fNumFrames = 0;
		
	BMessage message(kMsgControllerEncodeFinished);
	message.AddInt32("status", (int32)status);
	SendNotices(kMsgControllerEncodeFinished, &message);
}


int32
Controller::CaptureThread()
{
	BScreen screen;
	BRect bounds = Settings().CaptureArea();
	int32 token = GetWindowTokenForFrame(bounds);
	bigtime_t waitTime = 0;
	status_t error = B_ERROR;
	while (!fKillThread) {
		if (!fPaused) {		
			if (token != -1)
				bounds = GetWindowFrameForToken(token);
				
			screen.WaitForRetrace(waitTime); // Wait for Vsync
			BBitmap *bitmap = new BBitmap(bounds, screen.ColorSpace());
			error = ReadBitmap(bitmap, true, bounds);			
			
			bigtime_t currentTime = system_time();
			// Takes ownership of the bitmap
	    	if (error == B_OK && fFileList->AddItem(bitmap, currentTime))
				atomic_add(&fNumFrames, 1);
			else {
				//PRINT(("Error while getting screenshot: %s\n", strerror(error)));
				delete bitmap;
				break;
			}
		} else
			snooze(500000);
	}
	fCaptureThread = -1;
		
	return B_OK;
}


/* static */
int32
Controller::CaptureStarter(void *arg)
{
	return static_cast<Controller *>(arg)->CaptureThread();
}
