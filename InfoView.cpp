/*
 * Copyright 2016 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ControllerObserver.h"
#include "InfoView.h"


#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <StringView.h>

static BString
GetFormatString(const char* formatName)
{
	BString string = "Format: ";
	string << formatName;
	return string;
}


static BString
GetCodecString(const char* codecName)
{
	BString string = "Codec: ";
	string << codecName;
	return string;
}



InfoView::InfoView(Controller* controller)
	:
	BView("Info", B_WILL_DRAW),
	fController(controller)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	BView* layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fSourceSize = new BStringView("source size", ""))
			.Add(fClipSize = new BStringView("clip size", ""))
			.Add(fFormat = new BStringView("format",
				GetFormatString(fController->MediaFileFormatName())))
			.Add(fCodec = new BStringView("codec",
				GetCodecString(fController->MediaCodecName())))
		.End()
		.View();
	AddChild(layoutView);
}


void
InfoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32("be:observe_change_what", &code);
			switch (code) {
				case kMsgControllerSourceFrameChanged:
				{
					BString string = "Source region: ";
					BRect rect;
					if (message->FindRect("frame", &rect) == B_OK) {
						string << RectToString(rect);
						fSourceSize->SetText(string.String());
					}
					break;
				}
				case kMsgControllerTargetFrameChanged:
				{
					BString string = "Clip frame size: ";
					BRect rect;
					if (message->FindRect("frame", &rect) == B_OK) {
						string << RectToString(rect);
						fClipSize->SetText(string.String());
					}
					break;
				}
				case kMsgControllerCodecChanged:
				{
					const char* codecName = NULL;
					if (message->FindString("codec_name", &codecName) == B_OK) {
						fCodec->SetText(GetCodecString(codecName));
					}
					break;
				}
				case kMsgControllerMediaFileFormatChanged:
				{
					const char* formatName = NULL;
					if (message->FindString("format_name", &formatName) == B_OK) {						
						fFormat->SetText(GetFormatString(formatName));
					}
					break;
				}
				default:
					break;
			}
		}
		default:
			BView::MessageReceived(message);
			break;
	}
	
}


BString
InfoView::RectToString(const BRect& rect)
{
	BString string;
	string << "l: " << rect.left << ", t: " << rect.top;
	string << ", r: " << rect.right << ", b: " << rect.bottom;
	return string;
}
