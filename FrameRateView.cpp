/*
 * Copyright 2017 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "FrameRateView.h"

#include "Controller.h"
#include "ControllerObserver.h"
#include "Settings.h"
#include "SliderTextControl.h"

#include <Box.h>
#include <CheckBox.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <TextControl.h>
#include <StringView.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>

const static char* kFrameRateLabel = "Target frame rate";

const static uint32 kLocalFrameRateChanged = 'FrCh';
const static uint32 kAutoAdjust = 'FrCh';


FrameRateView::FrameRateView(Controller* controller)
	:
	BView("Frame Rate View", B_WILL_DRAW),
	fController(controller)
{	
	fFrameRateSlider = new SliderTextControl("frame rate",
			kFrameRateLabel, new BMessage(kLocalFrameRateChanged), 1, 60, 1, "fps");
			
	fAutoAdjust = new BCheckBox("AutoAdjust",
		"Auto Adjust", new BMessage(kAutoAdjust));			

	SetLayout(new BGroupLayout(B_VERTICAL));
	
	BView *layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL)
			.AddGroup(B_HORIZONTAL)
				.Add(fFrameRateSlider)
			.End()
			/*.Add(fAutoAdjust)*/
		.End()
		.View();
	
	AddChild(layoutView);
}


void
FrameRateView::AttachedToWindow()
{
	if (fController->LockLooper()) {
		fController->StartWatching(this, kMsgControllerCaptureStarted);
		fController->StartWatching(this, kMsgControllerCaptureStopped);
		fController->StartWatching(this, kMsgControllerResetSettings);
		fController->UnlockLooper();
	}
	fFrameRateSlider->SetValue(Settings::Current().CaptureFrameRate());
	fFrameRateSlider->SetTarget(this);
	/*fAutoAdjust->SetTarget(this);*/	
}


void
FrameRateView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kLocalFrameRateChanged:
		{
			int32 rate = fFrameRateSlider->Value();
			fController->SetCaptureFrameRate(rate);
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32("be:observe_change_what", &code);
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
