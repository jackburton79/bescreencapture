/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "DeskbarControlView.h"

#include "BSCApp.h"
#include "Constants.h"
#include "ControllerObserver.h"

#include <Bitmap.h>
#include <ControlLook.h>
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

class BSCMenuItem : public BMenuItem {
public:
	BSCMenuItem(const char* label, BMessage *message, BBitmap* bitmap = NULL);
	virtual ~BSCMenuItem();
	virtual void DrawContent();
	virtual void GetContentSize(float* width, float* height);
private:
	const char *ActionToString(uint32 action);
	
	BBitmap *fMenuIcon;
};


DeskbarControlView::DeskbarControlView(BRect rect)
	:
	BView(rect, BSC_DESKBAR_VIEW, B_FOLLOW_TOP|B_FOLLOW_LEFT,
		B_WILL_DRAW|B_PULSE_NEEDED|B_FRAME_EVENTS),
	fAppMessenger(BMessenger(kAppSignature)),
	fBitmap(NULL),
	fRecording(false),
	fPaused(false)
{
	_UpdateBitmap();
}


DeskbarControlView::DeskbarControlView(BMessage *data)
	:
	BView(data),
	fAppMessenger(BMessenger(kAppSignature)),
	fBitmap(NULL),
	fRecording(false),
	fPaused(false)
{
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

	return status;
}


/* virtual */
void
DeskbarControlView::AttachedToWindow()
{
	BView::AttachedToWindow();

	SetViewColor(Parent()->ViewColor());

	if (LockLooper()) {
		StartWatching(fAppMessenger, kMsgControllerCaptureStarted);
		StartWatching(fAppMessenger, kMsgControllerCaptureStopped);
		StartWatching(fAppMessenger, kMsgControllerCaptureProgress);
		StartWatching(fAppMessenger, kMsgControllerCapturePaused);
		StartWatching(fAppMessenger, kMsgControllerCaptureResumed);
		UnlockLooper();
	}
}


/* virtual */
void
DeskbarControlView::DetachedFromWindow()
{
	if (LockLooper()) {
		StopWatching(fAppMessenger, kMsgControllerCaptureStarted);
		StopWatching(fAppMessenger, kMsgControllerCaptureStopped);
		StopWatching(fAppMessenger, kMsgControllerCaptureProgress);
		StopWatching(fAppMessenger, kMsgControllerCapturePaused);	
		StopWatching(fAppMessenger, kMsgControllerCaptureResumed);	
		UnlockLooper();
	}

	BView::DetachedFromWindow();
}


/* virtual */
void
DeskbarControlView::FrameResized(float width, float height)
{
	BView::FrameResized(width, height);

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
		case kMsgGUIToggleCapture:
			if (fAppMessenger.IsValid())
				fAppMessenger.SendMessage(message);
			break;
		case kMsgGUITogglePause:
			if (fAppMessenger.IsValid())
				fAppMessenger.SendMessage(message->what);
			break;
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			switch (code) {			
				case kMsgControllerCaptureStarted:
				case kMsgControllerCaptureProgress:
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
	// maybe we should load the bitmap or resource once,
	// althought the performance hit is undetectable
	BPopUpMenu *menu = new BPopUpMenu("menu");
	if (fRecording) {
		BBitmap* stopBitmap = _LoadIconBitmap("stop_icon");
		menu->AddItem(new BSCMenuItem("Stop recording",
			new BMessage(kMsgGUIToggleCapture), stopBitmap));
		if (fPaused) {
			BBitmap* resumeBitmap = _LoadIconBitmap("resume_icon");
			menu->AddItem(new BSCMenuItem("Resume recording",
				new BMessage(kMsgGUITogglePause), resumeBitmap));
		} else {
			BBitmap* pauseBitmap = _LoadIconBitmap("pause_icon");
			menu->AddItem(new BSCMenuItem("Pause recording",
				new BMessage(kMsgGUITogglePause), pauseBitmap));
		}
	} else {
		BBitmap* bitmap = _LoadIconBitmap("record_icon");
		menu->AddItem(new BSCMenuItem("Start recording",
			new BMessage(kMsgGUIToggleCapture), bitmap));
	}
	menu->SetTargetForItems(this);
	
	ConvertToScreen(&where);
	menu->Go(where, true, false, true);
}


/* virtual */
void
DeskbarControlView::Pulse()
{
	if (!fAppMessenger.IsValid()) {
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
	if (be_roster->GetAppInfo(kAppSignature, &info) != B_OK)
		return;

	BResources resources(&info.ref);
	if (resources.InitCheck() != B_OK)
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
	if (be_roster->GetAppInfo(kAppSignature, &info) != B_OK)
		return NULL;
	BResources resources(&info.ref);
	BBitmap* bitmap = NULL;
	if (resources.InitCheck() == B_OK) {
		size_t size;
		const void* data = resources.LoadResource(B_VECTOR_ICON_TYPE, iconName, &size);
		if (data != NULL) {
			BRect bitmapRect(BPoint(0, 0),
				be_control_look->ComposeIconSize(B_MINI_ICON));
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
BSCMenuItem::BSCMenuItem(const char* label, BMessage *message, BBitmap* bitmap)
	:
	BMenuItem(label, message),
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
	BMenuItem::GetContentSize(width, height);
	if (fMenuIcon == NULL)
		return;

	const float limit = ceilf(fMenuIcon->Bounds().Height() +
		be_control_look->DefaultLabelSpacing() / 3.0f);

	if (height != NULL) {
		if (*height < limit)
			*height = limit;
	}

	if (width != NULL) {
		*width += fMenuIcon->Bounds().Width()
			+ be_control_look->DefaultLabelSpacing();
	}
}


/* virtual */
void
BSCMenuItem::DrawContent()
{
	BMenu* menu = Menu();
	if (fMenuIcon != NULL) {
		BPoint iconLocation = ContentLocation();
		BRect frame = Frame();
		iconLocation.y = frame.top
			+ (frame.bottom - frame.top - fMenuIcon->Bounds().Height()) / 2;
		menu->PushState();
		menu->SetDrawingMode(B_OP_ALPHA);
		menu->DrawBitmap(fMenuIcon, iconLocation);
		menu->PopState();
	}
	// Text
	BPoint textLocation = ContentLocation();
	textLocation.x += ceilf(be_control_look->DefaultLabelSpacing() * 3.3f);
	menu->MovePenTo(textLocation);
	BMenuItem::DrawContent();
}


extern "C" BView*
instantiate_deskbar_item(float maxWidth, float maxHeight)
{
	return new DeskbarControlView(BRect(0, 0, maxHeight - 1, maxHeight - 1));
}
