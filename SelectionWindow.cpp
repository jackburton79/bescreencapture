#include "SelectionWindow.h"
#include "Settings.h"
#include "Utils.h"

#include <Bitmap.h>
#include <Message.h>
#include <Screen.h>
#include <StringView.h>

#include <cstdio>

const char *kInfo = "Select the area to capture";
const static rgb_color sWindowSelectionColor = { 0, 0, 128, 100 };

class SelectionView : public BView {
public:
	SelectionView(BRect frame, const char *name);

	virtual void MouseUp(BPoint where);
	
	virtual BRect SelectionRect();
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
SelectionView::SelectionView(BRect frame, const char *name)
	:
	BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW)
{
	SetFontSize(20);
}


void
SelectionView::MouseUp(BPoint where)
{
	Window()->PostMessage(B_QUIT_REQUESTED);	
}


BRect
SelectionView::SelectionRect()
{
	return BRect(0, 0, -1, -1);
}


// SelectionViewRegion
SelectionViewRegion::SelectionViewRegion(BRect frame, const char *name)
	:
	SelectionView(frame, name),
	fMouseDown(false)
{
	SetFontSize(20);
	
	float stringWidth = StringWidth(kInfo);
	font_height height;
	GetFontHeight(&height);
	float stringHeight = height.ascent + height.descent + height.leading;
	
	fStringRect.left = (Bounds().Width() - stringWidth) / 2;
	fStringRect.top = (Bounds().Height() - stringHeight) / 2;
	fStringRect.right = fStringRect.left + stringWidth;
	fStringRect.bottom = fStringRect.top + stringHeight;
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
	SelectionView(frame, name)
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
	if (fHighlightFrame.IsValid()) {
		SetDrawingMode(B_OP_ALPHA);
		SetHighColor(sWindowSelectionColor);
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
	BMessage message(fCommand);
	BBitmap *bitmap = NULL;	
	BRect selection = fView->SelectionRect();
	FixRect(selection);
	BScreen(this).GetBitmap(&bitmap, false, &selection);
	
	snooze(2000);
	message.AddRect("selection", selection);
	message.AddPointer("bitmap", bitmap);
	
	fTarget.SendMessage(&message);
		
	return BWindow::QuitRequested();
}
