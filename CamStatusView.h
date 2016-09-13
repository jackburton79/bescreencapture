#ifndef __CAMSTATUSVIEW_H
#define __CAMSTATUSVIEW_H

#include <View.h>

class Controller;
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
	Controller* fController;
	int32 fNumFrames;
	bool fRecording;
	bool fPaused;
};

#endif
