/*
 * Copyright 2016-2021 Stefano Ceccherini <stefano.ceccherini@gmail.com>
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
	BString string;
	string << formatName;
	return string;
}


static BString
GetCodecString(const char* codecName)
{
	BString string;
	string << codecName;
	return string;
}


static BString
GetSourceRectString(const BRect& rect)
{
	BString string;
	string << "l: " << rect.left << ", t: " << rect.top;
	string << ", r: " << rect.right << ", b: " << rect.bottom;
	return string;
}


static BString
GetTargetRectString(const BRect& rect)
{
	BString string;
	string << (int32)(rect.right - rect.left + 1) << " x ";
	string << (int32)(rect.bottom - rect.top + 1);
	return string;
}


static BString
GetScaleString(float scale)
{
	BString string;
	string << scale << "%";
	return string;
}


static BString
GetFrameRateString(int32 fps)
{
	BString string;
	string << fps << " frames per second";
	return string;
}


InfoView::InfoView(Controller* controller)
	:
	BView("Info", B_WILL_DRAW),
	fController(controller)
{
	const Settings& settings = Settings::Current();
	BRect sourceArea = settings.CaptureArea();
	BRect targetRect = settings.TargetRect();
	float scale = settings.Scale();
	
	BLayoutBuilder::Grid<>(this, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(new BStringView("source_size", "Source region: "), 0, 0)
		.Add(fSourceSize = new BStringView("source_size_value", GetSourceRectString(sourceArea)), 1, 0)
		.Add(new BStringView("clip_size", "Clip frame size: "), 0, 1)
		.Add(fClipSize = new BStringView("clip_size_value", GetTargetRectString(targetRect)), 1, 1)
		.Add(new BStringView("scale", "Clip scaling factor: "), 0, 2)
		.Add(fScale = new BStringView("scale_value", GetScaleString(scale)), 1, 2)
		.Add(new BStringView("format", "Format: "), 0, 3)
		.Add(fFormat = new BStringView("format_value", ""), 1, 3)
		.Add(new BStringView("codec", "Codec: "), 0, 4)
		.Add(fCodec = new BStringView("codec value", ""), 1, 4)
		.Add(new BStringView("frame_rate", "Capture frame rate: "), 0, 5)
		.Add(fCaptureFrameRate = new BStringView("frame_rate value", GetFrameRateString(settings.CaptureFrameRate())), 1, 5);
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
		fController->StartWatching(this, kMsgControllerCaptureFrameRateChanged);
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
				case kMsgControllerCaptureFrameRateChanged:
				{
					int32 fps;
					if (message->FindInt32("frame_rate", &fps) == B_OK) {						
						fCaptureFrameRate->SetText(GetFrameRateString(fps));
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
