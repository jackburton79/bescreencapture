#ifndef __SELECTIONWINDOW_H
#define __SELECTIONWINDOW_H

#include <Window.h>

class SelectionView;
class SelectionWindow : public BWindow {
public:
	SelectionWindow();
	virtual void Show();
	virtual void SetTarget(BLooper *looper);
	virtual void SetCommand(uint32 command);
	virtual bool QuitRequested();
	
private:	
	SelectionView *fView;	
	BLooper *fTarget;
	uint32 fCommand;
};

#endif
