#ifndef __MESSAGES_H
#define __MESSAGES_H


enum appMessages {
	kMsgGUIStartCapture = 2000,
	kMsgGUIStopCapture,
	kAddonStarted,
	kSelectArea,
	kAreaSelected,
	kCaptureFinished,
	kEncodingFinished,
	kRebuildCodec,
	kFileNameChanged,
	kMinimizeOnRecording,
	kClipSizeChanged,
	kPauseResumeCapture,
	kMsgGetControllerMessenger
};

#endif 
