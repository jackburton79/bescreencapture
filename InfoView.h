/*
 * Copyright 2016-2021 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef INFOVIEW_H
#define INFOVIEW_H


#include <View.h>

class Controller;
class BStringView;
class InfoView : public BView {
public:
	InfoView(Controller* controller);
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);

private:
	Controller* fController;
	BStringView* fSourceSize;
	BStringView* fClipSize;
	BStringView* fScale;
	BStringView* fFormat;
	BStringView* fCodec;
	BStringView* fCaptureFrameRate;
};


#endif // INFOVIEW_H
