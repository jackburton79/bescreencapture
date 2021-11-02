/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __CAMSTATUSVIEW_H
#define __CAMSTATUSVIEW_H

#include <View.h>

class BBitmap;
class BStatusBar;
class BStringView;
class Controller;
class SquareBitmapView;
class CamStatusView : public BView {
public:
	CamStatusView(Controller* controller);

	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage *message);	
	virtual void Pulse();

	void TogglePause(const bool paused);
	bool Paused() const;

	void SetRecording(const bool recording);
	bool Recording() const;

	virtual BSize MinSize();
	virtual BSize MaxSize();

private:
	BString _GetRecordingTimeString() const;

	Controller* fController;
	BStringView* fStringView;
	SquareBitmapView* fBitmapView;
	BStringView* fEncodingStringView;
	BStatusBar* fStatusBar;
	int32 fNumFrames;
	bool fRecording;
	bool fPaused;
	BBitmap* fRecordingBitmap;
	BBitmap* fPauseBitmap;
};

#endif
