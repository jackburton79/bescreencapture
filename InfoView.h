/*
 * Copyright 2016 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef INFOVIEW_H
#define INFOVIEW_H


#include <String.h>
#include <View.h>

class Controller;
class BRect;
class BStringView;
class InfoView : public BView {
public:
	InfoView(Controller* controller);
	virtual void MessageReceived(BMessage* message);
									
private:
	static BString RectToString(const BRect& rect);

	Controller* fController;
	BStringView* fSourceSize;
	BStringView* fClipSize;
	BStringView* fCodec;
};


#endif // INFOVIEW_H
