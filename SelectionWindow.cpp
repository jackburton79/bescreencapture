#include "SelectionWindow.h"
#include "Settings.h"
#include "Utils.h"

#include <Bitmap.h>
#include <Message.h>
#include <Screen.h>
#include <String.h>
#include <StringView.h>

#include <cstdio>

const char *kInfoRegionMode = "Click and drag to select, release when finished";
const char *kInfoWindowMode = "Click to select a window";


const static rgb_color kWindowSelectionColor = { 0, 0, 128, 100 };
const static rgb_color kRed = { 240, 0, 0, 0 };

class SelectionView : public BView {
public:
	SelectionView(BRect frame, const char *name, const char* text = NULL);

	virtual void MouseUp(BPoint where);
	virtual void Draw(BRect updateRect);
	virtual BRect SelectionRect();

protected:
	BString fText;
};


class SelectionViewRegion : public SelectionView {
public:
	SelectionViewRegion(BRect frame, const char *name);
	virtual void MouseDown(BPoint where);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 code, const BMessage *message);
	virtual void Draw(BRect updateRect);
	
	virtual BRect SelectionRect();
		
private:
	bool fMouseDown;
	BPoint fSelectionStart;
	BPoint fSelectionEnd;
	
	BRect fStringRect;
	
	void MakeSelectionRect(BRect *rect);
};


class SelectionViewWindow : public SelectionView {
public:
	SelectionViewWindow(BRect frame, const char* name);
	
	virtual void MouseMoved(BPoint where, uint32 code, const BMessage *message);
	virtual void Draw(BRect updateRect);
	
	virtual BRect SelectionRect();
	
private:
	BObjectList<BRect> fFrameList;
	BRect fHighlightFrame;
	
	BRect HitTestFrame(BPoint& where);
};


// SelectionView
SelectionView::SelectionView(BRect frame, const char *name, const char* text)
	:
	BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW)
{
	fText = text;
	SetFontSize(30);	
}


void
SelectionView::MouseUp(BPoint where)
{
	Window()->PostMessage(B_QUIT_REQUESTED);	
}


void
SelectionView::Draw(BRect updateRect)
{
	PushState();
	SetDrawingMode(B_OP_OVER);
	SetHighColor(kRed);
	
	float width = StringWidth(fText.String());
	BPoint point;
	point.x = (Bounds().Width() - width) / 2;
	point.y = Bounds().Height() / 2;
	DrawString(fText.String(), point);
	
	PopState();
}


BRect
SelectionView::SelectionRect()
{
	return BRect(0, 0, -1, -1);
}


// SelectionViewRegion
SelectionViewRegion::SelectionViewRegion(BRect frame, const char *name)
	:
	SelectionView(frame, name, kInfoRegionMode),
	fMouseDown(false)
{
}


void
SelectionViewRegion::MouseDown(BPoint where)
{
	SelectionView::MouseDown(where);
	
	fMouseDown = true;
	fSelectionStart = where;
}


void
SelectionViewRegion::MouseMoved(BPoint where, uint32 code, const BMessage *a_message)
{
	if (fMouseDown && code == B_INSIDE_VIEW) {
		BRect selectionRect;
		MakeSelectionRect(&selectionRect);
	
		BRect newSelection;	
		fSelectionEnd = where;
		MakeSelectionRect(&newSelection);
		
		Invalidate(selectionRect | newSelection);
	}
}


void
SelectionViewRegion::MouseUp(BPoint where)
{
	fSelectionEnd = where;
	fMouseDown = false;
	
	SelectionView::MouseUp(where);
	Window()->PostMessage(B_QUIT_REQUESTED);	
}


void
SelectionViewRegion::Draw(BRect updateRect)
{
	SelectionView::Draw(updateRect);
	
	if (fMouseDown) {
		BRect selection;
		MakeSelectionRect(&selection);
		
		StrokeRect(selection, B_MIXED_COLORS);
	}
}


BRect
SelectionViewRegion::SelectionRect()
{
	BRect rect;
	MakeSelectionRect(&rect);
	return rect;
}


void
SelectionViewRegion::MakeSelectionRect(BRect *rect)
{
	rect->SetLeftTop(BPoint(min_c(fSelectionStart.x, fSelectionEnd.x),
					min_c(fSelectionStart.y, fSelectionEnd.y)));
	rect->SetRightBottom(BPoint(max_c(fSelectionStart.x, fSelectionEnd.x),
					max_c(fSelectionStart.y, fSelectionEnd.y)));
}


// SelectionViewWindow
SelectionViewWindow::SelectionViewWindow(BRect frame, const char *name)
	:
	SelectionView(frame, name, kInfoWindowMode)
{	
	GetWindowsFrameList(fFrameList, Settings().WindowFrameBorderSize());
}


void
SelectionViewWindow::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	BRect frame = HitTestFrame(where);
	if (frame.IsValid())
		fHighlightFrame = frame;
	else
		fHighlightFrame.Set(0, 0, -1, -1);
	Invalidate();
}


void
SelectionViewWindow::Draw(BRect updateRect)
{
	SelectionView::Draw(updateRect);
	
	if (fHighlightFrame.IsValid()) {
		SetDrawingMode(B_OP_ALPHA);
		SetHighColor(kWindowSelectionColor);
		FillRect(fHighlightFrame);
	}
}


BRect
SelectionViewWindow::SelectionRect()
{
	return fHighlightFrame;
}


BRect
SelectionViewWindow::HitTestFrame(BPoint& where)
{
	int32 count = fFrameList.CountItems();
	for (int32 i = 0; i < count; i++) {
		BRect *frame = fFrameList.ItemAt(i);
		if (frame->Contains(where))
			return *frame;
	}
	
	return BRect(0, 0, -1, -1);
}


// SelectionWindow
SelectionWindow::SelectionWindow(int mode, BMessenger& target, uint32 command)
	:
	BWindow(BScreen().Frame(), "Area Window", window_type(1026),
		B_ASYNCHRONOUS_CONTROLS|B_NOT_RESIZABLE|B_NOT_CLOSABLE|B_NOT_ZOOMABLE)
{
	if (mode == REGION)
		fView = new SelectionViewRegion(Bounds(), "Selection view");
	else
		fView = new SelectionViewWindow(Bounds(), "Selection view");
	AddChild(fView);
	
	SetTarget(target);
	SetCommand(command);
}


void
SelectionWindow::Show()
{
	BBitmap *bitmap = NULL;	
	BScreen(this).GetBitmap(&bitmap, false);
	fView->SetViewBitmap(bitmap);
	delete bitmap;
	
	BWindow::Show();	
}


void
SelectionWindow::SetTarget(BMessenger& target)
{
	fTarget = target;
}


void
SelectionWindow::SetCommand(uint32 command)
{
	fCommand = command;	
}


bool
SelectionWindow::QuitRequested()
{
	Hide();
	BScreen screen(this);
	BMessage message(fCommand);
	BBitmap *bitmap = NULL;	
	BRect selection = fView->SelectionRect();
	if (!selection.IsValid())
		selection = screen.Frame();
	FixRect(selection);
	screen.GetBitmap(&bitmap, false, &selection);
	
	snooze(2000);
	message.AddRect("selection", selection);
	message.AddPointer("bitmap", bitmap);
	
	fTarget.SendMessage(&message);
		
	return BWindow::QuitRequested();
}
