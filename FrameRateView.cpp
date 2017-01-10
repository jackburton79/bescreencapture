/*
 * Copyright 2017 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "FrameRateView.h"

#include "Controller.h"

#include <Box.h>
#include <CheckBox.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <TextControl.h>
#include <StringView.h>

#include <cstdlib>

const static char* kCaptureFreqLabel = "Capture frame every";
const static char* kFrameRateLabel = "Frame rate";

const static uint32 kLocalCaptureFreqChanged = 'CFCh';
const static uint32 kLocalFrameRateChanged = 'FrCh';
const static uint32 kAutoAdjust = 'FrCh';



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
			.AddGroup(B_HORIZONTAL)
				.Add(fFrameRate)
				.Add(new BStringView("frameratelabel", "frame/sec"))
			.End()
			.Add(fAutoAdjust)
		.End()
		.View();
	
	AddChild(layoutView);

}


void
FrameRateView::AttachedToWindow()
{
	fCaptureFreq->SetTarget(this);
	fFrameRate->SetTarget(this);
	fAutoAdjust->SetTarget(this);
}


void
FrameRateView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kLocalCaptureFreqChanged:
		{
			float rate = strtof(fCaptureFreq->Text(), NULL);
			fController->SetCaptureFrameRate(rate);
		}
		case kLocalFrameRateChanged:
		{
			float rate = strtof(fFrameRate->Text(), NULL);
			fController->SetPlaybackFrameRate(rate);
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}
