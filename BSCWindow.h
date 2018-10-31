#ifndef __BSCWINDOW_H
#define __BSCWINDOW_H

#include <DirectWindow.h>
#include <Locker.h>
#include <Messenger.h>

#include <queue>


class BButton;
class BCardLayout;
class BMenuBar;
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
	
	bool IsRecording();
	
	status_t ReadBitmap(BBitmap *bitmap, BRect bounds);

	BLooper* GetController();
	
private:
	void _BuildMenu();
	void _LayoutWindow(bool dock = false);

	status_t _CaptureStarted();
	status_t _CaptureFinished();

	Controller *fController;

	BMenuBar* fMenuBar;
	BTabView *fTabView;
	BButton *fStartStopButton;
	BButton *fPauseButton;
	CamStatusView *fCamStatus;
	
	BView* fOutputView;
	BView* fAdvancedOptionsView;
	BView* fInfoView;

	bool fCapturing;
	BMessenger fAddonMessenger;
};

#endif
