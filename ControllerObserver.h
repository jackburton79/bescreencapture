#ifndef __CONTROLLEROBSERVER_H
#define __CONTROLLEROBSERVER_H

#include "Controller.h"


enum {
	kMsgControllerCaptureStarted = 5000,
	kMsgControllerCaptureStopped,			// status_t status
	kMsgControllerCapturePaused,
	kMsgControllerCaptureResumed,
	kMsgControllerCaptureProgress,			// int32 frames_total
											// int32 frames_remaining
	
	kMsgControllerEncodeStarted,		
	kMsgControllerEncodeProgress,			// int32 num_files
	kMsgControllerEncodeFinished,			// status_t status
	
	kMsgControllerSelectionWindowClosed,
	kMsgControllerSourceFrameChanged,		// BRect frame,
	kMsgControllerTargetFrameChanged,		// BRect frame, float scale
	kMsgControllerVideoDepthChanged,
	kMsgControllerOutputFileNameChanged,
	kMsgControllerCodecListUpdated,
	kMsgControllerCodecChanged,				// const char* codec_name
	kMsgControllerMediaFileFormatChanged,	// const char* format_name
	
	kMsgControllerCaptureFrameDelayChanged, // int32 delay
	
	kMsgControllerResetSettings
};


#endif // __CONTROLLEROBSERVER_H
