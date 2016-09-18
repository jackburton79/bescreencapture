/*
 * Copyright 2015 Your Name <your@email.address>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "SizeControl.h"

#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <Slider.h>
#include <String.h>

#include <stdlib.h>

const int32 kTextControlMessage = '9TCM';

class SizeSlider : public BSlider {
public:
	SizeSlider(const char* name, const char* label,
		BMessage* message, int32 minValue,
		int32 maxValue, orientation posture,
		thumb_style thumbType = B_BLOCK_THUMB,
		uint32 flags = B_NAVIGABLE | B_WILL_DRAW
							| B_FRAME_EVENTS);

	virtual void SetValue(int32 value);
};


SizeControl::SizeControl(const char* name, const char* label,
		BMessage* message, int32 minValue,
		int32 maxValue, orientation posture,
		thumb_style thumbType,
		uint32 flags)
	:
	BControl(name, label, message, flags),
	fWhat(message->what)
{
	fSizeSlider = new SizeSlider("size_slider", label,
		message, minValue, maxValue, B_HORIZONTAL);

	fSizeSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fSizeSlider->SetHashMarkCount(8);
	
	BString minLabel;
	minLabel << minValue << "%";
	BString maxLabel;
	maxLabel << maxValue << "%";
	
	fSizeSlider->SetLimitLabels(minLabel.String(), maxLabel.String());
	fSizeTextControl = new BTextControl("%", "", new BMessage(kTextControlMessage));
	
	BLayoutBuilder::Group<>(this)
	.AddGroup(B_HORIZONTAL, 1)
		.Add(fSizeSlider, 20)
		.AddGroup(B_VERTICAL, 1)
			.AddStrut(1)
			.Add(fSizeTextControl)
			.End()
		.End();
		
	fSizeTextControl->SetModificationMessage(new BMessage(kTextControlMessage));
	fSizeTextControl->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_TOP));
	fSizeTextControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
}


void
SizeControl::AttachedToWindow()
{
	fSizeTextControl->SetTarget(this);
	fSizeSlider->SetTarget(this);
}


void
SizeControl::MessageReceived(BMessage* message)
{
	if (message->what == fWhat) {
		BString sizeString;
		sizeString << (int32)fSizeSlider->Value();
		fSizeTextControl->SetText(sizeString);
		Window()->PostMessage(message, Target());
		return;
	}
	switch (message->what) {
		case kTextControlMessage:
		{
			int32 value = atoi(fSizeTextControl->TextView()->Text());
			fSizeSlider->SetValue(value);
			break;
		}
		default:
			BControl::MessageReceived(message);
			break;
	}	
}
	

void
SizeControl::SetValue(int32 value)
{
	fSizeSlider->SetValue(value);
	BString sizeString;
	sizeString << (int32)fSizeSlider->Value();
	fSizeTextControl->SetText(sizeString);
}


// SizeSlider
SizeSlider::SizeSlider(const char* name, const char* label,
		BMessage* message, int32 minValue,
		int32 maxValue, orientation posture,
		thumb_style thumbType,
		uint32 flags)
	:
	BSlider(name, label, message, minValue, maxValue, posture, thumbType, flags)
{
}


/* virtual */
void
SizeSlider::SetValue(int32 value)
{
	// TODO: Not really, nice, should not have a fixed list of values
	const int32 validValues[] = { 25, 50, 75, 100, 125, 150, 175, 200 };
	int32 numValues = sizeof(validValues) / sizeof(int32);
	for (int32 i = 0; i < numValues - 1; i++) {
		if (value > validValues[i] && value < validValues[i + 1]) {
			value = value > validValues[i] + (validValues[i + 1] - validValues[i]) / 2
				? validValues[i + 1] : validValues[i];
		}
	}
	
	BSlider::SetValue(value);
}
