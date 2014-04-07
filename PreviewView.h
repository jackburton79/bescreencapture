#ifndef __PREVIEWVIEW_H
#define __PREVIEWVIEW_H

#include <View.h>

class BStringView;
class PreviewView : public BView {
public:
	PreviewView();
	virtual void AttachedToWindow();
	void SetRect(BRect rect);
	void UpdateBitmap(BBitmap* bitmap = NULL);

private:
	BView *fBitmapView;
	BRect fCoordRect;
	bool fChanged;

	BStringView *fLeftTop;
	BStringView *fRightBottom;

	
	virtual BSize MinSize();
};


#endif // __PREVIEWVIEW_H
