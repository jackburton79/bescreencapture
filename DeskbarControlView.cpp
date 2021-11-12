/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "DeskbarControlView.h"

#include "BSCApp.h"
#include "Constants.h"
#include "Controller.h"
#include "ControllerObserver.h"

#include <Bitmap.h>
#include <Deskbar.h>
#include <Entry.h>
#include <MenuItem.h>
#include <Message.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <Roster.h>

#include <stdio.h>
#include <string.h>

enum {
	BSC_STOP,
	BSC_START,
	BSC_PAUSE,
	BSC_RESUME
};

const float kContentSpacingHorizontal = 8;
const float kContentIconMinSize = 12;
const float kContentIconPad = 4;

class BSCMenuItem : public BMenuItem {
public:
	BSCMenuItem(uint32 action, BMessage *message);
	virtual void DrawContent();
	virtual void GetContentSize(float* width, float* height);
private:
	const char *ActionToString(uint32 action);
	
	uint32 fAction;
	BBitmap *fMenuIcon;
};


const static char* kControllerMessengerName = "controller_messenger";


DeskbarControlView::DeskbarControlView(BRect rect, const char *name)
	:
	BView(rect, name, B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW|B_PULSE_NEEDED)
{
	InitData();
	
	fAppMessenger = BMessenger(kAppSignature);
	fControllerMessenger = BMessenger(gControllerLooper);
}


DeskbarControlView::DeskbarControlView(BMessage *data)
	:
	BView(data)
{
	InitData();
	
	fAppMessenger = BMessenger(kAppSignature);
	data->FindMessenger(kControllerMessengerName, &fControllerMessenger);
}


DeskbarControlView::~DeskbarControlView()
{
	delete fBitmap;
}


DeskbarControlView*
DeskbarControlView::Instantiate(BMessage *archive)
{
	if (!validate_instantiation(archive, "DeskbarControlView"))
		return NULL;
	
	return new DeskbarControlView(archive);
}


/* virtual */
status_t
DeskbarControlView::Archive(BMessage *message, bool deep) const
{
	status_t status = BView::Archive(message, deep);
	if (status != B_OK)
		return status;
	
	status = message->AddString("add_on", kAppSignature);
	if (status != B_OK)
		return status;
	
	status = message->AddMessenger(kControllerMessengerName,
		fControllerMessenger);
	return status;
}


/* virtual */
void
DeskbarControlView::AttachedToWindow()
{
	SetViewColor(Parent()->ViewColor());
		
	if (LockLooper()) {
		StartWatching(fControllerMessenger, kMsgControllerCaptureStarted);
		StartWatching(fControllerMessenger, kMsgControllerCaptureStopped);
		StartWatching(fControllerMessenger, kMsgControllerCapturePaused);	
		StartWatching(fControllerMessenger, kMsgControllerCaptureResumed);
		UnlockLooper();
	}
}


/* virtual */
void
DeskbarControlView::DetachedFromWindow()
{
	if (LockLooper()) {
		StopWatching(fControllerMessenger, kMsgControllerCaptureStarted);
		StopWatching(fControllerMessenger, kMsgControllerCaptureStopped);	
		StopWatching(fControllerMessenger, kMsgControllerCapturePaused);	
		StopWatching(fControllerMessenger, kMsgControllerCaptureResumed);	
		UnlockLooper();
	}
}


/* virtual */
void
DeskbarControlView::Draw(BRect rect)
{
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fBitmap);
	
	if (fPaused) {
		SetDrawingMode(B_OP_COPY);
		SetHighColor(kBlack);
		FillRect(BRect(2, 12, 5, 15));
	} else if (fRecording) {
		SetDrawingMode(B_OP_COPY);
		SetHighColor(kRed);
		FillRect(BRect(2, 12, 5, 15));
	}
}


/* virtual */
void
DeskbarControlView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgGUIToggleCapture:
			if (fControllerMessenger.IsValid())
				fControllerMessenger.SendMessage(message);
			break;
		case kMsgGUITogglePause:
			if (fControllerMessenger.IsValid())
				fControllerMessenger.SendMessage(message->what);
			break;
		
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32("be:observe_change_what", &code);
			switch (code) {			
				case kMsgControllerCaptureStarted:
					fRecording = true;
					Invalidate();
					break;
				
				case kMsgControllerCaptureStopped:
					fRecording = false;
					fPaused = false;
					Invalidate();
					break;
				
				case kMsgControllerCapturePaused:
				case kMsgControllerCaptureResumed:
					fPaused = code == kMsgControllerCapturePaused;
					Invalidate();
					break;
					
				default:
					break;
			}
		}					
		default:
			BView::MessageReceived(message);
			break;
	}
}


/* virtual */
void
DeskbarControlView::MouseDown(BPoint where)
{
	BPopUpMenu *menu = new BPopUpMenu("menu");
	if (fRecording) {
		menu->AddItem(new BSCMenuItem(BSC_STOP, new BMessage(kMsgGUIToggleCapture)));
		if (fPaused)
			menu->AddItem(new BSCMenuItem(BSC_RESUME, new BMessage(kMsgGUITogglePause)));
		else
			menu->AddItem(new BSCMenuItem(BSC_PAUSE, new BMessage(kMsgGUITogglePause)));
	} else
		menu->AddItem(new BSCMenuItem(BSC_START, new BMessage(kMsgGUIToggleCapture)));
	
	menu->SetTargetForItems(this);
	
	ConvertToScreen(&where);
	menu->Go(where, true, false, true);
}


/* virtual */
void
DeskbarControlView::Pulse()
{
	if (!fControllerMessenger.IsValid()) {
		snooze(100000);
		BDeskbar deskbar;
		if (deskbar.IsRunning())
			deskbar.RemoveItem(BSC_DESKBAR_VIEW);
	}
}


void
DeskbarControlView::InitData()
{
	fBitmap = NULL;
	fRecording = false;
	fPaused = false;
	
	app_info info;
	be_roster->GetAppInfo(kAppSignature, &info);
	
	fBitmap = new BBitmap(BRect(0, 0, 15, 15), B_RGBA32);
	BNodeInfo::GetTrackerIcon(&info.ref, fBitmap, B_MINI_ICON);
}


BSCMenuItem::BSCMenuItem(uint32 action, BMessage *message)
	:
	BMenuItem(ActionToString(action), message),
	fAction(action),
	fMenuIcon(NULL)
{
}


/* virtual */
void
BSCMenuItem::GetContentSize(float* width, float* height)
{
	font_height fontHeight;
	Menu()->GetFontHeight(&fontHeight);
	float fullHeight = fontHeight.ascent + fontHeight.descent;
	float stringWidth = Menu()->StringWidth("Resume Recording");
	
	if (width != NULL)
		*width = stringWidth + kContentSpacingHorizontal + max_c(kContentIconMinSize, fullHeight);
	if (height != NULL)
		*height = max_c(fullHeight + kContentIconPad, kContentIconMinSize + kContentIconPad);
}


/* virtual */
void
BSCMenuItem::DrawContent()
{
	font_height fontHeight;
	Menu()->GetFontHeight(&fontHeight);
	float fullHeight = fontHeight.ascent + fontHeight.descent + fontHeight.leading;
	BPoint drawPoint(ContentLocation());
	drawPoint.x += fullHeight + kContentSpacingHorizontal;
	drawPoint.y += ((Frame().Height() - fullHeight) / 2) - 1;
	
	float iconSize = max_c(kContentIconMinSize, fullHeight);
	BRect imageRect;
	imageRect.SetLeftTop(ContentLocation());		
	imageRect.top += kContentIconPad / 2;
	imageRect.right = imageRect.left + iconSize;
	imageRect.bottom = imageRect.top + iconSize;
			
	switch (fAction) {
		case BSC_START:
		{
			Menu()->MovePenTo(drawPoint);
			BMenuItem::DrawContent();
			Menu()->SetHighColor(kRed);
			Menu()->FillEllipse(imageRect);
			break;
		}
		case BSC_STOP:
		{
			Menu()->MovePenTo(drawPoint);
			BMenuItem::DrawContent();	
			Menu()->SetHighColor(kRed);
			Menu()->FillRect(imageRect);
			break;
		}
		case BSC_PAUSE:
		{
			Menu()->MovePenTo(drawPoint);
			BMenuItem::DrawContent();
			
			float stripWidth = 4;
			BRect stripRect = imageRect;
			stripRect.left += 1;
			stripRect.right = stripRect.left + stripWidth;
			
			Menu()->SetHighColor(kBlack);
			Menu()->FillRect(stripRect);
			stripRect.OffsetBy(imageRect.Width() - stripWidth - 2, 0);
			Menu()->FillRect(stripRect);
			break;
		}
		case BSC_RESUME:
		{
			Menu()->MovePenTo(drawPoint);
			BMenuItem::DrawContent();
		
			BPoint ptOne = ContentLocation();
			BPoint ptTwo = ptOne;
			ptTwo.y = Frame().bottom - 2;
			BPoint ptThree = ptOne;
			ptThree.x += max_c(kContentIconMinSize, fullHeight);
			ptThree.y += (ptTwo.y - ptOne.y) / 2;			

			Menu()->SetHighColor(kBlack);
			Menu()->FillTriangle(ptOne, ptTwo, ptThree);
			break;
		}	
		default:
			BMenuItem::DrawContent();
			break;
	}
}


const char *
BSCMenuItem::ActionToString(uint32 action)
{
	switch (action) {
		case BSC_START:
			return "Start recording";
		case BSC_STOP:
			return "Stop recording";
		case BSC_PAUSE:
			return "Pause Recording";
		case BSC_RESUME:
			return "Resume Recording";
		default:
			return "";
	}
}
