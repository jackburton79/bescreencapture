/*
 * Copyright 2018-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef __CONTROLLEROBSERVER_H
#define __CONTROLLEROBSERVER_H

enum {
	kMsgControllerCaptureStarted = 5000,
	kMsgControllerCaptureStopped,			// status_t "status"
	kMsgControllerCapturePaused,
	kMsgControllerCaptureResumed,
	kMsgControllerCaptureProgress,			// int32 "frames_total"

	kMsgControllerEncodeStarted,			// int32 "frames_total"

	kMsgControllerEncodeProgress,			// int32 "frames_remaining"
											// const char* "text"
											// bool "reset"
											// int32 "frames_total"

	kMsgControllerEncodeFinished,			// status_t "status"
											// const char* "file_name"

	kMsgControllerSelectionWindowClosed,
	kMsgControllerSourceFrameChanged,		// BRect "frame",
	kMsgControllerTargetFrameChanged,		// BRect "frame", float "scale"
	kMsgControllerVideoDepthChanged,
	kMsgControllerOutputFileNameChanged,
	kMsgControllerCodecListUpdated,
	kMsgControllerCodecChanged,				// const char* "codec_name"
	kMsgControllerMediaFileFormatChanged,	// const char* "format_name"

	kMsgControllerCaptureFrameRateChanged,	// int32 "frame_rate"

	kMsgControllerResetSettings
};


#endif // __CONTROLLEROBSERVER_H
