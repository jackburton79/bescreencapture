#include "OutputView.h"

#include "Constants.h"
#include "Controller.h"
#include "ControllerObserver.h"
#include "PreviewView.h"
#include "Settings.h"
#include "SliderTextControl.h"
#include "Utils.h"

#include <Box.h>
#include <Button.h>
#include <FilePanel.h>
#include <LayoutBuilder.h>
#include <RadioButton.h>
#include <Screen.h>
#include <TextControl.h>

#include <iostream>


const static int32 kCheckBoxAreaSelectionChanged = 'CaCh';
const static int32 kOpenFilePanel = 'OpFp';
const static int32 kMsgTextControlSizeChanged = 'TCSC';
const static int32 kScaleChanged = 'ScCh';
const static uint32 kWindowBorderFrameChanged = 'WbFc';

const static char* kSelectWindowButtonText = "Select Window";
const static char* kSelectRegionButtonText = "Select Region";

OutputView::OutputView(Controller *controller)
	:
	BView("Capture Options", B_WILL_DRAW),
	fController(controller)
{
	_LayoutView();
}


void
OutputView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));

	// Watch for these from Controller
	if (fController->LockLooper()) {
		fController->StartWatching(this, kMsgControllerSourceFrameChanged);
		fController->StartWatching(this, kMsgControllerTargetFrameChanged);
		fController->StartWatching(this, kMsgControllerSelectionWindowClosed);
		fController->StartWatching(this, kMsgControllerMediaFileFormatChanged);
		fController->StartWatching(this, kMsgControllerCaptureStarted);
		fController->StartWatching(this, kMsgControllerCaptureStopped);
		fController->StartWatching(this, kMsgControllerEncodeStarted);
		fController->StartWatching(this, kMsgControllerEncodeFinished);
		fController->StartWatching(this, kMsgControllerResetSettings);
		fController->UnlockLooper();
	}

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
			fController->SetCaptureArea(rect);
			break;	
		}
		case kFileNameChanged:
		{
			BEntry entry(fFileName->Text());
			if (entry.Exists())
				_HandleExistingFileName(fFileName->Text());
			fController->SetOutputFileName(fFileName->Text());
			break;
		}
		case kScaleChanged:
		{
			int32 value;
			message->FindInt32("be:value", &value);
			fController->SetScale((float)value);
			break;
		}
		case kWindowBorderFrameChanged:
		{
			const int32 size = fBorderSlider->Value();
			Settings::Current().SetWindowFrameEdgeSize(size);
			break;
		}
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32("be:observe_change_what", &code);
			switch (code) {
				case kMsgControllerSourceFrameChanged:
				{
					BRect rect;
					if (message->FindRect("frame", &rect) == B_OK) {
						if (rect != BScreen().Frame())
							fCustomCaptureRect = rect;
					}
					_UpdatePreview(&rect);
					break;	
				}
				case kMsgControllerSelectionWindowClosed:
				{
					BRect rect;
					BBitmap* bitmap = NULL;
					if (message != NULL && message->FindRect("selection", &rect) == B_OK
						&& message->FindPointer("bitmap", (void**)&bitmap) == B_OK) {	
						_UpdatePreview(&rect, bitmap);
						delete bitmap;
					}
					break;
				}
				case kMsgControllerMediaFileFormatChanged:
					_SetFileNameExtension(fController->MediaFileFormat().file_extension);
					fController->SetOutputFileName(fFileName->Text());
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
					fFileName->SetEnabled(true);
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
			message->FindRef("directory", &ref);
			message->FindString("name", &name);

			BPath path(&ref);
			path.Append(name);
			fFileName->SetText(path.Path());

			BFilePanel* filePanel = NULL;
			if (message->FindPointer("source", (void**)&filePanel) == B_OK)
				delete filePanel;

			_SetFileNameExtension(fController->MediaFileFormat().file_extension);

			// TODO: why does the textcontrol not send the modification message ?
			fController->SetOutputFileName(fFileName->Text());
			
			break;
		}
		case B_CANCEL:
		{
			BFilePanel* filePanel = NULL;
			if (message->FindPointer("source", (void**)&filePanel) == B_OK)
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
	BPath path = fFileName->Text();
	return path;
}


void
OutputView::UpdatePreviewFromSettings()
{
	BRect rect = Settings::Current().CaptureArea();
	if (!rect.IsValid())
		rect = BScreen().Frame();
	fRectView->Update(&rect);
}


void
OutputView::_LayoutView()
{
	BBox *selectBox = new BBox("source");
	selectBox->SetLabel("Source");
	AddChild(selectBox);
	
	BBox *outputBox = new BBox("output");
	outputBox->SetLabel("Output");
	AddChild(outputBox);

	Settings& settings = Settings::Current();
	const char *kTCLabel = "File name:"; 
	BString fileName = settings.OutputFileName();
	fFileName = new BTextControl("file name",
			kTCLabel, fileName.String(), new BMessage(kFileNameChanged));
	
	fWholeScreen = new BRadioButton("screen frame", "Whole screen",
		new BMessage(kCheckBoxAreaSelectionChanged));
	fCustomArea = new BRadioButton("region",
		"Region", new BMessage(kCheckBoxAreaSelectionChanged));
	fWindow = new BRadioButton("window", 
		"Window", new BMessage(kCheckBoxAreaSelectionChanged));
		
	fSelectAreaButton = new BButton("select region", "Select Region", new BMessage(kSelectArea));
	fSelectAreaButton->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
	float selectAreaButtonMinWidth = std::max(fSelectAreaButton->StringWidth(kSelectWindowButtonText),
		fSelectAreaButton->StringWidth(kSelectRegionButtonText));
	fSelectAreaButton->SetExplicitMinSize(BSize(selectAreaButtonMinWidth, 30));
	fSelectAreaButton->SetEnabled(false);
	
	fFilePanelButton = new BButton("...", new BMessage(kOpenFilePanel));
	fFilePanelButton->SetExplicitMaxSize(BSize(35, 25));
	fFilePanelButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	
	fScaleSlider = new SliderTextControl("scale_slider", "Scale",
		new BMessage(kScaleChanged), 25, 200, 25, "%", B_HORIZONTAL);

	fBorderSlider = new SliderTextControl("border_slider", "Window edges",
		new BMessage(kWindowBorderFrameChanged), 0, 40, 1, "pixels", B_HORIZONTAL);
			
	BView *layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL)
			.AddGroup(B_HORIZONTAL)
				.AddGroup(B_VERTICAL, B_USE_HALF_ITEM_SPACING)
					.Add(fWholeScreen)
					.Add(fCustomArea)
					.Add(fWindow)
					.Add(fSelectAreaButton)
					.AddStrut(20)
				.End()
				.Add(fRectView = new PreviewView())
			.End()
			.Add(fBorderSlider)
		.End()
		
		.View();
	
	selectBox->AddChild(layoutView);
	
	layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL, B_USE_HALF_ITEM_SPACING)
			.AddGroup(B_HORIZONTAL)
				.Add(fFileName)
				.Add(fFilePanelButton)
			.End()
			.Add(fScaleSlider)
		.End()	
		.View();

	outputBox->AddChild(layoutView);	
	
	fScaleSlider->SetValue(100);
	
	fFileExtension = fController->MediaFileFormat().file_extension;		
}


void
OutputView::_InitControlsFromSettings()
{
	Settings& settings = Settings::Current();
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
	bool enabled = strcmp(fController->MediaFileFormat().short_name, NULL_FORMAT_SHORT_NAME) != 0;
	fFileName->SetEnabled(enabled);
}


BRect
OutputView::_CaptureRect() const
{
	if (fWholeScreen->Value() == B_CONTROL_ON)
		return BScreen(Window()).Frame();
	else
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
	fRectView->Update(rect, bitmap);
}


void
OutputView::_HandleExistingFileName(const char* fileName)
{
	BString newFileName = fileName;
	newFileName = GetUniqueFileName(newFileName, fFileExtension);
	fFileName->SetText(newFileName);
	// TODO: Give an hint to the user that the file name changed
}

