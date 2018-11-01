#include "Controller.h"

#include "Constants.h"
#include "ControllerObserver.h"
#include "FramesList.h"
#include "FunctionObject.h"
#include "MovieEncoder.h"
#include "Settings.h"
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
#include <StopWatch.h>
#include <String.h>


BLooper *gControllerLooper = NULL;


// If prettyName is NULL, returns the first found media_file_format,
// otherwise returns the media_file_format with the same prettyName
// TODO: Use the short_name instead
static bool
get_media_file_format(const char* prettyName, media_file_format* outFormat)
{
	const uint32 mediaFormatMask = media_file_format::B_KNOWS_ENCODED_VIDEO
								| media_file_format::B_WRITABLE;								
	media_file_format mediaFileFormat;
	int32 cookie = 0;
	while (get_next_file_format(&cookie, &mediaFileFormat) == B_OK) {
		if ((mediaFileFormat.capabilities & mediaFormatMask) == mediaFormatMask
			&& (prettyName == NULL
				|| strcmp(mediaFileFormat.pretty_name, prettyName) == 0)) {
			*outFormat = mediaFileFormat;
			return true;
		}
	}
	
	return false;
}
	
	
Controller::Controller()
	:
	BLooper("Controller"),
	fCaptureThread(-1),
	fNumFrames(0),
	fRecordWatch(NULL),
	fKillCaptureThread(true),
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
	BString name = settings.OutputFileName();
	fEncoder->SetOutputFile(name.String());

	BRect rect = settings.CaptureArea();
	SetCaptureArea(rect);
	
	BString fileFormatName;
	settings.GetOutputFileFormat(fileFormatName);
	
	media_file_format fileFormat;
	if (!get_media_file_format(fileFormatName.String(), &fileFormat)) {
		if (!get_media_file_format(NULL, &fileFormat))
			throw "Unable to find a suitable media_file_format!";
	}		
	
	SetMediaFileFormat(fileFormat);
	
	BString codecName;
	settings.GetOutputCodec(codecName);
	
	if (codecName != "")
		SetMediaCodec(codecName);
	else
		SetMediaCodec(fCodecList->ItemAt(0)->pretty_name);
	Run();
}


Controller::~Controller()
{
	delete fRecordWatch;
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

		case kMsgGUIToggleCapture:
			ToggleCapture();
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
			SendNotices(kMsgControllerEncodeProgress, &progressMessage);
			break;
		}
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
	switch (State()) {
		case STATE_RECORDING:
			reason = "Recording is in progress.";
			break;
		case STATE_ENCODING:
			reason = "Encoding is in progress.";
			break;
		case STATE_IDLE:
			return true;
		default:
			break;
	}
	
	return false;
}


void
Controller::Cancel()
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
Controller::State() const
{
	BAutolock _((BLooper*)this);
	if (fCaptureThread > 0)
		return STATE_RECORDING;
	
	if (fEncoderThread > 0 || fFileList != NULL)
		return STATE_ENCODING;
	
	return STATE_IDLE;
}


void
Controller::ToggleCapture()
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
Controller::TogglePause()
{
	BAutolock _(this);
	if (fCaptureThread < 0)
		return;
		
	if (fPaused)
		_ResumeCapture();
	else if (!fPaused)
		_PauseCapture();
}


int32
Controller::RecordedFrames() const
{
	return atomic_get((int32*)&fNumFrames);
}


bigtime_t
Controller::RecordTime() const
{
	return fRecordWatch != NULL ? fRecordWatch->ElapsedTime() : 0;
}


void
Controller::EncodeMovie()
{
	BAutolock _(this);
	
	int32 numFrames = fFileList->CountItems();
	if (numFrames <= 0) {
		_EncodingFinished(B_ERROR, NULL);
		delete fFileList;
		fFileList = NULL;
		return;
	}
			
	BString fileName;
	Settings().GetOutputFileName(fileName);
	BEntry entry(fileName.String());
	if (entry.Exists()) {
		// file exists.
		fileName = GetUniqueFileName(fileName, MediaFileFormat().file_extension);
	}
	
	SetOutputFileName(fileName);
	
	SendNotices(kMsgControllerEncodeStarted);
		 
	BMessage message(kMsgControllerEncodeProgress);
	message.AddInt32("frames_total", numFrames);
	message.AddInt32("frames_remaining", numFrames);
	
	SendNotices(kMsgControllerEncodeProgress, &message);
	
	fEncoder->SetSource(fFileList);
	fFileList = NULL;
	
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
	Settings settings;
	settings.SetCaptureArea(rect);
	BRect targetRect = settings.TargetRect();
	fEncoder->SetDestFrame(targetRect);
	
	BMessage message(kMsgControllerSourceFrameChanged);
	message.AddRect("frame", rect);
	SendNotices(kMsgControllerSourceFrameChanged, &message);

	_HandleTargetFrameChanged(targetRect);
}


void
Controller::SetCaptureFrameDelay(const int milliSeconds)
{
	BAutolock _(this);
	Settings().SetCaptureFrameDelay(milliSeconds);
	
	BMessage message(kMsgControllerCaptureFrameDelayChanged);
	message.AddInt32("delay", milliSeconds);
	SendNotices(kMsgControllerCaptureFrameDelayChanged, &message);
}


void
Controller::SetPlaybackFrameRate(const int rate)
{
	BAutolock _(this);
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
			Settings().SetOutputCodec(codec->pretty_name);
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


media_format
Controller::_ComputeMediaFormat(const int32 &width, const int32 &height,
	const color_space &colorSpace, const int32 &fieldRate)
{
	media_format initialFormat;
	initialFormat.type = B_MEDIA_RAW_VIDEO;
	initialFormat.u.raw_video.display.line_width = width;
	initialFormat.u.raw_video.display.line_count = height;
	initialFormat.u.raw_video.last_active = initialFormat.u.raw_video.display.line_count - 1;
	
	size_t pixelChunk;
	size_t rowAlign;
	size_t pixelPerChunk;

	status_t status;
	status = get_pixel_size_for(colorSpace, &pixelChunk,
			&rowAlign, &pixelPerChunk);
	if (status != B_OK)
		throw status;

	initialFormat.u.raw_video.display.bytes_per_row = width * rowAlign;			
	initialFormat.u.raw_video.display.format = colorSpace;
	initialFormat.u.raw_video.interlace = 1;	
	initialFormat.u.raw_video.field_rate = fieldRate; // Frames per second, overwritten later
	initialFormat.u.raw_video.pixel_width_aspect = 1;	// square pixels
	initialFormat.u.raw_video.pixel_height_aspect = 1;
	
	return initialFormat;
}


// Should be called every time the media_format_family is changed
status_t
Controller::UpdateMediaFormatAndCodecsForCurrentFamily()
{
	BAutolock _(this);
	
	Settings settings;
	BRect targetRect = settings.TargetRect();
	targetRect.right++;
	targetRect.bottom++;
	
	const int32 frameRate =  (int32)(1000 / settings.CaptureFrameDelay());
	media_format mediaFormat = _ComputeMediaFormat(targetRect.IntegerWidth(),
									targetRect.IntegerHeight(),
									settings.ClipDepth(), frameRate);
	
	fEncoder->SetMediaFormat(mediaFormat);
	
	delete fCodecList;
	fCodecList = new BObjectList<media_codec_info> (1, true);
	
	// Handle the fake media_file_format
	media_file_format fileFormat = fEncoder->MediaFileFormat();
	if (strcmp(fileFormat.short_name, FAKE_FORMAT_SHORT_NAME) != 0) {
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
			fFileList = new FramesList();
	} catch (status_t error) { 
		BMessage message(kMsgControllerCaptureStopped);
		message.AddInt32("status", error);
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

	SendNotices(kMsgControllerCaptureStarted);
}


void
Controller::EndCapture()
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
Controller::ResetSettings()
{
	Settings::SetDefaults();
	BMessage message(kMsgControllerResetSettings);
	SendNotices(kMsgControllerResetSettings, &message);
}


void
Controller::_PauseCapture()
{
	SendNotices(kMsgControllerCapturePaused);
	fRecordWatch->Suspend();
	
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
	
	fRecordWatch->Resume();
	SendNotices(kMsgControllerCaptureResumed);
}


void
Controller::_EncodingFinished(const status_t status, const char* fileName)
{
	// Reminder: fFilelist is already NULL here, since it's deleted in MovieEncoder,
	// except when Encoder didn't start at all, where it's deleted in EncodeMovie()
	// TODO: Review fFileList ownership policy

	fEncoderThread = -1;
	fNumFrames = 0;

	BMessage message(kMsgControllerEncodeFinished);
	message.AddInt32("status", (int32)status);
	if (fileName != NULL)
		message.AddString("file_name", fileName);
	SendNotices(kMsgControllerEncodeFinished, &message);
}


void
Controller::_HandleTargetFrameChanged(const BRect& targetRect)
{
	UpdateMediaFormatAndCodecsForCurrentFamily();
	
	BMessage targetFrameMessage(kMsgControllerTargetFrameChanged);
	targetFrameMessage.AddRect("frame", targetRect);
	SendNotices(kMsgControllerTargetFrameChanged, &targetFrameMessage);
}


void
Controller::_TestWaitForRetrace()
{
	fSupportsWaitForRetrace = BScreen().WaitForRetrace() == B_OK;
}


void
Controller::_WaitForRetrace(bigtime_t time)
{
	if (fSupportsWaitForRetrace)
		BScreen().WaitForRetrace(time);
	else
		snooze(time);		
}


void
Controller::_DumpSettings() const
{
	Settings().PrintToStream();
}


int32
Controller::CaptureThread()
{
	Settings settings;
	BScreen screen;
	BRect bounds = settings.CaptureArea();
	bigtime_t captureDelay = (bigtime_t)settings.CaptureFrameDelay() * 1000;
	
	// TODO: Validate captureDelay with some limits

	_TestWaitForRetrace();
	
	const int32 windowBorder = settings.WindowFrameBorderSize();
	int32 token = GetWindowTokenForFrame(bounds, windowBorder);
	status_t error = B_ERROR;
	BBitmap *bitmap = NULL;
	while (!fKillCaptureThread) {
		if (!fPaused) {		
			if (token != -1) {
				BRect windowBounds = GetWindowFrameForToken(token, windowBorder);
				if (windowBounds.IsValid())
					bounds.OffsetTo(windowBounds.LeftTop());
			}
				
			_WaitForRetrace(captureDelay); // Wait for Vsync
			bitmap = new BBitmap(bounds, screen.ColorSpace());
			error = ReadBitmap(bitmap, true, bounds);
			bigtime_t currentTime = system_time();

			if (error != B_OK)
				break;

			// Takes ownership of the bitmap
			if (!fFileList->AddItem(bitmap, currentTime)) {
				error = B_NO_MEMORY;
				break;
			}

			atomic_add(&fNumFrames, 1);
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
Controller::CaptureStarter(void *arg)
{
	return static_cast<Controller *>(arg)->CaptureThread();
}
