#include "CamStatusView.h"
#include "Controller.h"
#include "ControllerObserver.h"
#include "Messages.h"

#include <Application.h>
#include <Bitmap.h>
#include <GroupLayoutBuilder.h>
#include <IconUtils.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <Resources.h>
#include <StringView.h>
#include <Window.h>

#include <algorithm>
#include <stdio.h>

const static float kSpacing = 10;
const static float kBitmapSize = 48;

static float
capped_size(float size)
{
	return std::min(size, 12.00f);
}


class SquareBitmapView : public BView {
public:
	SquareBitmapView(const char* name)
		:
		BView(name, B_WILL_DRAW|B_FULL_UPDATE_ON_RESIZE),
		fBitmap(NULL)
	{
		
	}
	
	virtual void Draw(BRect updateRect)
	{
		SetDrawingMode(B_OP_ALPHA);
		DrawBitmap(fBitmap, fBitmap->Bounds(), Bounds());	
	}
	void SetBitmap(BBitmap* bitmap) { fBitmap = bitmap; }
private:
	BBitmap* fBitmap;
};

CamStatusView::CamStatusView(Controller* controller)
	:
	BView("CamStatusView", B_WILL_DRAW|B_PULSE_NEEDED),
	fController(controller),
	fStringView(NULL),
	fBitmapView(NULL),
	fNumFrames(0),
	fRecording(false),
	fPaused(false),
	fRecordingBitmap(NULL),
	fPauseBitmap(NULL)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	BRect bitmapRect(0, 0, kBitmapSize, kBitmapSize);
	fRecordingBitmap = new BBitmap(bitmapRect, B_RGBA32);
	fPauseBitmap = new BBitmap(bitmapRect, B_RGBA32);
	
	BView* layoutView = BLayoutBuilder::Group<>()
		.SetInsets(0)
		.Add(fBitmapView = new SquareBitmapView("bitmap view"))
		.Add(fStringView = new BStringView("cam string view", ""))
	.View();
	
	fStringView->Hide();
	fBitmapView->Hide();
	
	BFont font;
	GetFont(&font);
	float scaledSize = kBitmapSize * (capped_size(font.Size()) / 12);
	fBitmapView->SetExplicitMinSize(BSize(scaledSize, scaledSize));
	fBitmapView->SetExplicitMaxSize(BSize(scaledSize, scaledSize));
	fBitmapView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
	AddChild(layoutView);
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
	
	BResources* resources = be_app->AppResources();
	size_t size;
	const void* buffer = resources->LoadResource('VICN', "record_icon", &size);
	if (buffer != NULL)
		BIconUtils::GetVectorIcon((uint8*)buffer, size, fRecordingBitmap);
	buffer = resources->LoadResource('VICN', "pause_icon", &size);
	if (buffer != NULL)
		BIconUtils::GetVectorIcon((uint8*)buffer, size, fPauseBitmap);
	
	fBitmapView->SetBitmap(fRecordingBitmap);
}


void
CamStatusView::Draw(BRect updateRect)
{
	BView::Draw(updateRect);
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
	fRecordTime = (time_t)fController->RecordTime() / 1000000;
	if (fRecordTime < 0)
		fRecordTime = 0;
	struct tm* diffTime = localtime(&fRecordTime);
	char timeString[128];
	strftime(timeString, sizeof(timeString), "%T", diffTime);
	BString str(timeString);
	str << " (" << fNumFrames << " frames)";
	fStringView->SetText(str.String());
	Invalidate();
}


void
CamStatusView::TogglePause(const bool paused)
{
	fPaused = paused;
	if (fPaused)
		fBitmapView->SetBitmap(fPauseBitmap);
	else
		fBitmapView->SetBitmap(fRecordingBitmap);
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
	if (recording) {
		fStringView->Show();
		fBitmapView->Show();
	} else {
		fStringView->Hide();
		fBitmapView->Hide();
	}
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
	/*BFont font;
	GetFont(&font);
	float scale = capped_size(font.Size()) / 12;
	float bitmapSize = kBitmapSize * scale;
	float spacingSize = kSpacing * scale;
	float width = bitmapSize + StringWidth("99999999999999") + spacingSize;
	float height = bitmapSize;
	
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(width, height));*/
	return BView::MinSize();
}


BSize
CamStatusView::MaxSize()
{
	return BView::MaxSize();
}

