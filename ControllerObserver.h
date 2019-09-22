#ifndef __CONTROLLEROBSERVER_H
#define __CONTROLLEROBSERVER_H


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
											// const char* file_name
	
	kMsgControllerSelectionWindowClosed,
	kMsgControllerSourceFrameChanged,		// BRect frame,
	kMsgControllerTargetFrameChanged,		// BRect frame, float scale
	kMsgControllerVideoDepthChanged,
	kMsgControllerOutputFileNameChanged,
	kMsgControllerCodecListUpdated,
	kMsgControllerCodecChanged,				// const char* codec_name
	kMsgControllerMediaFileFormatChanged,	// const char* format_name
	
	kMsgControllerCaptureFrameRateChanged, // int32 frame_rate
	
	kMsgControllerResetSettings
};


#endif // __CONTROLLEROBSERVER_H
