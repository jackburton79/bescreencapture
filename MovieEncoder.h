/*
 * Copyright 2015-2023 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __MOVIE_ENCODER_H
#define __MOVIE_ENCODER_H

#include <MediaDefs.h>
#include <MediaFile.h>
#include <Path.h>

#include <queue>

class BBitmap;
class FramesList;
class MovieEncoder {
public:
	MovieEncoder();
	~MovieEncoder();

	void DisposeData();

	void Cancel();

	status_t SetSource(FramesList* fileList);
	status_t SetCursorQueue(std::queue<BPoint> *queue);
	status_t SetOutputFile(const char *fileName);
	BPath OutputFile() const;

	status_t SetDestFrame(const BRect &rect);
	void SetColorSpace(const color_space &space);
	status_t SetQuality(const float &quality);
	status_t SetThreadPriority(const int32 &value);
	status_t SetMessenger(const BMessenger &messenger);

	media_file_format	MediaFileFormat() const;
	media_format_family MediaFormatFamily() const;
	media_format		MediaFormat() const;
	media_codec_info	MediaCodecInfo() const;

	void SetMediaFileFormat(const media_file_format&);
	void SetMediaFormatFamily(const media_format_family &);
	void SetMediaFormat(const media_format &);
	void SetMediaCodecInfo(const media_codec_info &);

	thread_id EncodeThreaded();

private:
	void ResetConfiguration();

	BBitmap *GetCursorBitmap(const uint8 *data);
	status_t PopCursorPosition(BPoint &point);

	status_t _CreateFile(const char* path,
						const media_file_format& mff,
						const media_format& inputFormat,
						const media_codec_info& mci,
						float quality = -1);
	status_t _WriteFrame(const BBitmap* bitmap, int32 frameNum, bool isKeyFrame);
	status_t _CloseFile();

	static int32 EncodeStarter(void *arg);
	status_t _EncoderThread();

	status_t _ApplyImageFilters();
	status_t _WriteRawFrames();

	void _HandleEncodingFinished(const status_t& status,
								const int32& numFrames = 0);
	status_t _PostEncodingAction(const BPath& path, int32 numFrames, int32 fps);

	thread_id	fEncoderThread;
	bool		fKillThread;

	int32 fPriority;
	BMessenger fMessenger;

	FramesList* fFileList;

	std::queue<BPoint> *fCursorQueue;

	BPath fOutputFile;
	BPath fTempPath;

	BRect fDestFrame;
	color_space fColorSpace;
	BMediaFile*			fMediaFile;
	BMediaTrack*		fMediaTrack;
	bool				fHeaderCommitted;

	media_file_format	fFileFormat;
	media_format_family	fFamily;
	media_format		fFormat;
	media_codec_info	fCodecInfo;
};


#endif
