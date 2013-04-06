#ifndef __BSCWINDOW_H
#define __BSCWINDOW_H

#include <DirectWindow.h>
#include <Locker.h>
#include <Messenger.h>

#include <queue>


class BButton;
class BCardLayout;
class BStatusBar;
class BStringView;
class BTabView;
class Controller;
class ControllerObserver;
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

	BLooper* GetController();
	
private:
	status_t _CaptureStarted();
	status_t _CaptureFinished();
		
	Controller *fController;
	ControllerObserver *fObserver;

	BTabView *fTabView;
	BButton *fStartStopButton;
	BStringView *fStringView;
	BStringView *fRecordingInfo;
	BStatusBar *fStatusBar;
	CamStatusView *fCamStatus;
	BCardLayout* fCardLayout;
	
	bool fCapturing;
	BMessenger fAddonMessenger;
};

#endif
