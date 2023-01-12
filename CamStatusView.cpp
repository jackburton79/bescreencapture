/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "CamStatusView.h"

#include "BSCApp.h"
#include "Constants.h"
#include "ControllerObserver.h"

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
#include <iostream>

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


CamStatusView::CamStatusView()
	:
	BView("CamStatusView", B_WILL_DRAW|B_PULSE_NEEDED),
	fStringView(NULL),
	fBitmapView(NULL),
	fEncodingStringView(NULL),
	fStatusBar(NULL),
	fNumFrames(0),
	fStatusText(""),
	fRecording(false),
	fPaused(false),
	fRecordingBitmap(NULL),
	fPauseBitmap(NULL)
{
	BRect bitmapRect(0, 0, kBitmapSize, kBitmapSize);
	fRecordingBitmap = new BBitmap(bitmapRect, B_RGBA32);
	fPauseBitmap = new BBitmap(bitmapRect, B_RGBA32);

	fEncodingStringView = new BStringView("encoding_string_view", "");
	fStatusBar = new BStatusBar("progress_bar", "");
	fStatusBar->SetExplicitMinSize(BSize(100, B_SIZE_UNSET));
	
	fBitmapView = new SquareBitmapView("bitmap_view");
	fBitmapView->SetBitmap(NULL);

	BFont font;
	GetFont(&font);
	float scaledSize = kBitmapSize * (capped_size(font.Size()) / 12);
	fBitmapView->SetExplicitMinSize(BSize(scaledSize, scaledSize));
	fBitmapView->SetExplicitMaxSize(BSize(scaledSize, scaledSize));
	fBitmapView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_TOP));

	fStringView = new BStringView("cam_string_view", "");
	fStringView->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));

	BView* statusView = BLayoutBuilder::Group<>()
		.SetInsets(0)
		.Add(fEncodingStringView)
		.Add(fStatusBar)
		.View();

	BView* layoutView = BLayoutBuilder::Group<>()
		.SetInsets(0)
		.Add(fBitmapView)
		.Add(fStringView)
		.View();
	
	BLayoutBuilder::Cards<>(this)
		.Add(layoutView)
		.Add(statusView)
		.SetVisibleItem(int32(0));
}


void
CamStatusView::AttachedToWindow()
{
	if (be_app->LockLooper()) {
		be_app->StartWatching(this, kMsgControllerCaptureStarted);
		be_app->StartWatching(this, kMsgControllerCaptureStopped);
		be_app->StartWatching(this, kMsgControllerCapturePaused);
		be_app->StartWatching(this, kMsgControllerCaptureResumed);
		be_app->StartWatching(this, kMsgControllerEncodeStarted);
		be_app->StartWatching(this, kMsgControllerEncodeProgress);
		be_app->StartWatching(this, kMsgControllerEncodeFinished);
		be_app->UnlockLooper();
	}
	
	if (Parent())
		SetViewColor(Parent()->ViewColor());
	
	BResources* resources = be_app->AppResources();
	size_t size;
	const void* buffer = resources->LoadResource(B_VECTOR_ICON_TYPE, "record_icon", &size);
	if (buffer != NULL)
		BIconUtils::GetVectorIcon((uint8*)buffer, size, fRecordingBitmap);
	buffer = resources->LoadResource(B_VECTOR_ICON_TYPE, "pause_icon", &size);
	if (buffer != NULL)
		BIconUtils::GetVectorIcon((uint8*)buffer, size, fPauseBitmap);
	
	fBitmapView->SetBitmap(NULL);

	BString statusString = _GetRecordingStatusString();
	fEncodingStringView->SetExplicitMinSize(BSize(StringWidth(statusString) + 10, B_SIZE_UNSET));
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
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &what);
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
				case kMsgControllerEncodeStarted:
				{
					fEncodingStringView->SetText(fStatusText);
					BCardLayout* cardLayout = dynamic_cast<BCardLayout*>(GetLayout());
					if (cardLayout != NULL)
						cardLayout->SetVisibleItem(1);
					int32 totalFrames = 0;
					if (message->FindInt32("frames_total", &totalFrames) == B_OK)
						fStatusBar->SetMaxValue(float(totalFrames));
					break;
				}
				case kMsgControllerEncodeProgress:
				{
					bool reset = false;
					const char* text = NULL;
					message->FindString("text", &text);
					if (message->FindBool("reset", &reset) == B_OK && reset) {
						int32 totalFrames = 0;
						message->FindInt32("frames_total", &totalFrames);
						fStatusText = text;
						fEncodingStringView->SetText(fStatusText);
						fStatusBar->Reset();
						fStatusBar->SetMaxValue(float(totalFrames));
					}

					int32 remaining = 0;
					int32 current = int32(fStatusBar->CurrentValue());
					int32 total = int32(fStatusBar->MaxValue());
					int32 done = 0;
					if (message->FindInt32("frames_remaining", &remaining) == B_OK) {
						done = total - remaining;
						BString string = fStatusText;
						string << " (" << done << "/" << total << " frames)";
						fEncodingStringView->SetText(string);
					}
					fStatusBar->Update(done - current);
					break;
				}
				case kMsgControllerEncodeFinished:
				{
					BCardLayout* cardLayout = dynamic_cast<BCardLayout*>(GetLayout());
					if (cardLayout != NULL)
						cardLayout->SetVisibleItem((int32)0);
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
	if (!fRecording)
		return;

	BSCApp* app = dynamic_cast<BSCApp*>(be_app);
	fNumFrames = app->RecordedFrames();
	BString str = _GetRecordingStatusString();
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
		BCardLayout* cardLayout = dynamic_cast<BCardLayout*>(GetLayout());
		if (cardLayout != NULL)
			cardLayout->SetVisibleItem((int32)0);
		fStatusBar->Reset();
	} else {
		fBitmapView->SetBitmap(NULL);
		fStringView->SetText("");
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


BString
CamStatusView::_GetRecordingStatusString() const
{
	BSCApp* app = dynamic_cast<BSCApp*>(be_app);
	time_t recordTime = (time_t)app->RecordTime() / 1000000;
	if (recordTime < 0)
		recordTime = 0;
	struct tm timeStruct;
	struct tm* diffTime = gmtime_r(&recordTime, &timeStruct);
	BString timeString;
	strftime(timeString.LockBuffer(128), 128, "%T", diffTime);
	timeString.UnlockBuffer();
	timeString << ", " << fNumFrames << " frames";
	timeString << " (" << app->AverageFPS() << " avg)";
	return timeString;
}
