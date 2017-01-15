/*
 * Copyright 2015 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef SIZECONTROL_H
#define SIZECONTROL_H


#include <Slider.h>
#include <SupportDefs.h>

class BMessage;
class BTextControl;
class SizeControl : public BControl {
public:
	SizeControl(const char* name, const char* label,
		BMessage* message, int32 minValue,
		int32 maxValue, int32 stepValue = 1,
		const char* unit = "", orientation posture = B_HORIZONTAL,
		thumb_style thumbType = B_BLOCK_THUMB,
		uint32 flags = B_NAVIGABLE | B_WILL_DRAW
							| B_FRAME_EVENTS);
	
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);
	virtual void SetValue(int32 value);
	virtual void SetEnabled(bool enable);
private:
	BSlider* fSizeSlider;
	BTextControl* fSizeTextControl;
	uint32 fWhat;
};


#endif // _H
