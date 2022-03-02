/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __DESKBAR_CONTROL_H
#define __DESKBAR_CONTROL_H

#include <Messenger.h>
#include <View.h>

class BBitmap;
class DeskbarControlView : public BView {
public:
	DeskbarControlView(BRect rect);
	DeskbarControlView(BMessage *data);
	
	virtual ~DeskbarControlView();
	
	static DeskbarControlView *Instantiate(BMessage *archive);
	virtual status_t Archive(BMessage *message, bool deep) const;
	
	virtual void AttachedToWindow();
	virtual void DetachedFromWindow();
	
	virtual void Draw(BRect rect);
	virtual void MessageReceived(BMessage *message);

	virtual void MouseDown(BPoint where);

	virtual void Pulse();

private:
	void InitData();
	
	BMessenger fControllerMessenger;
	BMessenger fAppMessenger;
	BBitmap* fBitmap;
	
	bool fRecording;
	bool fPaused;
};


#endif // __DESKBAR_CONTROL_H
