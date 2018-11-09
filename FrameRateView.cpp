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
const static char* kFrameRateLabel = "Target frame rate";

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
			kCaptureFreqLabel, "50" , new BMessage(kLocalCaptureFreqChanged));
		
	fFrameRate = new BTextControl("frame rate",
			kFrameRateLabel, "20" , new BMessage(kLocalFrameRateChanged));
			
	fAutoAdjust = new BCheckBox("AutoAdjust",
		"Auto Adjust", new BMessage(kAutoAdjust));			

	SetLayout(new BGroupLayout(B_VERTICAL));
	
	BView *layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL)
			.AddGroup(B_HORIZONTAL)
				.Add(fFrameRate)
				.Add(new BStringView("frameratelabel", "frames/sec"))
			.End()
			.AddGroup(B_HORIZONTAL)
				.Add(fCaptureFreq)
				.Add(new BStringView("capturefreqlabel", "milliseconds"))
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
		BMessenger messenger(this);
		fController->StartWatching(messenger, kMsgControllerEncodeStarted);
		fController->StartWatching(messenger, kMsgControllerEncodeFinished);
		fController->StartWatching(messenger, kMsgControllerResetSettings);
		fController->UnlockLooper();
	}
	fCaptureFreq->SetTarget(this);
	fFrameRate->SetTarget(this);
	/*fAutoAdjust->SetTarget(this);*/
	
	float milliSeconds = (float)Settings::Current().CaptureFrameDelay();
	
	_UpdateCaptureRate(&milliSeconds, NULL);
}


void
FrameRateView::MessageReceived(BMessage* message)
{
	switch (message->what) {
		case kLocalCaptureFreqChanged:
		{
			float delay = strtof(fCaptureFreq->Text(), NULL);
			_UpdateCaptureRate(&delay, NULL);
			break;
		}
		case kLocalFrameRateChanged:
		{
			float rate = strtof(fFrameRate->Text(), NULL);
			_UpdateCaptureRate(NULL, &rate);
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32("be:observe_change_what", &code);
			switch (code) {
				case kMsgControllerEncodeStarted:
					fCaptureFreq->SetEnabled(false);
					fFrameRate->SetEnabled(false);
					break;
				case kMsgControllerEncodeFinished:
					fCaptureFreq->SetEnabled(true);
					fFrameRate->SetEnabled(true);
					break;
				case kMsgControllerCaptureFrameDelayChanged:
				{
					int32 delay;
					if (message->FindInt32("delay", &delay) == B_OK) {
						float floatDelay = (float)delay;					
						_UpdateCaptureRate(&floatDelay, NULL);
					}
					break;
				}
				case kMsgControllerResetSettings:
				{
					float milliSeconds = (float)Settings::Current().CaptureFrameDelay();
					_UpdateCaptureRate(&milliSeconds, NULL);
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


void
FrameRateView::_UpdateCaptureRate(float *delay, float *rate)
{
	BString text;
	float targetDelay = 0;
	if (delay != NULL) {
		targetDelay = *delay;
		CapFloat(targetDelay, 10, 1000);
		text << targetDelay;
		fCaptureFreq->SetText(text);
		
		text = "";
		text << (1000/targetDelay);
		
		fFrameRate->SetText(text);
	} else if (rate != NULL) {
		float targetRate = *rate;
		CapFloat(targetRate, 2, 60);
		text << targetRate;
		fFrameRate->SetText(text);
		
		targetDelay = 1000/targetRate;
		text = "";
		text << targetDelay;
		fCaptureFreq->SetText(text);
	}
	
	fController->SetCaptureFrameDelay(targetDelay);
}
