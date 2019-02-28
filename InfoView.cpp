/*
 * Copyright 2016 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "InfoView.h"

#include "Controller.h"
#include "ControllerObserver.h"
#include "Settings.h"

#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <String.h>
#include <StringView.h>

#include <iostream>

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


static BString
GetSourceRectString(const BRect& rect)
{
	BString string = "Source region: ";
	string << "l: " << rect.left << ", t: " << rect.top;
	string << ", r: " << rect.right << ", b: " << rect.bottom;
	return string;
}


static BString
GetTargetRectString(const BRect& rect)
{
	BString string = "Clip frame size: ";
	string << (int32)(rect.right - rect.left + 1) << " x ";
	string << (int32)(rect.bottom - rect.top + 1);
	return string;
}


static BString
GetScaleString(float scale)
{
	BString string = "Clip scaling factor: ";
	string << scale << "%";
	return string;
}


static BString
GetDelayString(int32 delay)
{
	BString string = "Capture frame delay: ";
	string << delay << " milliseconds";
	return string;
}


InfoView::InfoView(Controller* controller)
	:
	BView("Info", B_WILL_DRAW),
	fController(controller)
{
	Settings& settings = Settings::Current();
	BRect sourceArea = settings.CaptureArea();
	BRect targetRect = settings.TargetRect();
	float scale = settings.Scale();
	
	SetLayout(new BGroupLayout(B_VERTICAL));
	BView* layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fSourceSize = new BStringView("source size",
				GetSourceRectString(sourceArea)))
			.Add(fClipSize = new BStringView("clip size",
				GetTargetRectString(targetRect)))
			.Add(fScale = new BStringView("clip scale",
				GetScaleString(scale)))
			.Add(fFormat = new BStringView("format", ""))
			.Add(fCodec = new BStringView("codec", ""))
			.Add(fCaptureFrameDelay = new BStringView("capturedelay",
				GetDelayString(settings.CaptureFrameDelay())))
		.End()
		.View();
	AddChild(layoutView);
}


void
InfoView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	if (fController->LockLooper()) {
		fController->StartWatching(this, kMsgControllerSourceFrameChanged);
		fController->StartWatching(this, kMsgControllerTargetFrameChanged);
		fController->StartWatching(this, kMsgControllerCodecChanged);
		fController->StartWatching(this, kMsgControllerMediaFileFormatChanged);
		fController->StartWatching(this, kMsgControllerCaptureFrameDelayChanged);
		fController->UnlockLooper();
	}
	
	fFormat->SetText(GetFormatString(fController->MediaFileFormatName()));
	fCodec->SetText(GetCodecString(fController->MediaCodecName()));
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
					BRect rect;
					if (message->FindRect("frame", &rect) == B_OK) {
						BString string = GetSourceRectString(rect);
						fSourceSize->SetText(string.String());
					}
					break;
				}
				case kMsgControllerTargetFrameChanged:
				{
					BRect rect;
					if (message->FindRect("frame", &rect) == B_OK) {
						BString string = GetTargetRectString(rect);
						fClipSize->SetText(string.String());
					}
					float scale;
					if (message->FindFloat("scale", &scale) == B_OK) {
						BString string = GetScaleString(scale);
						fScale->SetText(string.String());
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
				case kMsgControllerCaptureFrameDelayChanged:
				{
					int32 delay;
					if (message->FindInt32("delay", &delay) == B_OK) {						
						fCaptureFrameDelay->SetText(GetDelayString(delay));
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
