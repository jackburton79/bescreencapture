#ifndef __CONTROLLEROBSERVER_H
#define __CONTROLLEROBSERVER_H

#include "Controller.h"


enum {
	kMsgControllerCaptureStarted = 5000,
	kMsgControllerCaptureStopped,
	kMsgControllerCapturePaused,
	kMsgControllerCaptureResumed,
	kMsgControllerCaptureFailed,		// status_t status
	
	kMsgControllerEncodeStarted,		
	kMsgControllerEncodeProgress,		// int32 num_files
	kMsgControllerEncodeFinished,		// status_t status
	
	
	kMsgControllerSelectionWindowClosed,
	kMsgControllerSourceFrameChanged,	// BRect frame
	kMsgControllerTargetFrameChanged,	// BRect frame
	kMsgControllerVideoDepthChanged,
	kMsgControllerOutputFileNameChanged,
	kMsgControllerCodecListUpdated,
	kMsgControllerCodecChanged			// const char* codec_name
};


#endif // __CONTROLLEROBSERVER_H
