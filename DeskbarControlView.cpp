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
#include <IconUtils.h>
#include <MenuItem.h>
#include <Message.h>
#include <NodeInfo.h>
#include <PopUpMenu.h>
#include <Resources.h>
#include <Roster.h>

#include <syslog.h>
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
	BSCMenuItem(uint32 action, BMessage *message, BBitmap* bitmap = NULL);
	virtual ~BSCMenuItem();
	virtual void DrawContent();
	virtual void GetContentSize(float* width, float* height);
private:
	const char *ActionToString(uint32 action);
	
	uint32 fAction;
	BBitmap *fMenuIcon;
};


const static char* kControllerMessengerName = "controller_messenger";


DeskbarControlView::DeskbarControlView(BRect rect)
	:
	BView(rect, BSC_DESKBAR_VIEW, B_FOLLOW_TOP|B_FOLLOW_LEFT,
		B_WILL_DRAW|B_PULSE_NEEDED|B_FRAME_EVENTS),
	fBitmap(NULL),
	fRecording(false),
	fPaused(false)
{
	fAppMessenger = BMessenger(kAppSignature);
	fControllerMessenger = BMessenger(gControllerLooper);
	_UpdateBitmap();
}


DeskbarControlView::DeskbarControlView(BMessage *data)
	:
	BView(data),
	fBitmap(NULL),
	fRecording(false),
	fPaused(false)
{
	fAppMessenger = BMessenger(kAppSignature);
	data->FindMessenger(kControllerMessengerName, &fControllerMessenger);
	_UpdateBitmap();
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
	BView::AttachedToWindow();

	SetViewColor(Parent()->ViewColor());

	if (!fControllerMessenger.IsValid()) {
		// ask the be_app to send us the controller looper messenger
		BMessage message(kMsgGetControllerMessenger);
		fAppMessenger.SendMessage(&message, this);
		return;
	}

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
DeskbarControlView::FrameResized(float width, float height)
{
	_UpdateBitmap();
	Invalidate();
}


/* virtual */
void
DeskbarControlView::Draw(BRect rect)
{
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fBitmap);
	
	BRect overlayRect = Bounds();
	overlayRect.left = 2;
	overlayRect.right = overlayRect.left + 3;
	overlayRect.top = overlayRect.bottom - 3;
	if (fPaused) {
		SetDrawingMode(B_OP_COPY);
		SetHighColor(kBlack);
		FillRect(overlayRect);
	} else if (fRecording) {
		SetDrawingMode(B_OP_COPY);
		SetHighColor(kRed);
		FillRect(overlayRect);
	}
}


/* virtual */
void
DeskbarControlView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgGetControllerMessenger:
		{
			// this is the reply from the be_app
			if (message->FindMessenger("ControllerMessenger",
					&fControllerMessenger) == B_OK
					&& fControllerMessenger.IsValid()) {
				StartWatching(fControllerMessenger, kMsgControllerCaptureStarted);
				StartWatching(fControllerMessenger, kMsgControllerCaptureStopped);
				StartWatching(fControllerMessenger, kMsgControllerCapturePaused);
				StartWatching(fControllerMessenger, kMsgControllerCaptureResumed);
			}
			break;
		}
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
	// TODO: this is done on every MouseDown: not nice
	// maybe we should load the bitmap or resource once
	BPopUpMenu *menu = new BPopUpMenu("menu");
	if (fRecording) {
		BBitmap* stopBitmap = _LoadIconBitmap("stop_icon");
		menu->AddItem(new BSCMenuItem(BSC_STOP, new BMessage(kMsgGUIToggleCapture), stopBitmap));
		if (fPaused) {
			BBitmap* resumeBitmap = _LoadIconBitmap("resume_icon");
			menu->AddItem(new BSCMenuItem(BSC_RESUME, new BMessage(kMsgGUITogglePause), resumeBitmap));
		} else {
			BBitmap* pauseBitmap = _LoadIconBitmap("pause_icon");
			menu->AddItem(new BSCMenuItem(BSC_PAUSE, new BMessage(kMsgGUITogglePause), pauseBitmap));
		}
	} else {
		BBitmap* bitmap = _LoadIconBitmap("record_icon");
		menu->AddItem(new BSCMenuItem(BSC_START, new BMessage(kMsgGUIToggleCapture), bitmap));
	}
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
DeskbarControlView::_UpdateBitmap()
{
	app_info info;
	be_roster->GetAppInfo(kAppSignature, &info);
	
	BResources resources(&info.ref);
	if (resources.InitCheck() < B_OK)
		return;

	size_t size;
	const void* data = resources.LoadResource(B_VECTOR_ICON_TYPE, 1, &size);
	if (data != NULL) {
		BBitmap* bitmap = new BBitmap(Bounds(), B_RGBA32);
		if (bitmap->InitCheck() == B_OK && BIconUtils::GetVectorIcon(
				(const uint8*)data, size, bitmap) == B_OK) {
			delete fBitmap;
			fBitmap = bitmap;
		}
	}
}


BBitmap*
DeskbarControlView::_LoadIconBitmap(const char* iconName)
{
	app_info info;
	be_roster->GetAppInfo(kAppSignature, &info);
	BResources resources(&info.ref);
	BBitmap* bitmap = NULL;
	if (resources.InitCheck() == B_OK) {
		size_t size;
		const void* data = resources.LoadResource(B_VECTOR_ICON_TYPE, iconName, &size);
		if (data != NULL) {
			BRect bitmapRect(0, 0, 47, 47);
			bitmap = new BBitmap(bitmapRect, B_RGBA32);
			if (bitmap->InitCheck() != B_OK || BIconUtils::GetVectorIcon(
					(const uint8*)data, size, bitmap) != B_OK) {
				delete bitmap;
				bitmap = NULL;
			}
		}
	}
	return bitmap;
}


// BSCMenuItem
BSCMenuItem::BSCMenuItem(uint32 action, BMessage *message, BBitmap* bitmap)
	:
	BMenuItem(ActionToString(action), message),
	fAction(action),
	fMenuIcon(bitmap)
{
}


/* virtual */
BSCMenuItem::~BSCMenuItem()
{
	delete fMenuIcon;
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

	Menu()->MovePenTo(drawPoint);
	BMenuItem::DrawContent();
	if (fMenuIcon != NULL)
		Menu()->DrawBitmap(fMenuIcon, imageRect);
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


extern "C" BView*
instantiate_deskbar_item(float maxWidth, float maxHeight)
{
	return new DeskbarControlView(BRect(0, 0, maxHeight - 1, maxHeight - 1));
}
