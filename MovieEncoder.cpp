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
	const media_file_format& mff,
	const media_format& inputFormat,
	const media_codec_info& mci,
	float quality)
{
	entry_ref ref;
	get_ref_for_path(path, &ref);
	BMediaFile* file = new BMediaFile(&ref, &mff);
	status_t err = file->InitCheck();
	if (err == B_OK) {
		fHeaderCommitted = false;
		fMediaFile = file;

		// This next line casts away const to avoid a warning.  MediaFile::CreateTrack()
		// *should* have the input format argument declared const, but it doesn't, and
		// it can't be changed because it would break binary compatibility.  Oh, well.
		fMediaTrack = file->CreateTrack(const_cast<media_format *>(&inputFormat), &mci);
		if (!fMediaTrack)
			err = B_ERROR;
		else {
			if (quality >= 0)
				fMediaTrack->SetQuality(quality);
		}
	}

	// clean up if we incurred an error
	if (err < B_OK) {
		fHeaderCommitted = false;
		delete fMediaFile;
		fMediaFile = NULL;
	}
	
	return err;
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
	int32 framesLeft = fFileList->CountItems();
	if (framesLeft <= 1) {
		DisposeData();
		_HandleEncodingFinished(B_ERROR);
		return B_ERROR;
	}
		
	BBitmap* bitmap = fFileList->ItemAt(0)->Bitmap();
	BRect sourceFrame = bitmap->Bounds();
	delete bitmap;

	if (!fDestFrame.IsValid())
		fDestFrame = sourceFrame.OffsetToCopy(B_ORIGIN);

	// First pass: apply filter to frames
	// TODO: update progress
	ImageFilterScale* filter = new ImageFilterScale(fDestFrame, fColorSpace);	
	for (int32 c = 0; c < framesLeft; c++) {
		BitmapEntry* entry = fFileList->ItemAt(c);
		BBitmap* filtered = filter->ApplyFilter(entry->Bitmap());
		entry->Replace(filtered);
	}
	delete filter;

	int32 framesWritten = 0;

	status_t status = B_ERROR;
	// TODO: Improve this: we are using the name of the media format to see if it's a fake format
	if ((strcmp(MediaFileFormat().short_name, NULL_FORMAT_SHORT_NAME) == 0) ||
		(strcmp(MediaFileFormat().short_name, GIF_FORMAT_SHORT_NAME) == 0)) {
		// TODO: Let the user select the output directory
		BPath path;
		status = find_directory(B_USER_DIRECTORY, &path);
		if (status == B_OK) {
			char tempDirectoryTemplate[PATH_MAX];
			snprintf(tempDirectoryTemplate, PATH_MAX, "%s/BeScreenCapture_XXXXXX", path.Path());
			char* tempDirectoryName = ::mkdtemp(tempDirectoryTemplate);
			if (tempDirectoryName == NULL)
				status = B_ERROR;
			else if (BEntry(tempDirectoryName).IsDirectory()) {
				fTempPath = tempDirectoryName;
				fFileList->WriteFrames(tempDirectoryName);
				framesWritten = framesLeft;
				framesLeft = 0;
			}
		}
	} else {
		BitmapEntry* firstEntry = fFileList->ItemAt(0);
		BitmapEntry* lastEntry = fFileList->ItemAt(framesLeft - 1);
		int framesPerSecond = (1000000 * framesLeft) / (lastEntry->TimeStamp() - firstEntry->TimeStamp());
		media_format inputFormat = fFormat;
		inputFormat.u.raw_video.field_rate = framesPerSecond;

		// Create movie
		status = _CreateFile(fOutputFile.Path(), fFileFormat, inputFormat, fCodecInfo);
		fTempPath = fFileList->Path();
	}

	if (status != B_OK) {
		DisposeData();
		_HandleEncodingFinished(status);
		return status;
	}

	const uint32 keyFrameFrequency = 10;
	// TODO: Make this tunable

	while (!fKillThread && framesLeft > 0) {
		BitmapEntry* entry = const_cast<FramesList*>(fFileList)->Pop();
		if (entry == NULL)
			break;

		BBitmap* frame = entry->Bitmap();
		if (frame == NULL) {
			// TODO: What to do here ? Exit with an error ?
			std::cerr << "Error while loading bitmap entry" << std::endl;
			delete entry;
			continue;
		}
		
		delete entry;

		bool keyFrame = (framesWritten % keyFrameFrequency == 0);
		if (status == B_OK)
			status = _WriteFrame(frame, framesWritten + 1, keyFrame);

		delete frame;

		if (status != B_OK)
			break;

		framesWritten++;
		framesLeft--;

		if (fMessenger.IsValid()) {
			BMessage progressMessage(kEncodingProgress);
			progressMessage.AddInt32("frames_remaining", fFileList->CountItems());
			fMessenger.SendMessage(&progressMessage);
		} else {
			// BMessenger is no longer valid. This means that the application
			// has been closed or it has crashed.
			break;
		}
	}

	if (strcmp(MediaFileFormat().short_name, GIF_FORMAT_SHORT_NAME) == 0)
		status = _PostEncodingAction(fTempPath);

	DisposeData();

	_HandleEncodingFinished(status, framesWritten);

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
MovieEncoder::_PostEncodingAction(BPath& path)
{
	if (!IsFFMPEGAvailable())
		return B_ERROR;

	BString command;
	command.Append("ffmpeg -i ");
	command.Append(path.Path()).Append("/frame_%05d.bmp ");
	command.Append(fOutputFile.Path());
	command.Append (" > /dev/null 2>&1");
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
