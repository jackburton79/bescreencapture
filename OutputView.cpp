/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "OutputView.h"

#include "BSCApp.h"
#include "Constants.h"
#include "ControllerObserver.h"
#include "PreviewView.h"
#include "Settings.h"
#include "SliderTextControl.h"
#include "Utils.h"

#include <Box.h>
#include <Button.h>
#include <Catalog.h>
#include <Debug.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <MessageRunner.h>
#include <RadioButton.h>
#include <Screen.h>
#include <TextControl.h>

#include <algorithm>

#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "OutputView"


const static int32 kInitialWarningCount = 20;

const static uint32 kCheckBoxAreaSelectionChanged = 'CaCh';
const static uint32 kOpenFilePanel = 'OpFp';
const static uint32 kScaleChanged = 'ScCh';
const static uint32 kWindowBorderFrameChanged = 'WbFc';
const static uint32 kWarningRunnerMessage = 'WaRm';

const static char* kSelectWindowButtonText = B_TRANSLATE("Select window");
const static char* kSelectRegionButtonText = B_TRANSLATE("Select region");

OutputView::OutputView()
	:
	BView("capture_options", B_WILL_DRAW),
	fWarningRunner(NULL),
	fWarningCount(0)
{
	_LayoutView();
}


void
OutputView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Watch for these from Controller
	if (be_app->LockLooper()) {
		be_app->StartWatching(this, kMsgControllerSourceFrameChanged);
		be_app->StartWatching(this, kMsgControllerTargetFrameChanged);
		be_app->StartWatching(this, kMsgControllerSelectionWindowClosed);
		be_app->StartWatching(this, kMsgControllerMediaFileFormatChanged);
		be_app->StartWatching(this, kMsgControllerCaptureStarted);
		be_app->StartWatching(this, kMsgControllerCaptureStopped);
		be_app->StartWatching(this, kMsgControllerEncodeStarted);
		be_app->StartWatching(this, kMsgControllerEncodeFinished);
		be_app->StartWatching(this, kMsgControllerResetSettings);
		be_app->UnlockLooper();
	}

	fSelectAreaButton->SetTarget(be_app);

	fFileName->SetTarget(this);
	fCustomArea->SetTarget(this);
	fWholeScreen->SetTarget(this);
	fWindow->SetTarget(this);
	fScaleSlider->SetTarget(this);
	fFilePanelButton->SetTarget(this);
	fBorderSlider->SetTarget(this);

	_InitControlsFromSettings();
	UpdatePreviewFromSettings();
}


void
OutputView::WindowActivated(bool active)
{
	if (fWindow->Value() != B_CONTROL_ON) {
		BRect rect = _CaptureRect();
		_UpdatePreview(&rect);
	}
}


void
OutputView::MessageReceived(BMessage *message)
{
	BSCApp* app = dynamic_cast<BSCApp*>(be_app);

	ASSERT(app != NULL);

	switch (message->what) {
		case kOpenFilePanel:
		{
			BMessenger target(this);
			BFilePanel* filePanel = new BFilePanel(B_SAVE_PANEL,
				&target, NULL, 0, false, NULL);
			filePanel->Show();
			break;
		}
		case kCheckBoxAreaSelectionChanged:
		{
			BRect rect = fCustomCaptureRect;
			if (fWholeScreen->Value() == B_CONTROL_ON) {
				rect = BScreen().Frame();
				fSelectAreaButton->SetEnabled(false);
				fBorderSlider->SetEnabled(false);
			} else {
				fSelectAreaButton->SetEnabled(true);
				if (fCustomArea->Value() == B_CONTROL_ON) {
					fSelectAreaButton->SetLabel(kSelectRegionButtonText);
					fSelectAreaButton->SetMessage(new BMessage(kSelectArea));
					fBorderSlider->SetEnabled(false);
				} else if (fWindow->Value() == B_CONTROL_ON) {
					fSelectAreaButton->SetLabel(kSelectWindowButtonText);
					fSelectAreaButton->SetMessage(new BMessage(kSelectWindow));
					fBorderSlider->SetEnabled(true);
				}
			}
			app->SetCaptureArea(rect);
			break;
		}
		case kFileNameChanged:
		{
			const BEntry entry(fFileName->Text());
			if (entry.InitCheck() != B_OK)
				_HandleInvalidFileName(fFileName->Text());

			app->SetOutputFileName(fFileName->Text());
			break;
		}
		case kScaleChanged:
		{
			int32 value;
			if (message->FindInt32("be:value", &value) == B_OK)
				app->SetScale(float(value));
			break;
		}
		case kWindowBorderFrameChanged:
		{
			const int32 size = fBorderSlider->Value();
			Settings::Current().SetWindowFrameEdgeSize(size);
			break;
		}
		case kWarningRunnerMessage:
		{
			if (--fWarningCount == 0) {
				// Invalidate so it reverts to the default color
				fFileName->Invalidate();
				break;
			}
			rgb_color color = kRed;
			color.red = kRed.red + (fWarningCount - kInitialWarningCount) * 6;
			color.green = kRed.green - (fWarningCount - kInitialWarningCount) * 6;
			color.blue = kRed.blue - (fWarningCount - kInitialWarningCount) * 6;

			fFileName->PushState();
			fFileName->SetHighColor(color);
			BRect rect(fFileName->Bounds());
			rect.left += fFileName->Divider();
			fFileName->StrokeRect(rect, B_SOLID_HIGH);
			fFileName->PopState();
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			if (message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code) != B_OK)
				break;
			switch (code) {
				case kMsgControllerSourceFrameChanged:
				{
					BRect rect;
					if (message->FindRect("frame", &rect) == B_OK) {
						if (rect != BScreen().Frame())
							fCustomCaptureRect = rect;
						_UpdatePreview(&rect);
					}
					break;
				}
				case kMsgControllerTargetFrameChanged:
				{
					Settings& settings = Settings::Current();
					fScaleSlider->SetValue(settings.Scale());
					break;
				}
				case kMsgControllerSelectionWindowClosed:
				{
					BRect rect;
					BBitmap* bitmap = NULL;
					if (message != NULL && message->FindRect("selection", &rect) == B_OK
						&& message->FindPointer("bitmap", reinterpret_cast<void**>(&bitmap)) == B_OK) {
						_UpdatePreview(&rect, bitmap);
						delete bitmap;
					}
					break;
				}
				case kMsgControllerMediaFileFormatChanged:
					_SetFileNameExtension(app->MediaFileFormat().file_extension);
					app->SetOutputFileName(fFileName->Text());
					_UpdateFileNameControlState();
					break;

				case kMsgControllerCaptureStarted:
					fScaleSlider->SetEnabled(false);
					break;

				case kMsgControllerCaptureStopped:
					break;

				case kMsgControllerEncodeStarted:
					fFileName->SetEnabled(false);
					break;

				case kMsgControllerEncodeFinished:
					_UpdateFileNameControlState();
					fScaleSlider->SetEnabled(true);
					break;

				case kMsgControllerResetSettings:
				{
					_InitControlsFromSettings();
					UpdatePreviewFromSettings();
					break;
				}

				default:
					break;
			}
			break;
		}
		case B_SAVE_REQUESTED:
		{
			entry_ref ref;
			const char* name = NULL;
			if (message->FindRef("directory", &ref) != B_OK
				|| message->FindString("name", &name) != B_OK)
				break;

			BPath path(&ref);
			path.Append(name);
			fFileName->SetText(path.Path());

			BFilePanel* filePanel = NULL;
			if (message->FindPointer("source", reinterpret_cast<void**>(&filePanel)) == B_OK)
				delete filePanel;

			_SetFileNameExtension(app->MediaFileFormat().file_extension);

			// TODO: why does the textcontrol not send the modification message ?
			app->SetOutputFileName(fFileName->Text());

			break;
		}
		case B_CANCEL:
		{
			BFilePanel* filePanel = NULL;
			if (message->FindPointer("source", reinterpret_cast<void**>(&filePanel)) == B_OK)
				delete filePanel;
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}


BPath
OutputView::OutputFileName() const
{
	const BPath path = fFileName->Text();
	return path;
}


void
OutputView::UpdatePreviewFromSettings()
{
	BRect rect = Settings::Current().CaptureArea();
	if (!rect.IsValid())
		rect = BScreen().Frame();
	_UpdatePreview(&rect, NULL);
}


void
OutputView::_LayoutView()
{
	SetLayout(new BGroupLayout(B_VERTICAL));

	BBox *selectBox = new BBox("source");
	selectBox->SetLabel(B_TRANSLATE("Source"));
	AddChild(selectBox);

	BBox *outputBox = new BBox("output");
	outputBox->SetLabel(B_TRANSLATE("Output"));
	AddChild(outputBox);

	const Settings& settings = Settings::Current();
	const char *kTCLabel = B_TRANSLATE("File name:");
	BString fileName = settings.OutputFileName();
	fFileName = new BTextControl("file name",
			kTCLabel, fileName.String(), new BMessage(kFileNameChanged));

	fWholeScreen = new BRadioButton("screen frame",
		B_TRANSLATE("Whole screen"), new BMessage(kCheckBoxAreaSelectionChanged));
	fCustomArea = new BRadioButton("region",
		B_TRANSLATE("Region"), new BMessage(kCheckBoxAreaSelectionChanged));
	fWindow = new BRadioButton("window",
		B_TRANSLATE("Window"), new BMessage(kCheckBoxAreaSelectionChanged));

	fSelectAreaButton = new BButton("select region", B_TRANSLATE("Select region"),
		new BMessage(kSelectArea));
	fSelectAreaButton->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
	float selectAreaButtonMinWidth = std::max(fSelectAreaButton->StringWidth(kSelectWindowButtonText),
		fSelectAreaButton->StringWidth(kSelectRegionButtonText));
	fSelectAreaButton->SetExplicitMinSize(BSize(selectAreaButtonMinWidth, B_SIZE_UNSET));
	fSelectAreaButton->SetEnabled(false);

	fFilePanelButton = new BButton(B_TRANSLATE("Browse" B_UTF8_ELLIPSIS),
		new BMessage(kOpenFilePanel));
	fFilePanelButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));

	fScaleSlider = new SliderTextControl("scale_slider", B_TRANSLATE("Scale"),
		new BMessage(kScaleChanged), 25, 200, 25, "%", B_HORIZONTAL);

	fBorderSlider = new SliderTextControl("border_slider", B_TRANSLATE("Window edges"),
		new BMessage(kWindowBorderFrameChanged), 0, 40, 1, B_TRANSLATE("pixels"), B_HORIZONTAL);

	BGroupLayout *layout = BLayoutBuilder::Group<>(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGroup(B_VERTICAL, B_USE_HALF_ITEM_SPACING)
				.Add(fWholeScreen)
				.Add(fCustomArea)
				.Add(fWindow)
				.Add(fSelectAreaButton)
			.End()
			.AddStrut(30)
			.Add(fPreviewView = new PreviewView())
		.End()
		.Add(fBorderSlider)
		.SetInsets(B_USE_DEFAULT_SPACING);

	selectBox->AddChild(layout->View());

	layout = BLayoutBuilder::Group<>(B_VERTICAL)
		.AddGroup(B_HORIZONTAL)
			.Add(fFileName)
			.Add(fFilePanelButton)
		.End()
		.Add(fScaleSlider)
		.SetInsets(B_USE_DEFAULT_SPACING);

	outputBox->AddChild(layout->View());

	fScaleSlider->SetValue(100);

	BSCApp* app = dynamic_cast<BSCApp*>(be_app);
	if (app != NULL)
		fFileExtension = app->MediaFileFormat().file_extension;
}


void
OutputView::_InitControlsFromSettings()
{
	const Settings& settings = Settings::Current();
	fBorderSlider->SetValue(settings.WindowFrameEdgeSize());
	fScaleSlider->SetValue(settings.Scale());
	fBorderSlider->SetEnabled(fWindow->Value() == B_CONTROL_ON);
	fCustomCaptureRect = settings.CaptureArea();
	if (fCustomCaptureRect == BScreen().Frame())
		fWholeScreen->SetValue(B_CONTROL_ON);
	else {
		fCustomArea->SetValue(B_CONTROL_ON);
		fSelectAreaButton->SetEnabled(true);
	}
	_UpdateFileNameControlState();
}


void
OutputView::_UpdateFileNameControlState()
{
	BSCApp* app = dynamic_cast<BSCApp*>(be_app);
	bool enabled = app != NULL && ::strcmp(app->MediaFileFormat().short_name,
											NULL_FORMAT_SHORT_NAME) != 0;
	fFileName->SetEnabled(enabled);
}


BRect
OutputView::_CaptureRect() const
{
	if (fWholeScreen->Value() == B_CONTROL_ON)
		return BScreen(Window()).Frame();
	return fCustomCaptureRect;
}


void
OutputView::_SetFileNameExtension(const char* newExtension)
{
	// TODO: If fFileExtension contains the wrong extension
	// (for example if the user renamed the file manually)
	// this will fail. For example, outputfile.avi, but
	// fFileExtension is mkv -> outputfile.avi.mkv
	BString fileName = fFileName->Text();
	BString extension = ".";
	extension.Append(fFileExtension);
	fileName.RemoveLast(extension);
	fFileExtension = newExtension;
	fileName.Append(".");
	fileName.Append(fFileExtension);
	fFileName->SetText(fileName.String());
}


void
OutputView::_UpdatePreview(BRect* rect, BBitmap* bitmap)
{
	BSCApp* app = dynamic_cast<BSCApp*>(be_app);
	if (fPreviewView != NULL && app != NULL && app->State() == BSCApp::STATE_IDLE)
		fPreviewView->Update(rect, bitmap);
}


void
OutputView::_HandleInvalidFileName(const char* fileName)
{
	// Revert to previous name
	Settings& settings = Settings::Current();
	fFileName->SetText(settings.OutputFileName());

	BMessenger thisMessenger(this);
	if (fWarningCount > 0) {
		// Re-start runner
		fWarningRunner->SetCount(kInitialWarningCount);
	} else
		fWarningRunner = new BMessageRunner(thisMessenger,
				new BMessage(kWarningRunnerMessage), 200000, kInitialWarningCount);

	fWarningCount = kInitialWarningCount;
}
