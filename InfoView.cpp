/*
 * Copyright 2016-2023 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "InfoView.h"

#include "BSCApp.h"
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


InfoView::InfoView()
	:
	BView("info", B_WILL_DRAW)
{
	const Settings& settings = Settings::Current();
	BRect sourceArea = settings.CaptureArea();
	BRect targetRect = settings.TargetRect();
	float scale = settings.Scale();

	BStringView* sizeView = new BStringView("source_size", "Source region:");
	sizeView->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	BStringView* frameSizeView = new BStringView("clip_size", "Clip frame size:");
	frameSizeView->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	BStringView* scaleView = new BStringView("scale", "Clip scaling factor:");
	scaleView->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	BStringView* formatView = new BStringView("format", "Format:");
	formatView->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	BStringView* codecView = new BStringView("codec", "Codec:");
	codecView->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	BStringView* rateView = new BStringView("frame_rate", "Capture frame rate:");
	rateView->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	BLayoutBuilder::Grid<>(this, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(sizeView, 0, 0)
		.Add(fSourceSize = new BStringView("source_size_value", GetSourceRectString(sourceArea)), 1, 0)
		.Add(frameSizeView, 0, 1)
		.Add(fClipSize = new BStringView("clip_size_value", GetTargetRectString(targetRect)), 1, 1)
		.Add(scaleView, 0, 2)
		.Add(fScale = new BStringView("scale_value", GetScaleString(scale)), 1, 2)
		.Add(formatView, 0, 3)
		.Add(fFormat = new BStringView("format_value", ""), 1, 3)
		.Add(codecView, 0, 4)
		.Add(fCodec = new BStringView("codec value", ""), 1, 4)
		.Add(rateView, 0, 5)
		.Add(fCaptureFrameRate = new BStringView("frame_rate value", GetFrameRateString(settings.CaptureFrameRate())), 1, 5)
		.AddGlue(0, 2, 6, 0);
}


void
InfoView::AttachedToWindow()
{
	BView::AttachedToWindow();

	if (be_app->LockLooper()) {
		be_app->StartWatching(this, kMsgControllerSourceFrameChanged);
		be_app->StartWatching(this, kMsgControllerTargetFrameChanged);
		be_app->StartWatching(this, kMsgControllerCodecChanged);
		be_app->StartWatching(this, kMsgControllerMediaFileFormatChanged);
		be_app->StartWatching(this, kMsgControllerCaptureFrameRateChanged);
		be_app->UnlockLooper();
	}

	BSCApp* app = dynamic_cast<BSCApp*>(be_app);
	fFormat->SetText(GetFormatString(app->MediaFileFormatName()));
	fCodec->SetText(GetCodecString(app->MediaCodecName()));
}


void
InfoView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
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
