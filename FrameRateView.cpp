/*
 * Copyright 2017-2024 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "FrameRateView.h"

#include "BSCApp.h"
#include "ControllerObserver.h"
#include "Settings.h"
#include "SliderTextControl.h"

#include <Box.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <Debug.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <TextControl.h>
#include <StringView.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "FrameRateView"

const static char* kFrameRateLabel = B_TRANSLATE("Target frame rate");

const static uint32 kLocalFrameRateChanged = 'FrCh';


FrameRateView::FrameRateView()
	:
	BView("frame_rate_view", B_WILL_DRAW)
{
	fFrameRateSlider = new SliderTextControl("frame_rate_slider",
			kFrameRateLabel, new BMessage(kLocalFrameRateChanged), 1, 60, 1, B_TRANSLATE("fps"));

	BLayoutBuilder::Group<>(this, B_VERTICAL, B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL)
			.Add(fFrameRateSlider)
		.End();
}


void
FrameRateView::AttachedToWindow()
{
	if (be_app->LockLooper()) {
		be_app->StartWatching(this, kMsgControllerCaptureStarted);
		be_app->StartWatching(this, kMsgControllerCaptureStopped);
		be_app->StartWatching(this, kMsgControllerResetSettings);
		be_app->UnlockLooper();
	}
	fFrameRateSlider->SetValue(Settings::Current().CaptureFrameRate());
	fFrameRateSlider->SetTarget(this);
}


void
FrameRateView::MessageReceived(BMessage* message)
{
	BSCApp* app = dynamic_cast<BSCApp*>(be_app);
	ASSERT(app != NULL);
	switch (message->what) {
		case kLocalFrameRateChanged:
		{
			int32 rate = fFrameRateSlider->Value();
			app->SetCaptureFrameRate(rate);
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			switch (code) {
				case kMsgControllerCaptureStarted:
					fFrameRateSlider->SetEnabled(false);
					break;
				case kMsgControllerCaptureStopped:
					fFrameRateSlider->SetEnabled(true);
					break;
				case kMsgControllerCaptureFrameRateChanged:
				{
					int32 fps;
					if (message->FindInt32("frame_rate", &fps) == B_OK)
						fFrameRateSlider->SetValue(fps);
					break;
				}
				case kMsgControllerResetSettings:
				{
					int32 fps = Settings::Current().CaptureFrameRate();
					fFrameRateSlider->SetValue(fps);
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
