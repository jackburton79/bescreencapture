#include "CamStatusView.h"
#include "Controller.h"
#include "ControllerObserver.h"
#include "messages.h"

#include <LayoutUtils.h>
#include <Message.h>
#include <Window.h>

CamStatusView::CamStatusView(const char *name)
	:
	BView(name, B_WILL_DRAW),
	fRecording(false),
	fPaused(false)
{
}


void
CamStatusView::AttachedToWindow()
{
	if (Parent())
		SetViewColor(Parent()->ViewColor());
}


void
CamStatusView::Draw(BRect updateRect)
{
	BRect bounds = Bounds();
	/*
	SetHighColor(0, 0, 0);
	StrokeRect(bounds);
	bounds.InsetBy(.5, .5);
	*/
	if (fRecording) {
		if (fPaused) {
			SetHighColor(0, 240, 0);
			BRect one(2, 1, bounds.right / 2 - 2, bounds.bottom - 1);
			FillRect(one);
			one.OffsetTo(bounds.Width() - one.Width() - 2, 1);
			FillRect(one);
		} else {
			SetHighColor(248, 0, 0);
			//bounds.InsetBy(4, 4);
			FillEllipse(bounds);
		}
	} else {
		SetHighColor(ViewColor());
		FillRect(bounds & updateRect);
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
				case kMsgGUIToggleCapture:
					SetRecording(!Recording());
					break;
				case kMsgControllerCapturePaused:
				case kMsgControllerCaptureResumed:
					TogglePause();
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
CamStatusView::TogglePause()
{
	fPaused = !fPaused;
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
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(25, 25));
}


BSize
CamStatusView::MaxSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMaxSize(), BSize(25, 25));
}

