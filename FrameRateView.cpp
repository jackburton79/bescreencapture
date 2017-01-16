/*
 * Copyright 2017 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "FrameRateView.h"

#include "Controller.h"
#include "ControllerObserver.h"
#include "Settings.h"

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

const static char* kCaptureFreqLabel = "Capture frame every";
const static char* kFrameRateLabel = "Frame rate";

const static uint32 kLocalCaptureFreqChanged = 'CFCh';
const static uint32 kLocalFrameRateChanged = 'FrCh';
const static uint32 kAutoAdjust = 'FrCh';


static
void
CapFloat(float &toLimit, const float min, const float max)
{
	toLimit = std::max(min, toLimit);
	toLimit = std::min(max, toLimit);
}


FrameRateView::FrameRateView(Controller* controller)
	:
	BView("Frame Rate View", B_WILL_DRAW),
	fController(controller)
{
	fCaptureFreq = new BTextControl("capture freq",
			kCaptureFreqLabel, "20" , new BMessage(kLocalCaptureFreqChanged));
		
	fFrameRate = new BTextControl("frame rate",
			kFrameRateLabel, "30" , new BMessage(kLocalFrameRateChanged));
			
	fAutoAdjust = new BCheckBox("AutoAdjust",
		"Auto Adjust", new BMessage(kAutoAdjust));			

	SetLayout(new BGroupLayout(B_VERTICAL));
	
	BView *layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL)
			.AddGroup(B_HORIZONTAL)
				.Add(fCaptureFreq)
				.Add(new BStringView("capturefreqlabel", "milliseconds"))
			.End()
			/*.AddGroup(B_HORIZONTAL)
				.Add(fFrameRate)
				.Add(new BStringView("frameratelabel", "frame/sec"))
			.End()
			.Add(fAutoAdjust)*/
		.End()
		.View();
	
	AddChild(layoutView);

}


void
FrameRateView::AttachedToWindow()
{
	fCaptureFreq->SetTarget(this);
	/*fFrameRate->SetTarget(this);
	fAutoAdjust->SetTarget(this);*/
	
	int32 milliSeconds = Settings().CaptureFrameDelay();
	BString text;
	text << milliSeconds;
	fCaptureFreq->SetText(text);
}


void
FrameRateView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kLocalCaptureFreqChanged:
		{
			float delay = strtof(fCaptureFreq->Text(), NULL);
			CapFloat(delay, 10, 1000);
			BString text;
			text << delay;
			fCaptureFreq->SetText(text);
			fController->SetCaptureFrameDelay(delay);
			break;
		}
		case kLocalFrameRateChanged:
		{
			float rate = strtof(fFrameRate->Text(), NULL);
			fController->SetPlaybackFrameRate(rate);
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32("be:observe_change_what", &code);
			switch (code) {
				case kMsgControllerCaptureFrameDelayChanged:
				{
					int32 delay;
					if (message->FindInt32("delay", &delay) == B_OK) {						
						BString text;
						text << delay;
						fCaptureFreq->SetText(text);
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
