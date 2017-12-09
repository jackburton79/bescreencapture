#include "CamStatusView.h"
#include "Controller.h"
#include "ControllerObserver.h"
#include "Messages.h"

#include <Application.h>
#include <Bitmap.h>
#include <CardLayout.h>
#include <GroupLayoutBuilder.h>
#include <IconUtils.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Message.h>
#include <Resources.h>
#include <StatusBar.h>
#include <StringView.h>
#include <Window.h>

#include <algorithm>

const static float kSpacing = 10;
const static float kBitmapSize = 48;
const char* kEncodingString = "Encoding movie...";
const char* kDoneString = "Done!";

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
		if (fBitmap != NULL)
			DrawBitmap(fBitmap, fBitmap->Bounds(), Bounds());	
	}
	void SetBitmap(const BBitmap* bitmap)
	{
		fBitmap = bitmap;
		Invalidate();
	}
private:
	const BBitmap* fBitmap;
};

CamStatusView::CamStatusView(Controller* controller)
	:
	BView("CamStatusView", B_WILL_DRAW|B_PULSE_NEEDED),
	fController(controller),
	fStringView(NULL),
	fBitmapView(NULL),
	fEncodingStringView(NULL),
	fStatusBar(NULL),
	fNumFrames(0),
	fRecording(false),
	fPaused(false),
	fRecordingBitmap(NULL),
	fPauseBitmap(NULL)
{
	BCardLayout* cardLayout = new BCardLayout();
	SetLayout(cardLayout);
	BRect bitmapRect(0, 0, kBitmapSize, kBitmapSize);
	fRecordingBitmap = new BBitmap(bitmapRect, B_RGBA32);
	fPauseBitmap = new BBitmap(bitmapRect, B_RGBA32);
	
	BView* statusView = BLayoutBuilder::Group<>()
		.SetInsets(0)
		//.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.Add(fEncodingStringView = new BStringView("stringview", kEncodingString))
			.Add(fStatusBar = new BStatusBar("", ""))
		//.End()
		.View();
	
	fStatusBar->SetExplicitMinSize(BSize(100, 20));
	
	BView* layoutView = BLayoutBuilder::Group<>()
		.SetInsets(0)
		.Add(fBitmapView = new SquareBitmapView("bitmap view"))
		.Add(fStringView = new BStringView("cam string view", ""))
	.View();
	cardLayout->AddView(layoutView);
	cardLayout->AddView(statusView);
	
	fStringView->Hide();
	fBitmapView->Hide();
	
	BFont font;
	GetFont(&font);
	float scaledSize = kBitmapSize * (capped_size(font.Size()) / 12);
	fBitmapView->SetExplicitMinSize(BSize(scaledSize, scaledSize));
	fBitmapView->SetExplicitMaxSize(BSize(scaledSize, scaledSize));
	fBitmapView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));
	
	cardLayout->SetVisibleItem(int32(0));
}


void
CamStatusView::AttachedToWindow()
{
	if (fController->LockLooper()) {
		fController->StartWatching(this, B_UPDATE_STATUS_BAR);
		fController->StartWatching(this, B_RESET_STATUS_BAR);
		fController->StartWatching(this, kMsgControllerCaptureStarted);
		fController->StartWatching(this, kMsgControllerCaptureStopped);
		fController->StartWatching(this, kMsgControllerCapturePaused);
		fController->StartWatching(this, kMsgControllerCaptureResumed);
		fController->StartWatching(this, kMsgControllerEncodeStarted);
		fController->StartWatching(this, kMsgControllerEncodeProgress);
		fController->StartWatching(this, kMsgControllerEncodeFinished);
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
				case B_UPDATE_STATUS_BAR:
				case B_RESET_STATUS_BAR:
				{
					BMessage newMessage(*message);
					message->what = what;
					Window()->PostMessage(message, fStatusBar);
					break;
				}
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
				case kMsgControllerEncodeStarted:
					fEncodingStringView->SetText(kEncodingString);
					((BCardLayout*)GetLayout())->SetVisibleItem(1);		
					break;
				case kMsgControllerEncodeProgress:
				{
					int32 numFiles = 0;
					message->FindInt32("num_files", &numFiles);					
					fStatusBar->SetMaxValue(float(numFiles));
					
					BString string = kEncodingString;
					string << " (" << numFiles << " frames)";
					fEncodingStringView->SetText(string);
					break;
				}
				case kMsgControllerEncodeFinished:
				{
					((BCardLayout*)GetLayout())->SetVisibleItem((int32)0);
					break;
				}
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
		fBitmapView->SetBitmap(fRecordingBitmap);
		((BCardLayout*)GetLayout())->SetVisibleItem((int32)0);
		fStatusBar->Reset();
		fStringView->SetText("");
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
	return BView::MinSize();
}


BSize
CamStatusView::MaxSize()
{
	return BView::MaxSize();
}

