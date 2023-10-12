/*
 * Copyright 2017-2023 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __FRAMERATEVIEW_H
#define __FRAMERATEVIEW_H

#include <View.h>

class SliderTextControl;
class FrameRateView : public BView {
public:
	FrameRateView();
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);

private:
	SliderTextControl* fFrameRateSlider;
};


#endif // __FRAMERATEVIEW_H
