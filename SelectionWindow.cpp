/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "SelectionWindow.h"

#include "Constants.h"
#include "Settings.h"
#include "Utils.h"

#include <Bitmap.h>
#include <Cursor.h>
#include <Screen.h>
#include <String.h>


const char *kInfoRegionMode = "Click and drag to select, press ENTER to confirm";
const char *kInfoWindowMode = "Click to select a window";

const static rgb_color kSelectionColor = { 0, 0, 128, 100 };

const static float kDraggerSize = 16;
const static float kDraggerSpacing = 4;
const static float kDraggerFullSize = kDraggerSize + kDraggerSpacing;

class SelectionView : public BView {
public:
	SelectionView(BRect frame, const char *name, const char* text = NULL);

	virtual void Draw(BRect updateRect);
	virtual BRect SelectionRect() const;

protected:
	BString fText;
};


class SelectionViewRegion : public SelectionView {
public:
	enum drag_mode {
		DRAG_MODE_NONE = 0,
		DRAG_MODE_SELECT = 1,
		DRAG_MODE_MOVE = 2,
		DRAG_MODE_RESIZE_LEFT_TOP = 3,
		DRAG_MODE_RESIZE_RIGHT_TOP = 4,
		DRAG_MODE_RESIZE_LEFT_BOTTOM = 5,
		DRAG_MODE_RESIZE_RIGHT_BOTTOM = 6,
	};

	enum which_dragger {
		DRAGGER_NONE = -1,
		DRAGGER_LEFT_TOP = 0,
		DRAGGER_RIGHT_TOP = 1,
		DRAGGER_LEFT_BOTTOM = 2,
		DRAGGER_RIGHT_BOTTOM = 3
	};

	SelectionViewRegion(BRect frame, const char *name);
	virtual ~SelectionViewRegion();

	virtual void MouseDown(BPoint where);
	virtual void MouseUp(BPoint where);
	virtual void MouseMoved(BPoint where, uint32 code, const BMessage *message);
	virtual void KeyDown(const char* bytes, int32 numBytes);
	virtual void Draw(BRect updateRect);

	virtual BRect SelectionRect() const;

	BRect LeftTopDragger() const;
	BRect RightTopDragger() const;
	BRect LeftBottomDragger() const;
	BRect RightBottomDragger() const;

private:
	void _DrawDraggers();
	int _MouseOnDragger(BPoint where) const;

	BPoint fSelectionStart;
	BPoint fSelectionEnd;
	BPoint fCurrentMousePosition;
	BRect fStringRect;
	int fDragMode;
	BCursor* fCursorNWSE;
	BCursor* fCursorNESW;
	BCursor* fCursorGrab;
	BCursor* fCursorSelect;
};


class SelectionViewWindow : public SelectionView {
public:
	SelectionViewWindow(BRect frame, const char* name);

	virtual void MouseMoved(BPoint where, uint32 code, const BMessage *message);
	virtual void MouseUp(BPoint where);
	virtual void Draw(BRect updateRect);

	virtual BRect SelectionRect() const;

private:
	BObjectList<BRect> fFrameList;
	BRect fHighlightFrame;

	BRect HitTestFrame(BPoint& where);
};


// SelectionView
SelectionView::SelectionView(BRect frame, const char *name, const char* text)
	:
	BView(frame, name, B_FOLLOW_NONE, B_WILL_DRAW),
	fText(text)
{
	SetFontSize(30);
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
SelectionView::SelectionRect() const
{
	return BRect(0, 0, -1, -1);
}


// SelectionViewRegion
SelectionViewRegion::SelectionViewRegion(BRect frame, const char *name)
	:
	SelectionView(frame, name, kInfoRegionMode),
	fSelectionStart(-1, -1),
	fSelectionEnd(-1, -1),
	fCursorNWSE(NULL),
	fCursorNESW(NULL),
	fCursorGrab(NULL),
	fCursorSelect(NULL)
{
	fDragMode = DRAG_MODE_NONE;
	fCursorNWSE = new BCursor(B_CURSOR_ID_RESIZE_NORTH_WEST_SOUTH_EAST);
	fCursorNESW = new BCursor(B_CURSOR_ID_RESIZE_NORTH_EAST_SOUTH_WEST);
	fCursorGrab = new BCursor(B_CURSOR_ID_GRABBING);
	fCursorSelect = new BCursor(B_CURSOR_ID_CROSS_HAIR);
}


SelectionViewRegion::~SelectionViewRegion()
{
	delete fCursorNWSE;
	delete fCursorNESW;
	delete fCursorGrab;
	delete fCursorSelect;
}


void
SelectionViewRegion::MouseDown(BPoint where)
{
	SelectionView::MouseDown(where);

	if (SelectionRect().Contains(where))
		fDragMode = DRAG_MODE_MOVE;
	else {
		switch (_MouseOnDragger(where)) {
			case DRAGGER_LEFT_TOP:
				fDragMode = DRAG_MODE_RESIZE_LEFT_TOP;
				break;
			case DRAGGER_RIGHT_TOP:
				fDragMode = DRAG_MODE_RESIZE_RIGHT_TOP;
				break;
			case DRAGGER_LEFT_BOTTOM:
				fDragMode = DRAG_MODE_RESIZE_LEFT_BOTTOM;
				break;
			case DRAGGER_RIGHT_BOTTOM:
				fDragMode = DRAG_MODE_RESIZE_RIGHT_BOTTOM;
				break;
			default:
				Invalidate();
				fDragMode = DRAG_MODE_SELECT;
				fSelectionStart = where;
				fSelectionEnd = where;
				break;
		}
	}
}


void
SelectionViewRegion::MouseMoved(BPoint where, uint32 code, const BMessage *message)
{
	if (fDragMode != DRAG_MODE_NONE) {
		BRect selectionRect = SelectionRect();
		float xOffset = where.x - fCurrentMousePosition.x;
		float yOffset = where.y - fCurrentMousePosition.y;
		switch (fDragMode) {
			case DRAG_MODE_SELECT:
			{
				SetViewCursor(fCursorSelect);
				fSelectionEnd = where;
				break;
			}
			case DRAG_MODE_MOVE:
			{
				SetViewCursor(fCursorGrab);
				fSelectionStart.x += xOffset;
				fSelectionStart.y += yOffset;
				fSelectionEnd.x += xOffset;
				fSelectionEnd.y += yOffset;
				break;
			}
			case DRAG_MODE_RESIZE_LEFT_TOP:
			{
				SetViewCursor(fCursorNWSE);
				fSelectionStart.x += xOffset;
				fSelectionStart.y += yOffset;
				break;
			}
			case DRAG_MODE_RESIZE_RIGHT_TOP:
			{
				SetViewCursor(fCursorNESW);
				fSelectionEnd.x += xOffset;
				fSelectionStart.y += yOffset;
				break;
			}
			case DRAG_MODE_RESIZE_LEFT_BOTTOM:
			{
				SetViewCursor(fCursorNESW);
				fSelectionStart.x += xOffset;
				fSelectionEnd.y += yOffset;
				break;
			}
			case DRAG_MODE_RESIZE_RIGHT_BOTTOM:
			{
				SetViewCursor(fCursorNWSE);
				fSelectionEnd.x += xOffset;
				fSelectionEnd.y += yOffset;
				break;
			}
			default:
				SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);
				break;
		}

		BRect newSelection = SelectionRect();
		BRect invalidateRect = (selectionRect | newSelection);
		invalidateRect.InsetBySelf(-(kDraggerSize + kDraggerSpacing), -(kDraggerSize + kDraggerSpacing));
		Invalidate(invalidateRect);
	} else
		SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);

	SelectionView::MouseMoved(where, code, message);

	fCurrentMousePosition = where;
}


void
SelectionViewRegion::MouseUp(BPoint where)
{
	SetViewCursor(B_CURSOR_SYSTEM_DEFAULT);

	if (fDragMode == DRAG_MODE_SELECT)
		fSelectionEnd = where;

	fDragMode = DRAG_MODE_NONE;

	// Sanitize the selection rect (fSelectionStart < fSelectionEnd)
	BRect selectionRect = SelectionRect();
	fSelectionStart = selectionRect.LeftTop();
	fSelectionEnd = selectionRect.RightBottom();

	SelectionView::MouseUp(where);
}


void
SelectionViewRegion::KeyDown(const char* bytes, int32 numBytes)
{
	switch (bytes[0]) {
		case B_ENTER:
			Window()->PostMessage(B_QUIT_REQUESTED);
			break;
		case B_ESCAPE:
			fSelectionStart = BPoint(-1, -1);
			fSelectionEnd = BPoint(-1, -1);
			Window()->PostMessage(B_QUIT_REQUESTED);
			break;
		default:
			SelectionView::KeyDown(bytes, numBytes);
			break;
	}
}


void
SelectionViewRegion::Draw(BRect updateRect)
{
	SelectionView::Draw(updateRect);

	if (SelectionRect().IsValid()) {
		BRect selection = SelectionRect();
		SetDrawingMode(B_OP_ALPHA);
		SetHighColor(kSelectionColor);
		FillRect(selection);
		BString sizeString;
		sizeString << selection.IntegerWidth() << " x " << selection.IntegerHeight();
		float stringWidth = StringWidth(sizeString.String());
		BPoint position;
		position.x = (selection.Width() - stringWidth) / 2;
		position.y = selection.Height() / 2;
		position += selection.LeftTop();
		SetHighColor(kBlack);
		SetLowColor(kBlack);
		DrawString(sizeString.String(), position);
	}

	_DrawDraggers();
}


BRect
SelectionViewRegion::SelectionRect() const
{
	if (fSelectionStart == BPoint(-1, -1)
		&& fSelectionEnd == BPoint(-1, -1))
		return BRect(0, 0, -1, -1);

	BRect rect;
	rect.SetLeftTop(BPoint(min_c(fSelectionStart.x, fSelectionEnd.x),
					min_c(fSelectionStart.y, fSelectionEnd.y)));
	rect.SetRightBottom(BPoint(max_c(fSelectionStart.x, fSelectionEnd.x),
					max_c(fSelectionStart.y, fSelectionEnd.y)));
	return rect;
}


BRect
SelectionViewRegion::LeftTopDragger() const
{
	BPoint leftTop(min_c(fSelectionStart.x, fSelectionEnd.x),
					min_c(fSelectionStart.y, fSelectionEnd.y));
	return BRect(leftTop + BPoint(-kDraggerFullSize, -kDraggerFullSize),
			leftTop + BPoint(-kDraggerSpacing, -kDraggerSpacing));
}


BRect
SelectionViewRegion::RightTopDragger() const
{
	BPoint leftTop(max_c(fSelectionStart.x, fSelectionEnd.x),
					min_c(fSelectionStart.y, fSelectionEnd.y));
	return BRect(leftTop + BPoint(kDraggerSpacing, -kDraggerFullSize),
				leftTop + BPoint(kDraggerFullSize, -kDraggerSpacing));
}


BRect
SelectionViewRegion::LeftBottomDragger() const
{
	BPoint leftTop(min_c(fSelectionStart.x, fSelectionEnd.x),
					max_c(fSelectionStart.y, fSelectionEnd.y));
	return BRect(leftTop + BPoint(-kDraggerFullSize, kDraggerSpacing),
				leftTop + BPoint(-kDraggerSpacing, kDraggerFullSize));
}


BRect
SelectionViewRegion::RightBottomDragger() const
{
	BPoint leftTop(max_c(fSelectionStart.x, fSelectionEnd.x),
					max_c(fSelectionStart.y, fSelectionEnd.y));
	return BRect(leftTop + BPoint(kDraggerSpacing, kDraggerSpacing),
				leftTop + BPoint(kDraggerFullSize, kDraggerFullSize));
}


void
SelectionViewRegion::_DrawDraggers()
{
	if (fSelectionStart == BPoint(-1, -1)
		&& fSelectionEnd == BPoint(-1, -1))
		return;

	PushState();
	SetHighColor(0, 0, 0);
	StrokeRect(LeftTopDragger(), B_SOLID_HIGH);
	StrokeRect(LeftBottomDragger(), B_SOLID_HIGH);
	StrokeRect(RightTopDragger(), B_SOLID_HIGH);
	StrokeRect(RightBottomDragger(), B_SOLID_HIGH);
	PopState();
}


int
SelectionViewRegion::_MouseOnDragger(BPoint point) const
{
	if (LeftTopDragger().Contains(point))
		return DRAGGER_LEFT_TOP;
	else if (RightTopDragger().Contains(point))
		return DRAGGER_RIGHT_TOP;
	else if (LeftBottomDragger().Contains(point))
		return DRAGGER_LEFT_BOTTOM;
	else if (RightBottomDragger().Contains(point))
		return DRAGGER_RIGHT_BOTTOM;

	return DRAGGER_NONE;
}


// SelectionViewWindow
SelectionViewWindow::SelectionViewWindow(BRect frame, const char *name)
	:
	SelectionView(frame, name, kInfoWindowMode)
{
	GetWindowsFrameList(fFrameList, Settings::Current().WindowFrameEdgeSize());
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
SelectionViewWindow::MouseUp(BPoint where)
{
	Window()->PostMessage(B_QUIT_REQUESTED);
}


void
SelectionViewWindow::Draw(BRect updateRect)
{
	SelectionView::Draw(updateRect);

	if (fHighlightFrame.IsValid()) {
		SetDrawingMode(B_OP_ALPHA);
		SetHighColor(kSelectionColor);
		FillRect(fHighlightFrame);
	}
}


BRect
SelectionViewWindow::SelectionRect() const
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
	fView->MakeFocus(true);
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
	while (!IsHidden()) {
		Hide();
		snooze(10000);
	}

	BScreen screen(this);
	BMessage message(fCommand);
	BBitmap *bitmap = NULL;
	BRect selection = fView->SelectionRect();
	if (!selection.IsValid())
		selection = screen.Frame();

	FixRect(selection, screen.Frame());

	snooze(10000);

	screen.GetBitmap(&bitmap, false, &selection);

	message.AddRect("selection", selection);
	message.AddPointer("bitmap", bitmap);

	fTarget.SendMessage(&message);

	return BWindow::QuitRequested();
}
