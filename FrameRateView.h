/*
 * Copyright 2017-2021 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __FRAMERATEVIEW_H
#define __FRAMERATEVIEW_H

#include <View.h>

class Controller;
class SliderTextControl;
class FrameRateView : public BView {
public:
	FrameRateView(Controller* controller);
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);
	
private:
	Controller* fController;
	SliderTextControl* fFrameRateSlider;
};


#endif // __FRAMERATEVIEW_H
