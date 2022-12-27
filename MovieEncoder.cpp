/*
 * Copyright 2015 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
// Part of this code was taken from BitmapMovie class, by Be, Inc.
/*
----------------------
Be Sample Code License
----------------------

Copyright 1991-1999, Be Incorporated.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions, and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions, and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF TITLE, NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.    
*/


#include "MovieEncoder.h"

#include "Constants.h"
#include "FramesList.h"
#include "ImageFilter.h"
#include "Settings.h"
#include "Utils.h"

#include <Bitmap.h>
#include <Debug.h>
#include <Directory.h>
#include <Entry.h>
#include <FindDirectory.h>
#include <MediaTrack.h>
#include <View.h>

#include <iostream>


MovieEncoder::MovieEncoder()
	:
	fEncoderThread(-1),
	fKillThread(false),
	fFileList(NULL),
	fCursorQueue(NULL),
	fColorSpace(B_NO_COLOR_SPACE),
	fMediaFile(NULL),
	fMediaTrack(NULL),
	fHeaderCommitted(false)
{
}


MovieEncoder::~MovieEncoder()
{
	DisposeData();
}


void
MovieEncoder::DisposeData()
{
	// If the movie is still opened, close it; this also flushes all tracks
	if (fMediaFile)
		_CloseFile();
	
	// Deleting the filelist deletes the files referenced by it
	// and also the temporary folder
	delete fFileList;
	fFileList = NULL;
}


void
MovieEncoder::Cancel()
{
	if (fEncoderThread > 0) {
		fKillThread = true;
		status_t dummy;
		wait_for_thread(fEncoderThread, &dummy);
	}
}


status_t
MovieEncoder::SetSource(FramesList* fileList)
{
	// Takes ownership
	fFileList = fileList;
	return B_OK;
}


status_t
MovieEncoder::SetOutputFile(const char* fileName)
{
	fOutputFile.SetTo(fileName);
	return B_OK;
}


BPath
MovieEncoder::OutputFile() const
{
	return fOutputFile;
}


status_t
MovieEncoder::SetDestFrame(const BRect& rect)
{
	fDestFrame = rect;
	fDestFrame.OffsetTo(B_ORIGIN);
	
	return B_OK;
}


void
MovieEncoder::SetColorSpace(const color_space& space)
{
	fColorSpace = space;
}


status_t
MovieEncoder::SetQuality(const float& quality)
{
	return B_OK;
}


status_t
MovieEncoder::SetCursorQueue(std::queue<BPoint> *queue)
{
	fCursorQueue = queue;
	return B_OK;
}


status_t
MovieEncoder::SetMessenger(const BMessenger& messenger)
{
	if (!messenger.IsValid())
		return B_BAD_VALUE;
	
	fMessenger = messenger;
	return B_OK;
}


status_t 
MovieEncoder::_CreateFile(
	const char* path,
	const media_file_format& mediaFileFormat,
	const media_format& mediaFormat,
	const media_codec_info& mediaCodecInfo,
	float quality)
{
	std::cerr << "MovieEncoder::_CreateFile()" << std::endl;
	std::cerr << "path: " << path << std::endl;
	std::cerr << "media_file_format: " << mediaFileFormat.pretty_name << "(" << mediaFileFormat.short_name << ")" << std::endl;
	std::cerr << "media_format: " << "width: " << mediaFormat.Width();
	std::cerr << ", height: " << mediaFormat.Height();
	std::cerr << ", colorspace: " << mediaFormat.ColorSpace();
	std::cerr << std::endl;
	entry_ref ref;
	status_t status = get_ref_for_path(path, &ref);
	if (status != B_OK) {
		std::cerr << "MovieEncoder::_CreateFile(): get_ref_for_path() failed with " << ::strerror(status) << std::endl;
		return status;
	}
	BMediaFile* file = new (std::nothrow) BMediaFile(&ref, &mediaFileFormat);
	if (file == NULL)
		return B_NO_MEMORY;

	status = file->InitCheck();
	if (status == B_OK) {
		fHeaderCommitted = false;
		fMediaFile = file;

		// This next line casts away const to avoid a warning.  MediaFile::CreateTrack()
		// *should* have the input format argument declared const, but it doesn't, and
		// it can't be changed because it would break binary compatibility.  Oh, well.
		fMediaTrack = file->CreateTrack(const_cast<media_format *>(&mediaFormat), &mediaCodecInfo);
		if (fMediaTrack == NULL) {
			status = B_ERROR;
			std::cerr << "BMediaFile::CreateTrack() failed." << std::endl;
		} else {
			if (quality >= 0)
				fMediaTrack->SetQuality(quality);
		}
	} else
		std::cerr << "BMediaFile::InitCheck() failed with " << ::strerror(status) << std::endl;

	// clean up if we incurred an error
	if (status < B_OK) {
		fHeaderCommitted = false;
		delete fMediaFile;
		fMediaFile = NULL;
	}
	
	return status;
}


status_t 
MovieEncoder::_WriteFrame(const BBitmap* bitmap, int32 frameNum, bool isKeyFrame)
{
	// NULL is not a valid bitmap pointer
	if (!bitmap)
		return B_BAD_VALUE;

	ASSERT((fMediaTrack != NULL));

	// okay, it's the right kind of bitmap -- commit the header if necessary, and
	// write it as one video frame.  We defer committing the header until the first
	// frame is written in order to allow the client to adjust the image quality at
	// any time up to actually writing video data.
	status_t err = B_OK;
	if (!fHeaderCommitted) {
		isKeyFrame = true;
		err = fMediaFile->CommitHeader();
		if (err == B_OK)
			fHeaderCommitted = true;
	}

	if (err == B_OK)
		err = fMediaTrack->WriteFrames(bitmap->Bits(), 1, isKeyFrame ? B_MEDIA_KEY_FRAME : 0);
	
	return err;
}


status_t 
MovieEncoder::_CloseFile()
{
	status_t err = B_OK;
	if (fMediaFile != NULL) {
		fMediaFile->ReleaseAllTracks();
		err = fMediaFile->CloseFile();
		
		delete fMediaFile;		// deletes the track, too
		fMediaFile = NULL;
		fMediaTrack = NULL;
	}
	return err;
}


// When this is running, no member variable should be accessed
// from other threads
status_t
MovieEncoder::_EncoderThread()
{	
	std::cerr << "MovieEncoder::_EncoderThread() started" << std::endl;
	int32 framesLeft = fFileList->CountItems();
	if (framesLeft <= 0) {
		std::cerr << "MovieEncoder::_EncoderThread(): no frames to encode." << std::endl;
		_HandleEncodingFinished(B_ERROR);
		return B_ERROR;
	}

	status_t status = _ApplyImageFilters();
	if (status != B_OK) {
		std::cerr << "MovieEncoder::_EncoderThread(): error while applying filters: " << ::strerror(status) << std::endl;
		_HandleEncodingFinished(status);
		return status;
	}

	// If destination frame is not valid (I.E: something went wrong)
	// then get source frame and use it as dest frame
	if (!fDestFrame.IsValid()) {
		std::cerr << "MovieEncoder::_EncoderThread(): invalid destination frame. Getting it from first frame...";
		std::flush(std::cerr);
		BBitmap* bitmap = fFileList->ItemAt(0)->Bitmap();
		if (bitmap == NULL) {
			std::cerr << "FAILED" << std::endl;
			status = B_ERROR;
			_HandleEncodingFinished(status);
			return status;
		}
		std::cerr << "OK" << std::endl;
		BRect sourceFrame = bitmap->Bounds();
		delete bitmap;
		fDestFrame = sourceFrame.OffsetToCopy(B_ORIGIN);
	}

	// TODO: Improve this: we are using the name of the media format to see if it's a fake format
	if ((strcmp(MediaFileFormat().short_name, NULL_FORMAT_SHORT_NAME) == 0) ||
		(strcmp(MediaFileFormat().short_name, GIF_FORMAT_SHORT_NAME) == 0)) {
		return _WriteRawFrames();
	}

	media_format mediaFormat = fFormat;
	const BitmapEntry* firstEntry = fFileList->ItemAt(0);
	const BitmapEntry* lastEntry = fFileList->ItemAt(framesLeft - 1);
	ASSERT((firstEntry != NULL));
	ASSERT((lastEntry != NULL));
	const bigtime_t diff = lastEntry->TimeStamp() - firstEntry->TimeStamp();
	float fps = CalculateFPS(framesLeft, diff);
	mediaFormat.u.raw_video.field_rate = fps;

	// Create movie
	status = _CreateFile(fOutputFile.Path(), fFileFormat, mediaFormat, fCodecInfo);
	fTempPath = fFileList->Path();
	if (status != B_OK) {
		std::cerr << "MovieEncoder::_EncoderThread(): _CreateFile failed with " << ::strerror(status) << std::endl;
		_HandleEncodingFinished(status);
		return status;
	}

	const uint32 keyFrameFrequency = 10;
	// TODO: Make this tunable

	BMessage initialMessage(kEncodingProgress);
	initialMessage.AddBool("reset", true);
	initialMessage.AddInt32("frames_total", framesLeft);
	initialMessage.AddString("text", "Encoding...");
	fMessenger.SendMessage(&initialMessage);

	int32 framesWritten = 0;
	while (!fKillThread && framesLeft > 0) {
		BitmapEntry* entry = const_cast<FramesList*>(fFileList)->Pop();
		if (entry == NULL) {
			status = B_ERROR;
			break;
		}

		BBitmap* frame = entry->Bitmap();
		delete entry;

		if (frame == NULL) {
			// TODO: What to do here ? Exit with an error ?
			status = B_ERROR;
			std::cerr << "Error while loading bitmap entry" << std::endl;
			break;
		}

		bool keyFrame = (framesWritten % keyFrameFrequency == 0);
		if (status == B_OK)
			status = _WriteFrame(frame, framesWritten + 1, keyFrame);
		delete frame;

		if (status != B_OK)
			break;

		framesWritten++;
		framesLeft--;

		if (!fMessenger.IsValid()) {
			// BMessenger is no longer valid. This means that the application
			// has been closed or it has crashed.
			break;
		}
		BMessage progressMessage(kEncodingProgress);
		progressMessage.AddInt32("frames_remaining", fFileList->CountItems());
		fMessenger.SendMessage(&progressMessage);
	}

	if (status == B_OK)
		status = _PostEncodingAction(fTempPath, fps);

	if (status != B_OK) {
		// Something went wrong during encoding
		// TODO: at least save the frames somewhere ?
		std::cerr << "Something went very wrong during encoding." << std::endl;
		std::cerr << framesWritten << " frames were sent to the mediakit." << std::endl;
		std::cerr << "The system returned: " << strerror(status) << std::endl;
	}
	_HandleEncodingFinished(status, framesWritten);

	return status;
}


status_t
MovieEncoder::_ApplyImageFilters()
{
	// TODO: update progress
	if (Settings::Current().Scale() != 100) {
		const int32 framesTotal = fFileList->CountItems();

		BMessage initialMessage(kEncodingProgress);
		initialMessage.AddBool("reset", true);
		initialMessage.AddInt32("frames_total", framesTotal);
		initialMessage.AddString("text", "Scaling...");
		fMessenger.SendMessage(&initialMessage);

		// First pass: scale frames if needed
		// TODO: we could apply different filters
		ImageFilterScale* filter = new ImageFilterScale(fDestFrame, fColorSpace);
		for (int32 c = 0; c < framesTotal; c++) {
			if (fKillThread) {
				delete filter;
				return B_ERROR;
			}
			BitmapEntry* entry = fFileList->ItemAt(c);
			BBitmap* filtered = filter->ApplyFilter(entry->Bitmap());
			entry->Replace(filtered);

			BMessage progressMessage(kEncodingProgress);
			progressMessage.AddInt32("frames_remaining", framesTotal - c);
			fMessenger.SendMessage(&progressMessage);
		}
		delete filter;
	}

	return B_OK;
}


status_t
MovieEncoder::_WriteRawFrames()
{
	// TODO: Let the user select the output directory
	BPath path;
	status_t status = find_directory(B_USER_DIRECTORY, &path);
	if (status != B_OK)
		return status;

	// TODO: Code duplication between here and _EncoderThread
	const int32 frames = fFileList->CountItems();
	const BitmapEntry* firstEntry = fFileList->ItemAt(0);
	const BitmapEntry* lastEntry = fFileList->ItemAt(frames - 1);
	const bigtime_t diff = lastEntry->TimeStamp() - firstEntry->TimeStamp();
	float fps = CalculateFPS(fFileList->CountItems(), diff);

	BMessage progressMessage(kEncodingProgress);
	progressMessage.AddBool("reset", true);
	progressMessage.AddString("text", "Exporting...");
	progressMessage.AddInt32("frames_total", frames);
	fMessenger.SendMessage(&progressMessage);

	char tempDirectoryTemplate[PATH_MAX];
	snprintf(tempDirectoryTemplate, PATH_MAX, "%s/BeScreenCapture_XXXXXX", path.Path());
	char* tempDirectoryName = ::mkdtemp(tempDirectoryTemplate);
	if (tempDirectoryName == NULL)
		status = B_ERROR;
	else if (BEntry(tempDirectoryName).IsDirectory()) {
		fTempPath = tempDirectoryName;
		status = fFileList->WriteFrames(tempDirectoryName);
	}

	if (status == B_OK)
		status = _PostEncodingAction(fTempPath, fps);

	_HandleEncodingFinished(status, status == B_OK ? frames : 0);

	return status;
}


thread_id
MovieEncoder::EncodeThreaded()
{
	fKillThread = false;
	
	fEncoderThread = spawn_thread((thread_entry)EncodeStarter,
		"Encoder Thread", B_DISPLAY_PRIORITY, this);
					
	if (fEncoderThread < 0)
		return fEncoderThread;
	
	status_t status = resume_thread(fEncoderThread);
	if (status < B_OK) {
		kill_thread(fEncoderThread);
		return status;
	}
	
	return fEncoderThread;
}


void
MovieEncoder::ResetConfiguration()
{
	fColorSpace = B_NO_COLOR_SPACE;
}


media_format_family
MovieEncoder::MediaFormatFamily() const
{
	return fFamily;
}


void
MovieEncoder::SetMediaFormatFamily(const media_format_family& family)
{
	fFamily = family;
}


media_file_format
MovieEncoder::MediaFileFormat() const
{
	return fFileFormat;
}


void
MovieEncoder::SetMediaFileFormat(const media_file_format& fileFormat)
{
	fFileFormat = fileFormat;
}


media_format
MovieEncoder::MediaFormat() const
{
	return fFormat;
}


void
MovieEncoder::SetMediaFormat(const media_format& format)
{
	fFormat = format;
}


void
MovieEncoder::SetMediaCodecInfo(const media_codec_info& info)
{
	fCodecInfo = info;
}


media_codec_info
MovieEncoder::MediaCodecInfo() const
{
	return fCodecInfo;
}


// private methods
BBitmap*
MovieEncoder::GetCursorBitmap(const uint8* cursor)
{
	uint8 size = *cursor;
	
	BBitmap* cursorBitmap = new BBitmap(BRect(0, 0, size - 1, size - 1), B_RGBA32);
	
	uint32 black = 0xFF000000;
	uint32 white = 0xFFFFFFFF;
	
	uint8* castCursor = const_cast<uint8*>(cursor);
	uint16* cursorPtr = reinterpret_cast<uint16*>(castCursor + 4);
	uint16* maskPtr = reinterpret_cast<uint16*>(castCursor + 36);
	uint8* buffer = static_cast<uint8*>(cursorBitmap->Bits());
	uint16 cursorFlip, maskFlip;
	uint16 cursorVal, maskVal;
	for (uint8 row = 0; row < size; row++) {
		uint32* bits = (uint32*)(buffer + (row * cursorBitmap->BytesPerRow()));
		cursorFlip = (cursorPtr[row] & 0xFF) << 8;
		cursorFlip |= (cursorPtr[row] & 0xFF00) >> 8;
		
		maskFlip = (maskPtr[row] & 0xFF) << 8;
		maskFlip |= (maskPtr[row] & 0xFF00) >> 8;
		
		for (uint8 column = 0; column < size; column++) {
			uint16 posVal = 1 << (15 - column);
			cursorVal = cursorFlip & posVal;
			maskVal = maskFlip & posVal;
			bits[column] = (cursorVal != 0 ? black : white) &
							(maskVal > 0 ? white : 0x00FFFFFF);
		}
	}

	return cursorBitmap;
}


status_t
MovieEncoder::PopCursorPosition(BPoint& point)
{
	ASSERT(fCursorQueue != NULL);
	//point = fCursorQueue->front();
	//fCursorQueue->pop();
	
	return B_OK;
}


int32
MovieEncoder::EncodeStarter(void* arg)
{
	return static_cast<MovieEncoder*>(arg)->_EncoderThread();
}


status_t
MovieEncoder::_PostEncodingAction(BPath& path, uint32 fps)
{
	// For now we need PostEncoding only for GIF export
	if (strcmp(MediaFileFormat().short_name, GIF_FORMAT_SHORT_NAME) != 0)
		return B_OK;

	if (!IsFFMPEGAvailable()) {
		std::cerr << "ffmpeg_tools is not available" << std::endl;
		return B_ERROR;
	}

	BMessage progressMessage(kEncodingProgress);
	progressMessage.AddBool("reset", true);
	progressMessage.AddString("text", "Processing...");
	fMessenger.SendMessage(&progressMessage);

	BString command;
	command.Append("ffmpeg "); // command
	command.Append("-i ").Append(path.Path()).Append("/frame_%07d.png"); // input
	command.Append(" -f gif "); // output type
	// filter
	command.Append("-vf \"");
	// add FPS
	command.Append("fps=");
	command << fps;
	command.Append(",");
	// optimize conversion by generating a palette
	command.Append("split[s0][s1];[s0]palettegen=stats_mode=diff[p];[s1][p]paletteuse=new=1:diff_mode=rectangle ");
	command.Append("\"");
	command.Append(" ");
	command.Append(fOutputFile.Path()); // output
	command.Append (" > /dev/null 2>&1");
#if 1
	std::cout << "PostEncodingAction command: " << command.String() << std::endl;
#endif
	int result = system(command.String());

	BEntry pathEntry(fTempPath.Path());
	if (pathEntry.Exists()) {
		BDirectory dir(fTempPath.Path());
		BEntry fileEntry;
		while (dir.GetNextEntry(&fileEntry) == B_OK)
			fileEntry.Remove();
		pathEntry.Remove();
	}

	if (WEXITSTATUS(result) != 0)
		return B_ERROR;

	return B_OK;
}


void
MovieEncoder::_HandleEncodingFinished(const status_t& status, const int32& numFrames)
{
	DisposeData();

	if (!fMessenger.IsValid())
		return;

	BMessage message(kEncodingFinished);
	message.AddInt32("status", (int32)status);
	if (numFrames > 0) {
		message.AddInt32("frames", (int32)numFrames);
		if (strcmp(MediaFileFormat().short_name, NULL_FORMAT_SHORT_NAME) == 0)
			message.AddString("file_name", fTempPath.Path());
		else
			message.AddString("file_name", fOutputFile.Path());
	}
	fMessenger.SendMessage(&message);
}
