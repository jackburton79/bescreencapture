#include "CamStatusView.h"
#include "Controller.h"
#include "ControllerObserver.h"
#include "messages.h"

#include <LayoutUtils.h>
#include <Message.h>
#include <Window.h>

#include <stdio.h>

CamStatusView::CamStatusView(Controller* controller)
	:
	BView("CamStatusView", B_WILL_DRAW|B_PULSE_NEEDED),
	fController(controller),
	fNumFrames(0),
	fRecording(false),
	fPaused(false)
{
}


void
CamStatusView::AttachedToWindow()
{
	if (fController->LockLooper()) {
		fController->StartWatching(this, kMsgControllerCaptureStarted);
		fController->StartWatching(this, kMsgControllerCaptureStopped);
		fController->StartWatching(this, kMsgControllerCapturePaused);
		fController->StartWatching(this, kMsgControllerCaptureResumed);
		fController->UnlockLooper();
	}
	
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
CamStatusView::Draw(BRect updateRect)
{
	BRect bounds = Bounds();
	bounds.InsetBy(20, 20);
	
	if (fRecording) {
		if (fPaused) {
			SetHighColor(0, 240, 0);
			BRect one(2, 1, bounds.right / 2 - 2, bounds.bottom - 1);
			FillRect(one);
			one.OffsetTo(bounds.Width() - one.Width() - 2, 1);
			FillRect(one);
		} else {
			SetHighColor(248, 0, 0);
			FillEllipse(bounds);
		}
		SetHighColor(0, 0, 0);
		SetDrawingMode(B_OP_OVER);
		BString string;
		string << fNumFrames;
		float width = StringWidth(string);
		font_height height;
		GetFontHeight(&height);	
		BPoint point((bounds.Width() - width) / 2 + bounds.left,
					(bounds.Height() - (height.ascent + height.descent)) + bounds.top + 1);
		DrawString(string, point);
	}
}


void
CamStatusView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 what;
			message->FindInt32("be:observe_change_what", &what);
			switch (what) {
				case kMsgControllerCaptureStarted:
					SetRecording(true);
					break;
				case kMsgControllerCaptureStopped:
					if (fPaused)
						fPaused = false;
					SetRecording(false);
					break;
				case kMsgControllerCapturePaused:
				case kMsgControllerCaptureResumed:
					TogglePause(what == kMsgControllerCapturePaused);
					break;
	
				default:
					break;
			}
			break;
		}
		
		default:
			BView::MessageReceived(message);
			break;
	}
	
}


void
CamStatusView::Pulse()
{
	fNumFrames = fController->RecordedFrames();
	Invalidate();
}


void
CamStatusView::TogglePause(const bool paused)
{
	fPaused = paused;
	Invalidate();
}


bool
CamStatusView::Paused() const
{
	return fPaused;
}


void
CamStatusView::SetRecording(const bool recording)
{
	fRecording = recording;
	Invalidate();
}


bool
CamStatusView::Recording() const
{
	return fRecording;
}


BSize
CamStatusView::MinSize()
{
	float width = StringWidth("999999") + 20;
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(width, width));
}


BSize
CamStatusView::MaxSize()
{
	float width = StringWidth("999999") + 20;
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), BSize(width, width));
}

