#ifndef __CAMSTATUSVIEW_H
#define __CAMSTATUSVIEW_H

#include <View.h>

class CamStatusView : public BView {
public:
	CamStatusView(const char *name);
	
	virtual void AttachedToWindow();
	virtual void Draw(BRect updateRect);
	virtual void MessageReceived(BMessage *message);	

	void TogglePause();
	bool Paused() const;
	
	void SetRecording(const bool recording);
	bool Recording() const;

	virtual BSize MinSize();
	virtual BSize MaxSize();
private:
	bool fRecording;
	bool fPaused;
};

#endif
