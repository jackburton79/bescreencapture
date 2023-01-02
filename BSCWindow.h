/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __BSCWINDOW_H
#define __BSCWINDOW_H

#include <DirectWindow.h>
#include <Locker.h>
#include <Messenger.h>

#include <queue>


class BButton;
class BMenuBar;
class BStringView;
class BTabView;
class CamStatusView;
class BSCWindow : public BDirectWindow {
public:
	BSCWindow();
	~BSCWindow();
	
	virtual void DispatchMessage(BMessage *, BHandler *);
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *);
	virtual void ScreenChanged(BRect screen_size, color_space depth);
	virtual void DirectConnected(direct_buffer_info *info);
	
	status_t ReadBitmap(BBitmap *bitmap, BRect bounds);
	
private:
	void _BuildMenu();
	void _LayoutWindow(bool dock = false);

	status_t _CaptureStarted();
	status_t _CaptureFinished();

	BMenuBar* fMenuBar;
	BTabView *fTabView;
	BButton *fStartStopButton;
	BButton *fPauseButton;
	CamStatusView *fCamStatus;
	
	BView* fOutputView;
	BView* fAdvancedOptionsView;
	BView* fInfoView;

	BMessenger fAddonMessenger;
};

#endif // __BSCWINDOW_H
