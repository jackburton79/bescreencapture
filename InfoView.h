/*
 * Copyright 2016 Stefano Ceccherini <stefano.ceccherini@gmail.com>
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
								
private:
	Controller* fController;
	BStringView* fSourceSize;
	BStringView* fClipSize;
	BStringView* fCodec;
};


#endif // INFOVIEW_H
