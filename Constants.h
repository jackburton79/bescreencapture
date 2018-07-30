#ifndef __MESSAGES_H
#define __MESSAGES_H

#include "PublicMessages.h"

#define FAKE_FORMAT_SHORT_NAME "no_encoding"

enum appMessages {
	kAddonStarted = 2000,
	kSelectArea,
	kSelectWindow,
	kSelectionWindowClosed,
	kCaptureFinished,
	kEncodingFinished,
	kEncodingProgress,
	kFileNameChanged,
	kMsgGetControllerMessenger
};


#endif 
