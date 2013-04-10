#ifndef __ADVANCEDOPTIONSVIEW_H
#define __ADVANCEDOPTIONSVIEW_H

#include <View.h>

class BCheckBox;
class BSlider;
class BTextControl;
class BOptionPopUp;
class Controller;
class AdvancedOptionsView : public BView {
public:
	AdvancedOptionsView(Controller* controller);
	
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);

	virtual BSize MinSize();
	
private:
	Controller* fController;
	BCheckBox* fUseDirectWindow;
	
	BOptionPopUp* fDepthControl;
	BOptionPopUp* fPriorityControl;
	
	BSlider* fSizeSlider;
	BTextControl* fSizeTextControl;
};

#endif
