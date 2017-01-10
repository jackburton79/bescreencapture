#ifndef __PREVIEWVIEW_H
#define __PREVIEWVIEW_H

#include <View.h>

class BStringView;
class PreviewView : public BView {
public:
	PreviewView();
	virtual void AttachedToWindow();
	void Update(const BRect* rect = NULL, BBitmap* bitmap = NULL);
	BRect Rect() const;
	
private:
	BView *fBitmapView;
	BRect fCoordRect;
	bigtime_t fTimeStamp;
	bool fChanged;

	BStringView *fLeftTop;
	BStringView *fRightBottom;
	
	void _SetRect(const BRect& rect);
};


#endif // __PREVIEWVIEW_H
