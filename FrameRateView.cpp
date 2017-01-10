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

const static char* kCaptureFreqLabel = "Capture frame every";
const static char* kFrameRateLabel = "Frame rate";

const static uint32 kCaptureFreqChanged = 'CFCh';
const static uint32 kFrameRateChanged = 'FrCh';
const static uint32 kAutoAdjust = 'FrCh';

FrameRateView::FrameRateView(Controller* controller)
	:
	BView("Frame Rate View", B_WILL_DRAW),
	fController(controller)
{
	fCaptureFreq = new BTextControl("capture freq",
			kCaptureFreqLabel, "20" , new BMessage(kCaptureFreqChanged));
		
	fFrameRate = new BTextControl("frame rate",
			kFrameRateLabel, "30" , new BMessage(kFrameRateChanged));
			
	fAutoAdjust = new BCheckBox("AutoAdjust",
		"Auto Adjust", new BMessage(kAutoAdjust));			

	SetLayout(new BGroupLayout(B_VERTICAL));
	
	BView *layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL)
			.Add(fCaptureFreq)
			.Add(fFrameRate)
			.Add(fAutoAdjust)
		.End()
		.View();
	
	AddChild(layoutView);

}


void
FrameRateView::AttachedToWindow()
{
}
