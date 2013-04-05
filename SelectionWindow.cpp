#include "SelectionWindow.h"
#include "Utils.h"

#include <Bitmap.h>
#include <Message.h>
#include <Screen.h>
#include <StringView.h>

#include <cstdio>

const char *kInfo = "Select the area to capture";

class SelectionView : public BView {
public:
	SelectionView(BRect frame, const char *name);
	virtual void MouseDown(BPoint where);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 code, const BMessage *message);
	virtual void Draw(BRect updateRect);
	
	BRect SelectionRect();
		
private:
	bool fMouseDown;
	BPoint fSelectionStart;
	BPoint fSelectionEnd;
	
	BRect fStringRect;
	
	void MakeSelectionRect(BRect *rect);
};


SelectionView::SelectionView(BRect frame, const char *name)
	:
	BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW),
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
SelectionView::MouseDown(BPoint where)
{
	fMouseDown = true;
	fSelectionStart = where;
}


void
SelectionView::MouseMoved(BPoint where, uint32 code, const BMessage *a_message)
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
SelectionView::MouseUp(BPoint where)
{
	fSelectionEnd = where;
	fMouseDown = false;
	
	Window()->PostMessage(B_QUIT_REQUESTED);	
}


void
SelectionView::Draw(BRect updateRect)
{
	if (fMouseDown) {
		BRect selection;
		MakeSelectionRect(&selection);
		
		StrokeRect(selection, B_MIXED_COLORS);
	}
}


BRect
SelectionView::SelectionRect()
{
	BRect rect;
	MakeSelectionRect(&rect);
	return rect;
}


void
SelectionView::MakeSelectionRect(BRect *rect)
{
	rect->SetLeftTop(BPoint(min_c(fSelectionStart.x, fSelectionEnd.x),
					min_c(fSelectionStart.y, fSelectionEnd.y)));
	rect->SetRightBottom(BPoint(max_c(fSelectionStart.x, fSelectionEnd.x),
					max_c(fSelectionStart.y, fSelectionEnd.y)));
}


SelectionWindow::SelectionWindow(BMessenger& target, uint32 command)
	:
	BWindow(BScreen().Frame(), "Area Window", window_type(1026),
		B_ASYNCHRONOUS_CONTROLS|B_NOT_RESIZABLE|B_NOT_CLOSABLE|B_NOT_ZOOMABLE)
{
	fView = new SelectionView(Bounds(), "Selection view");
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
	BScreen(this).GetBitmap(&bitmap, false);
	BRect selection = fView->SelectionRect();
	FixRect(selection);
	message.AddRect("selection", selection);
	message.AddPointer("bitmap", bitmap);
	
	fTarget.SendMessage(&message);
	
		
	return BWindow::QuitRequested();
}
