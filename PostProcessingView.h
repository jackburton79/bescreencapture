#ifndef __POSTPROCESSINGVIEW_H
#define __POSTPROCESSINGVIEW_H

#include <View.h>

class PostProcessingView : public BView {
public:
	PostProcessingView(const char *name, uint32 = B_WILL_DRAW);
	
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);
	
	virtual BSize MinSize();
};


#endif
