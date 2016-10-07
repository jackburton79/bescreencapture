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
	fCoordRect = BScreen().Frame();
	fChanged = true;

	SetLayout(new BGroupLayout(B_VERTICAL));
	BLayoutBuilder::Group<>(this)
	.AddGroup(B_VERTICAL, 0)
		.AddGroup(B_HORIZONTAL, 0)
			.Add(fBitmapView = new BitmapView())
		.End()
	.End();
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
	fCoordRect = rect;
	
	BString text;
	text << "Capture area:\n";
	text << "left: " << (int)rect.left << ", ";
	text << "top: " << (int)rect.top << ", ";
	text << "right: " << (int)rect.right << ", ";
	text << "bottom: " << (int)rect.bottom;
	SetToolTip(text.String());
}


static float
BRectRatio(const BRect& rect)
{
	return rect.Width() / rect.Height();
}


static float
BRectHorizontalOverlap(const BRect& original, const BRect& resized)
{
	return ((original.Height() / resized.Height() * resized.Width()) - original.Width()) / 2;
}


static float
BRectVerticalOverlap(const BRect& original, const BRect& resized)
{
	return ((original.Width() / resized.Width() * resized.Height()) - original.Height()) / 2;
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
		BRect destRect;
		BRect bitmapBounds = bitmap->Bounds();
		BRect viewBounds = fBitmapView->Bounds();
		if (BRectRatio(viewBounds) >= BRectRatio(bitmapBounds)) {
			float overlap = BRectHorizontalOverlap(viewBounds, bitmapBounds);
			destRect.Set(-overlap, 0, viewBounds.Width() + overlap,
						viewBounds.Height());
		} else {
			float overlap = BRectVerticalOverlap(viewBounds, bitmapBounds);
			destRect.Set(0, -overlap, viewBounds.Width(), viewBounds.Height() + overlap);
		}
		
		fBitmapView->SetViewBitmap(bitmap,
			bitmap->Bounds().OffsetToCopy(B_ORIGIN),
			destRect,
			B_FOLLOW_TOP|B_FOLLOW_LEFT, B_FILTER_BITMAP_BILINEAR);
		if (Window() != NULL)
			Invalidate();
	}
}


BRect
PreviewView::Rect() const
{
	return fCoordRect;
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
}
	
	
BSize
BitmapView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(90, 70));
}

