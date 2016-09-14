#ifndef __MESSAGES_H
#define __MESSAGES_H


enum appMessages {
	kMsgGUIStartCapture = 2000,
	kMsgGUIStopCapture,
	kAddonStarted,
	kSelectArea,
	kSelectWindow,
	kSelectionWindowClosed,
	kCaptureFinished,
	kEncodingFinished,
	kFileNameChanged,
	kMinimizeOnRecording,
	kPauseResumeCapture,
	kMsgGetControllerMessenger
};

#endif 
