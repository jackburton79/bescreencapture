#include "BSCApp.h"
#include "Controller.h"
#include "ControllerObserver.h"
#include "DeskbarControlView.h"
#include "messages.h"

#include <Bitmap.h>
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
	BSC_PAUSE_RESUME
};

class BSCMenuItem : public BMenuItem {
public:
	BSCMenuItem(uint32 action, BMessage *message);
	virtual void DrawContent();

private:
	const char *ActionToString(uint32 action);
	
	uint32 fAction;
	BBitmap *fMenuIcon;
};


const static char* kControllerMessengerName = "controller_messenger";


DeskbarControlView::DeskbarControlView(BRect rect, const char *name)
	:
	BView(rect, name, B_FOLLOW_LEFT|B_FOLLOW_TOP, B_WILL_DRAW)
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
	delete fRecordingBitmap;
}


DeskbarControlView*
DeskbarControlView::Instantiate(BMessage *archive)
{
	if (!validate_instantiation(archive, "DeskbarControlView"))
		return NULL;
	
	return new DeskbarControlView(archive);
}


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


void
DeskbarControlView::Draw(BRect rect)
{
	SetDrawingMode(B_OP_OVER);
	DrawBitmap(fBitmap);
	
	if (fPaused) {
		SetDrawingMode(B_OP_COPY);
		SetHighColor(124, 124, 0);
		FillRect(BRect(2, 12, 5, 15));
	} else if (fRecording) {
		SetDrawingMode(B_OP_COPY);
		SetHighColor(250, 0, 0);
		FillRect(BRect(2, 12, 5, 15));
	}
}


void
DeskbarControlView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgGUIStartCapture:
		case kMsgGUIStopCapture:
			if (fControllerMessenger.IsValid())
				fControllerMessenger.SendMessage(message);
			break;
		case kPauseResumeCapture:
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


void
DeskbarControlView::MouseDown(BPoint where)
{
	BPopUpMenu *menu = new BPopUpMenu("menu");
	if (fRecording) {
		menu->AddItem(new BSCMenuItem(BSC_STOP, new BMessage(kMsgGUIStopCapture)));
		menu->AddItem(new BSCMenuItem(BSC_PAUSE_RESUME, new BMessage(kPauseResumeCapture)));	
	} else
		menu->AddItem(new BSCMenuItem(BSC_START, new BMessage(kMsgGUIStartCapture)));
	
	menu->SetTargetForItems(this);
	
	ConvertToScreen(&where);
	menu->Go(where, true, false, true);
}


void
DeskbarControlView::InitData()
{
	fBitmap = NULL;
	fRecordingBitmap = NULL;
	fRecording = false;
	fPaused = false;
	
	app_info info;
	be_roster->GetAppInfo(kAppSignature, &info);
	
	fBitmap = new BBitmap(BRect(0, 0, 15, 15), B_COLOR_8_BIT);
	BNodeInfo::GetTrackerIcon(&info.ref, fBitmap, B_MINI_ICON);
}


BSCMenuItem::BSCMenuItem(uint32 action, BMessage *message)
	:
	BMenuItem(ActionToString(action), message),
	fAction(action),
	fMenuIcon(NULL)
{
	
}


void
BSCMenuItem::DrawContent()
{
	Menu()->SetFontSize(10);
	if (fAction == BSC_START) {
		BPoint drawPoint(ContentLocation());
		drawPoint.x += 20;
		Menu()->MovePenTo(drawPoint);
		BMenuItem::DrawContent();
	
		BRect imageRect;
		imageRect.SetLeftTop(ContentLocation());
	
		imageRect.top += 2;
		imageRect.right = imageRect.left + 10;
		imageRect.bottom = Frame().bottom - 2;
	
		Menu()->SetHighColor(248, 0, 0);
		Menu()->FillEllipse(imageRect);
	} else
		BMenuItem::DrawContent();
}


const char *
BSCMenuItem::ActionToString(uint32 action)
{
	switch (action) {
		case BSC_START:
			return "Start recording";
		case BSC_STOP:
			return "Stop recording";
		case BSC_PAUSE_RESUME:
			return "Pause/Resume";
		default:
			return "No Action";
	}
}
