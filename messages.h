#ifndef __MESSAGES_H
#define __MESSAGES_H


enum appMessages {
	kMsgGUIToggleCapture = 2000,
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
