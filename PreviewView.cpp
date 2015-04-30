#include "PreviewView.h"

#include <cstdio>

#include <AbstractLayout.h>
#include <Alignment.h>
#include <Bitmap.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Screen.h>
#include <StringView.h>

#include <iostream>

class BitmapView : public BView {
public:
	BitmapView();
	virtual ~BitmapView();
	
	virtual void Draw(BRect update);
	//virtual bool HasHeightForWidth() { return true; };
	virtual void GetHeightForWidth(float width, float* min, float* max, float* preferred);
	virtual BSize MinSize();
};


PreviewView::PreviewView()
	:
	BView("Rect View", B_WILL_DRAW),
	fBitmapView(NULL),
	fLeftTop(NULL),
	fRightBottom(NULL)
{
	fCoordRect = BRect(10, 10, 20, 20);
	fChanged = true;

	SetLayout(new BGroupLayout(B_VERTICAL));
	//SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
	//	B_ALIGN_VERTICAL_CENTER));
	BLayoutBuilder::Group<>(this)
	.AddGroup(B_VERTICAL, 0)
		//.AddGroup(B_HORIZONTAL)
		//	.AddGlue()
		//	.AddGlue()
		//.End()
		.AddGroup(B_HORIZONTAL, 0)
			//.AddStrut(10)
			//.Add(fLeftTop = new BStringView("lefttop", ""))
			.Add(fBitmapView = new BitmapView())
			//.Add(fRightBottom = new BStringView("rightbottom", ""))
			//.AddStrut(10)
			.End()
		/*.AddGroup(B_HORIZONTAL)
			.AddGlue()
			.AddGlue()
		.End()*/
		.End();
	
	/*fLeftTop->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT,
		B_ALIGN_TOP));
	fRightBottom->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT,
		B_ALIGN_BOTTOM));*/
}


void
PreviewView::AttachedToWindow()
{	
	BView::AttachedToWindow();
	SetViewColor(Parent()->ViewColor());

	Update();
}


void
PreviewView::_SetRect(const BRect& rect)
{
	if (fCoordRect != rect) {
		fCoordRect = rect;
		
		char str[16];
		snprintf(str, sizeof(str), "%d, %d", (int)rect.left, (int)rect.top);
		//fLeftTop->SetText(str);
		snprintf(str, sizeof(str), "%d, %d", (int)rect.right, (int)rect.bottom);
		//fRightBottom->SetText(str);
	}
}


void
PreviewView::Update(const BRect* rect, BBitmap* bitmap)
{
	if (rect != NULL)
		_SetRect(*rect);

	if (bitmap == NULL && Window() != NULL) {
		BScreen screen(Window());
		screen.GetBitmap(&bitmap, false, &fCoordRect);
	}
	if (bitmap != NULL) {
		fBitmapView->SetViewBitmap(bitmap,
			bitmap->Bounds().OffsetToCopy(B_ORIGIN),
			fBitmapView->Bounds(),
			B_FOLLOW_TOP|B_FOLLOW_LEFT, 0);
		if (Window() != NULL)
			Invalidate();
	}
}


// BitmapView
BitmapView::BitmapView()
	:
	BView("bitmap view", B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE)
{
}


BitmapView::~BitmapView()
{
}


void
BitmapView::Draw(BRect rect)
{
	BView::Draw(rect);
	SetHighColor(0, 0, 0);
	StrokeRect(Bounds());
}


void
BitmapView::GetHeightForWidth(float width, float* min,
			float* max, float* preferred)
{
	float preferredHeight = (width / 16) * 9;
	float variation = preferredHeight / 5;
	if (preferred != NULL)
		*preferred = preferredHeight;
	if (min != NULL)
		*min = preferredHeight - variation;
	if (max != NULL)
		*max = preferredHeight;
	std::cout << "width: " << width << ", min: " << (min ? *min : 0);
	std::cout << ", preferred: " << (preferred ? *preferred : 0);
	std::cout << ", max: " << (max ? *max : 0) << std::endl;
}
	
	
BSize
BitmapView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(90, 70));
}

