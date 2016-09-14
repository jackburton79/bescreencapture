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
#include <FindDirectory.h>
#include <Screen.h>
#include <String.h>
#include <TranslationKit.h>
#include <TranslatorRoster.h>


static BTranslatorRoster* sTranslatorRoster = NULL;


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
	if (sTranslatorRoster == NULL)
		sTranslatorRoster = BTranslatorRoster::Default();

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
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 what;
			message->FindInt32("be:observe_change_what", &what);
			switch (what) {
				case kClipSizeChanged:
				{
					std::cout << "Controller: kClipSizeChanged" << std::endl;
					message->PrintToStream();
					int32 value = 0;
					if (message->FindInt32("be:value", &value) == B_OK)
						
					Settings().PrintToStream();
					break;
				}
				default:
					break;
			}
			break;
		}
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
	if (fCaptureThread < 0 && fTemporaryPath == NULL && fEncoderThread < 0)
		return BLooper::QuitRequested();
	
	return false;
}


bool
Controller::CanQuit(BString& reason) const
{
	BAutolock _((BLooper*)this);
	if (fCaptureThread > 0)
		reason = "Recording is in progress.";
	else if (fTemporaryPath != NULL)
		reason = "Encoding is in progress.";
		
	return fCaptureThread < 0 && fTemporaryPath == NULL && fEncoderThread < 0;
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
		fEncoder->SetOutputFile(fileName);
	}
	
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
		
	UpdateAreaDescription(rect);
	
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
	UpdateAreaDescription(Settings().CaptureArea());
}


status_t
Controller::ReadBitmap(BBitmap* bitmap, BRect bounds)
{
	void* from = (void*)((uint8*)fDirectInfo.bits + fAreaDesc.offset);    		
	void* to = bitmap->Bits();
	 
	const int32& bytesPerRow = bitmap->BytesPerRow();
	   		
	for (int32 y = 0; y < fAreaDesc.height; y++) {  
		memcpy(to, from, fAreaDesc.size);
	   	to = (void*)((uint8*)to + bytesPerRow);
		from = (void*)((uint8*)from + fDirectInfo.bytes_per_row);
	}
	 
	return B_OK;
}


void
Controller::UpdateAreaDescription(const BRect &rect)
{
	int32 bytesPerPixel = fDirectInfo.bits_per_pixel >> 3;
	if (bytesPerPixel <= 0)
		return;
		
	uint32 rowBytes = fDirectInfo.bytes_per_row / bytesPerPixel;	    		
		
	fAreaDesc.size = (rect.IntegerWidth() + 1) * bytesPerPixel;
	fAreaDesc.height = rect.IntegerHeight() + 1;
	fAreaDesc.offset = ((uint32)rect.left +
		 ((uint32)rect.top * rowBytes)) * bytesPerPixel;
}


void
Controller::StartCapture()
{
	fNumFrames = 0;
	
	BPath path;
	status_t status = find_directory(B_SYSTEM_TEMP_DIRECTORY, &path);
	if (status != B_OK) {
		BMessage message(kMsgControllerCaptureFailed);
		message.AddInt32("status", status);
		SendNotices(kMsgControllerCaptureFailed, &message);
		return;
	}
	
	// Create temporary path
	fTemporaryPath = tempnam((char*)path.Path(), (char*)"_BSC");
	status = create_directory(fTemporaryPath, 0777);
	if (status < B_OK) {
		BMessage message(kMsgControllerCaptureFailed);
		message.AddInt32("status", status);
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
		
	status = resume_thread(fCaptureThread);
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
	BEntry(fTemporaryPath).Remove();
	free(fTemporaryPath);
	fTemporaryPath = NULL;

	fEncoderThread = -1;
	
	delete fFileList;
	fFileList = NULL;
	
	BMessage message(kMsgControllerEncodeFinished);
	message.AddInt32("status", (int32)status);
	SendNotices(kMsgControllerEncodeFinished, &message);
}


int32
Controller::CaptureThread()
{
	const bool &useDirectWindow = Settings().UseDirectWindow();
	BScreen screen;
	BRect bounds = Settings().CaptureArea();
	
	bigtime_t waitTime = 1000000 / /*fCaptureOptions.framesPerSecond*/10;
	BFile outFile;
	BBitmap *bitmap = new BBitmap(bounds, screen.ColorSpace());
	
	bool firstFrame = true;
	translator_info translatorInfo;
	
	if (fFileList == NULL)
		fFileList = new FileList();
		
	status_t error = B_ERROR;
	while (!fKillThread) {
		if (!fPaused) {
			screen.WaitForRetrace(waitTime); // Wait for Vsync
		
			if (fDirectWindowAvailable && useDirectWindow)		
				error = ReadBitmap(bitmap, bounds);			
			else 
				error = screen.ReadBitmap(bitmap, false, &bounds);
					 
	    	if (error == B_OK) {
				//Save bitmap to disk
				BBitmapStream bitmapStream(bitmap);
				if (firstFrame) {
					firstFrame = false;
					sTranslatorRoster->Identify(&bitmapStream, NULL,
						&translatorInfo, 0, NULL, 'BMP ');
				}

				char* string = tempnam(fTemporaryPath, "frame_");
				outFile.SetTo(string, B_WRITE_ONLY|B_CREATE_FILE);
				error = sTranslatorRoster->Translate(&bitmapStream,
					&translatorInfo, NULL, &outFile, 'BMP ');	
				fFileList->AddItem(string);
				atomic_add(&fNumFrames, 1);
				
				// Cleanup
				bitmapStream.DetachBitmap(&bitmap);	
				outFile.Unset();
				free(string);
			} else {
				//PRINT(("Error while getting screenshot: %s\n", strerror(error)));
				break;
			}
		} else
			snooze(500000);
	}
		
	delete bitmap;
	
	fCaptureThread = -1;
		
	return B_OK;
}


/* static */
int32
Controller::CaptureStarter(void *arg)
{
	return static_cast<Controller *>(arg)->CaptureThread();
}
