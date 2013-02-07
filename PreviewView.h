#ifndef __PREVIEWVIEW_H
#define __PREVIEWVIEW_H

#include <View.h>

class BStringView;
class PreviewView : public BView {
public:
	PreviewView();
	virtual void AttachedToWindow();
	void SetRect(BRect rect);

private:
	BView *fBitmapView;
	BRect fCoordRect;
	bool fChanged;

	BStringView *fTop;
	BStringView *fLeft;
	BStringView *fRight;
	BStringView *fBottom;

	void UpdateBitmap();
	
	virtual BSize MinSize();
};


#endif // __PREVIEWVIEW_H
