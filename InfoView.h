/*
 * Copyright 2016-2023 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef INFOVIEW_H
#define INFOVIEW_H


#include <View.h>

class BStringView;
class InfoView : public BView {
public:
	InfoView();
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage* message);

private:
	BStringView* fSourceSize;
	BStringView* fClipSize;
	BStringView* fScale;
	BStringView* fFormat;
	BStringView* fCodec;
	BStringView* fCaptureFrameRate;
};


#endif // INFOVIEW_H
