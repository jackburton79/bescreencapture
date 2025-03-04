/*
 * Copyright 2018-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef __CONSTANTS_H
#define __CONSTANTS_H

#include <GraphicsDefs.h>

#define NULL_FORMAT_SHORT_NAME "no_encoding"
#define NULL_FORMAT_PRETTY_NAME "Export frames as Bitmaps"
#define GIF_FORMAT_SHORT_NAME "gif"
#define GIF_FORMAT_PRETTY_NAME "GIF"
#define BSC_DESKBAR_VIEW "BSC Control"

extern const rgb_color kRed;
extern const rgb_color kGreen;
extern const rgb_color kBlack;

enum appMessages {
	kAddonStarted = 2000,
	kSelectArea,
	kSelectWindow,
	kSelectionWindowClosed,
	kCaptureFinished,
	kEncodingFinished,		// int32 "status",
							// const char* "file_name",
							// int32 "frames_processed"
	kEncodingProgress,
	kFileNameChanged
};


#endif // __CONSTANTS_H
