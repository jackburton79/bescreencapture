/*
 * Copyright 2015-2016 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SliderTextControl.h"

#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <Slider.h>
#include <String.h>
#include <StringView.h>

#include <algorithm>
#include <cstdlib>
#include <iostream>

const int32 kTextControlMessage = '9TCM';
const int32 kTextControlRunnerMessage = '9RTC';

class SizeSlider : public BSlider {
public:
	SizeSlider(const char* name, const char* label,
		BMessage* message, int32 minValue,
		int32 maxValue, int32 stepValue,
		orientation posture,
		thumb_style thumbType = B_BLOCK_THUMB,
		uint32 flags = B_NAVIGABLE | B_WILL_DRAW
							| B_FRAME_EVENTS);

	virtual void SetValue(int32 value);
private:
	int32 fStep;
};


SliderTextControl::SliderTextControl(const char* name, const char* label,
		BMessage* message, int32 minValue,
		int32 maxValue, int32 stepValue, const char* unit,
		orientation posture,
		thumb_style thumbType,
		uint32 flags)
	:
	BControl(name, label, message, flags),
	fSizeSlider(NULL),
	fSizeTextControl(NULL),
	fTextModificationRunner(NULL),
	fWhat(message->what)
{
	fSizeSlider = new SizeSlider("size_slider", label,
		new BMessage(*message), minValue, maxValue, stepValue, B_HORIZONTAL);

	fSizeSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fSizeSlider->SetHashMarkCount(8);
	
	BString minLabel;
	minLabel << minValue << " " << unit;
	BString maxLabel;
	maxLabel << maxValue << " " << unit;
	
	fSizeSlider->SetLimitLabels(minLabel.String(), maxLabel.String());
	fSizeTextControl = new BTextControl("", "", new BMessage(kTextControlMessage));
	fTextLabel = new BStringView("label", unit);
	
	BLayoutBuilder::Group<>(this)
	.AddGroup(B_HORIZONTAL, 1)
		.Add(fSizeSlider, 20)
		.AddGroup(B_VERTICAL, 1)
			.AddStrut(1)
			.Add(fSizeTextControl)
		.End()
		.Add(fTextLabel)
	.End();
		
	fSizeTextControl->SetModificationMessage(new BMessage(kTextControlMessage));
	fSizeTextControl->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_TOP));
	fSizeTextControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
}


void
SliderTextControl::AttachedToWindow()
{
	fSizeTextControl->SetTarget(this);
	fSizeSlider->SetTarget(this);
}


void
SliderTextControl::MessageReceived(BMessage* message)
{
	if (message->what == fWhat) {
		BString sizeString;
		sizeString << (int32)fSizeSlider->Value();
		fSizeTextControl->SetModificationMessage(NULL);
		fSizeTextControl->SetText(sizeString);
		BControl::SetValue(fSizeSlider->Value());
		fSizeTextControl->SetModificationMessage(new BMessage(kTextControlMessage));
		Window()->PostMessage(message, Target());
		return;
	}
	switch (message->what) {
		case kTextControlMessage:
		{
			if (fTextModificationRunner == NULL) {
				BMessenger thisMessenger(this);
				fTextModificationRunner = new BMessageRunner(thisMessenger,
					new BMessage(kTextControlRunnerMessage), 1000000, 1);
			} else
				fTextModificationRunner->SetInterval(1000000);
			break;
		}
		case kTextControlRunnerMessage:
		{
			// Apply limits
			int32 value = atoi(fSizeTextControl->TextView()->Text());
			int32 min;
			int32 max;
			fSizeSlider->GetLimits(&min, &max);
			value = std::max(value, min);
			value = std::min(value, max);
			BControl::SetValue(value);

			fSizeSlider->SetValue(value);

			BString text;
			text << value;
			if (::strcmp(text, fSizeTextControl->Text()) != 0) {
				// Avoid sending the modification message
				fSizeTextControl->SetModificationMessage(NULL);
				fSizeTextControl->SetText(text.String());
				fSizeTextControl->SetModificationMessage(new BMessage(kTextControlMessage));
			}

			BMessage whatMessage(fWhat);
			whatMessage.AddInt32("be:value", value);
			Window()->PostMessage(&whatMessage, Target());

			delete fTextModificationRunner;
			fTextModificationRunner = NULL;
			break;
		}
		default:
			BControl::MessageReceived(message);
			break;
	}	
}
	

void
SliderTextControl::SetValue(int32 value)
{
	fSizeSlider->SetValue(value);
	BString sizeString;
	sizeString << (int32)fSizeSlider->Value();
	fSizeTextControl->SetText(sizeString);
	
	BControl::SetValue(value);
}


/* virtual */
void
SliderTextControl::SetEnabled(bool enable)
{
	BControl::SetEnabled(enable);
	fSizeSlider->SetEnabled(enable);
	fSizeTextControl->SetEnabled(enable);
}


// SizeSlider
SizeSlider::SizeSlider(const char* name, const char* label,
		BMessage* message, int32 minValue,
		int32 maxValue, int32 stepValue, orientation posture,
		thumb_style thumbType,
		uint32 flags)
	:
	BSlider(name, label, message, minValue, maxValue, posture, thumbType, flags),
	fStep(stepValue)
{
}


/* virtual */
void
SizeSlider::SetValue(int32 value)
{
	int32 max, min;
	GetLimits(&min, &max);

	const int32 numValues = 1 + (max - min) / fStep;
	int32 validValues[numValues];
	for (int32 i = 0; i < numValues; i++)
		validValues[i] = min + fStep * i;

	for (int32 i = 0; i < numValues - 1; i++) {
		if (value > validValues[i] && value < validValues[i + 1]) {
			value = value > validValues[i] + (validValues[i + 1] - validValues[i]) / 2
				? validValues[i + 1] : validValues[i];
		}
	}

	BSlider::SetValue(value);
}
