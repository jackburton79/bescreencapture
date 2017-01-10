/*
 * Copyright 2017 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef __FRAMERATEVIEW_H
#define __FRAMERATEVIEW_H


#include <View.h>

class Controller;
class BCheckBox;
class BTextControl;
class FrameRateView : public BView {
public:
	FrameRateView(Controller* controller);
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);
	
private:
	Controller* fController;
	BTextControl* fCaptureFreq;
	BTextControl* fFrameRate;
	BCheckBox* fAutoAdjust;
};


#endif // __FRAMERATEVIEW_H
