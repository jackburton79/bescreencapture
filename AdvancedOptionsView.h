#ifndef __ADVANCEDOPTIONSVIEW_H
#define __ADVANCEDOPTIONSVIEW_H

#include <View.h>

class BCheckBox;
class BOptionPopUp;
class Controller;
class SizeControl;
class AdvancedOptionsView : public BView {
public:
	AdvancedOptionsView(Controller* controller);
	
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);
	
private:
	Controller* fController;
	BCheckBox* fUseDirectWindow;
	
	BOptionPopUp* fDepthControl;
	
	BCheckBox* fHideDeskbarIcon;
};

#endif
