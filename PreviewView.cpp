#include "PreviewView.h"

#include <Bitmap.h>
#include <GroupLayout.h>
#include <LayoutBuilder.h>
#include <Screen.h>
#include <String.h>


class BitmapView : public BView {
public:
	BitmapView();
	virtual ~BitmapView();
	
	virtual void Draw(BRect rect);
};


PreviewView::PreviewView()
	:
	BView("Rect View", B_WILL_DRAW),
	fBitmapView(NULL),
	fTimeStamp(0),
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
	if (Window() == NULL || Window()->IsHidden())
		return;

	if (rect != NULL)
		_SetRect(*rect);

	bigtime_t now = system_time();
	if (bitmap == NULL) {
		// Avoid updating preview too often
		if (fTimeStamp + 50000 >= now)
			return;
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
		fTimeStamp = now;
		fBitmapView->SetViewBitmap(bitmap,
			bitmap->Bounds().OffsetToCopy(B_ORIGIN),
			destRect,
			B_FOLLOW_TOP|B_FOLLOW_LEFT, B_FILTER_BITMAP_BILINEAR);
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
