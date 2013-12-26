#ifndef __BSCAPP_H
#define __BSCAPP_H

#include <Application.h>

const static char *kAppSignature = "application/x-vnd.BeScreenCapture";
const static int32 kCmdToggleRecording = 'StoR';
class BSCApp : public BApplication {
public:
	BSCApp();
	virtual ~BSCApp();
	
	virtual void ReadyToRun();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *message);
	
private:
	BWindow *fWindow;
	bool fShouldStartRecording;
};

#endif
