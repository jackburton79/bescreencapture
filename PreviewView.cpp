#include "PreviewView.h"

#include <cstdio>

#include <Bitmap.h>
#include <LayoutUtils.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <Screen.h>
#include <StringView.h>


class BitmapView : public BView {
public:
	BitmapView();
	virtual ~BitmapView();
	
	virtual void Draw(BRect update);
	virtual BSize MinSize();
};


PreviewView::PreviewView()
	:
	BView("Rect View", B_WILL_DRAW),
	fBitmapView(NULL),
	fTop(NULL),
	fLeft(NULL),
	fRight(NULL),
	fBottom(NULL)
{
	fCoordRect = BRect(10, 10, 20, 20);
	fChanged = true;
	
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	AddChild(BGroupLayoutBuilder(B_VERTICAL)
		.Add(fTop = new BStringView("top", "top"))
		.AddGroup(B_HORIZONTAL)
			.Add(fLeft = new BStringView("left", "left"))
			.Add(fBitmapView = new BitmapView())
			.Add(fRight = new BStringView("right", "right"))
		.End()
		.Add(fBottom = new BStringView("bottom", "bottom"))
	);
}


void
PreviewView::AttachedToWindow()
{	
	BView::AttachedToWindow();
	SetViewColor(Parent()->ViewColor());
	UpdateBitmap();
}


void
PreviewView::SetRect(BRect rect)
{
	if (fCoordRect == rect)
		return;
		
	fCoordRect = rect;
	
	char str[16];
	snprintf(str, sizeof(str), "%d", (int)rect.left);
	fLeft->SetText(str);
	snprintf(str, sizeof(str), "%d", (int)rect.top);
	fTop->SetText(str);
	snprintf(str, sizeof(str), "%d", (int)rect.right);
	fRight->SetText(str);
	snprintf(str, sizeof(str), "%d", (int)rect.bottom);
	fBottom->SetText(str);
	
	fChanged = true;
	
	UpdateBitmap();
}


void
PreviewView::UpdateBitmap()
{
	if (Window() == NULL)
		printf("window is null\n");
	if (fChanged && Window() != NULL) {
		BBitmap *bitmap;
		fChanged = false;
		BScreen screen(Window());
		screen.GetBitmap(&bitmap, false, &fCoordRect);		
	
		if (bitmap != NULL) {
			fBitmapView->SetViewBitmap(bitmap, bitmap->Bounds(),
				fBitmapView->Bounds(),
				B_FOLLOW_TOP|B_FOLLOW_LEFT, 0);
			Invalidate();
		}
	}
}


BSize
PreviewView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(160, 100));
}


// BitmapView
BitmapView::BitmapView()
	:
	BView("bitmap view", B_WILL_DRAW)
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


BSize
BitmapView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(100, 60));
}

