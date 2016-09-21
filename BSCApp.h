#ifndef __BSCAPP_H
#define __BSCAPP_H

#include <Application.h>

#define kAppSignature "application/x-vnd.BeScreenCapture"

enum {
	kCmdToggleRecording = 'StoR'
};


class BSCApp : public BApplication {
public:
	BSCApp();
	virtual ~BSCApp();
	
	virtual void ReadyToRun();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *message);
	virtual void AboutRequested();
	
private:
	BWindow *fWindow;
	bool fShouldStartRecording;
};

#endif
