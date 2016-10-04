#ifndef __BSCAPP_H
#define __BSCAPP_H

#include <Application.h>

#define kAppSignature "application/x-vnd.BeScreenCapture"

enum {
	kCmdToggleRecording = 'StoR'
};


class Arguments;
class BSCApp : public BApplication {
public:
	BSCApp();
	virtual ~BSCApp();
	
	virtual void ArgvReceived(int32 argc, char** argv);
	virtual void ReadyToRun();
	virtual bool QuitRequested();
	virtual void MessageReceived(BMessage *message);
	virtual void AboutRequested();

	bool WasLaunchedSilently() const;
	bool LaunchedFromCommandline() const;
	
private:
	void _UsageRequested();

	BWindow *fWindow;
	Arguments* fArgs;
	bool fShouldStartRecording;
};

#endif
