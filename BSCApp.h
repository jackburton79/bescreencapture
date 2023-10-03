/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __BSCAPP_H
#define __BSCAPP_H

#include <Application.h>

#include <DirectWindow.h>
#include <List.h>
#include <MediaDefs.h>
#include <MediaFile.h>
#include <ObjectList.h>
#include <OS.h>

#define kAppSignature "application/x-vnd.BeScreenCapture"

class BBitmap;
class BMessageRunner;
class BStopWatch;
class BString;
class FramesList;
class MovieEncoder;
class Arguments;
class BSCApp : public BApplication {
public:
	BSCApp();
	virtual ~BSCApp();
	
	virtual void ArgvReceived(int32 argc, char** argv);
	virtual void ReadyToRun();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *message);
	virtual status_t GetSupportedSuites(BMessage* message);
	BHandler* ResolveSpecifier(BMessage* message,
								int32 index,
								BMessage* specifier,
								int32 what,
								const char* property);
	virtual void AboutRequested();
	void ShowHelp();

	enum state {
		STATE_IDLE = 0,
		STATE_RECORDING,
		STATE_ENCODING
	};

	bool		CanQuit(BString& reason) const;
	void		StopThreads();
	
	int			State() const;
		
	void		ToggleCapture();
	void		TogglePause();
	
	int32		RecordedFrames() const;
	bigtime_t	RecordTime() const;
	void		SetRecordingTime(const bigtime_t msecs);
	
	float		AverageFPS() const;
	
	void		EncodeMovie();
	
	void		SetUseDirectWindow(const bool &use);
	bool		SetCaptureArea(const BRect &rect);
	void		SetCaptureFrameRate(const int fps);
	void		SetPlaybackFrameRate(const int rate);
	
	void		SetScale(const float &scale);
	void		SetVideoDepth(const color_space &space);
	void		SetOutputFileName(const char *fileName);

	media_format_family MediaFormatFamily() const;
	void		SetMediaFormatFamily(const media_format_family &family);
	
	media_file_format	MediaFileFormat() const;
	void		SetMediaFileFormat(const media_file_format& fileFormat);
	
	BString		MediaFileFormatName() const;
	BString		MediaCodecName() const;
	void		SetMediaCodec(const char* codecName);

	status_t	GetCodecsList(BObjectList<media_codec_info>& codecList) const;
	status_t	UpdateMediaFormatAndCodecsForCurrentFamily();
	
	void		UpdateDirectInfo(direct_buffer_info *info);

	status_t	ReadBitmap(BBitmap *bitmap, bool includeCursor, BRect bounds);
	
	void		ResetSettings();
	
	void		TestSystem();


	void InstallDeskbarReplicant();
	void RemoveDeskbarReplicant();

	bool WasLaunchedSilently() const;
	bool LaunchedFromCommandline() const;
	
private:
	BWindow *fWindow;
	Arguments* fArgs;
	bool fShouldStartRecording;

	thread_id			fCaptureThread;
	int32				fNumFrames;
	BStopWatch*			fRecordWatch;
	bool				fKillCaptureThread;
	bool				fPaused;

	bool				fDirectWindowAvailable;	
	direct_buffer_info	fDirectInfo;
	MovieEncoder*		fEncoder;
	thread_id			fEncoderThread;
	
	FramesList			*fFileList;
	
	BObjectList<media_codec_info>* fCodecList;

	BMessageRunner*		fStopRunner;
	bigtime_t			fRequestedRecordTime;

	bool		fSupportsWaitForRetrace;

	void _UsageRequested();
	bool _HandleScripting(BMessage* message);
	
	void		StartCapture();
	void		EndCapture();

	void		_PauseCapture();
	void		_ResumeCapture();

	void		_EncodingFinished(const status_t status, const char* fileName);
	void		_HandleTargetFrameChanged(const BRect& targetRect);
	void		_ForwardGUIMessage(BMessage *message);

	media_format	_ComputeMediaFormat(const int32 &width, const int32 &height,
							const color_space &colorSpace, const float &fieldRate);

	void		_TestWaitForRetrace();
	void		_WaitForRetrace(bigtime_t time);
	void		_UpdateFromSettings();
	void		_DumpSettings() const;
	
	status_t CaptureThread();
	static int32 CaptureStarter(void *arg);	
};

#endif // __BSCAPP_H
