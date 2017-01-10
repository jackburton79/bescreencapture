#include "ControllerObserver.h"
#include "MediaFormatView.h"
#include "Messages.h"
#include "OutputView.h"
#include "PreviewView.h"
#include "Settings.h"
#include "SizeControl.h"
#include "Utils.h"

#include <Alignment.h>
#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
#include <FilePanel.h>
#include <GridLayoutBuilder.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <MenuItem.h>
#include <PopUpMenu.h>
#include <Path.h>
#include <RadioButton.h>
#include <Screen.h>
#include <Slider.h>
#include <SplitView.h>
#include <SplitLayoutBuilder.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <iostream>

const static int32 kCheckBoxAreaSelectionChanged = 'CaCh';
const static int32 kOpenFilePanel = 'OpFp';
const static int32 kMsgTextControlSizeChanged = 'TCSC';
const static int32 kScaleChanged = 'ScCh';


OutputView::OutputView(Controller *controller)
	:
	BView("Capture Options", B_WILL_DRAW),
	fController(controller)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	
	BBox *selectBox = new BBox("source");
	selectBox->SetLabel("Source");
	AddChild(selectBox);
	
	BBox *outputBox = new BBox("output");
	outputBox->SetLabel("Output");
	AddChild(outputBox);

	Settings settings;
	
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
		
	fSelectArea = new BButton("select region", "Select Region", new BMessage(kSelectArea));
	fSelectArea->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
	fSelectArea->SetEnabled(false);
	
	fFilePanelButton = new BButton("...", new BMessage(kOpenFilePanel));
	fFilePanelButton->SetExplicitMaxSize(BSize(35, 25));
	fFilePanelButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	fMinimizeOnStart = new BCheckBox("HideWhenRecording",
		"Hide window when recording", new BMessage(kMinimizeOnRecording));
	
	fScaleSlider = new SizeControl("scale_slider", "Scale",
		new BMessage(kScaleChanged), 25, 200, 25, "%", B_HORIZONTAL);
				
	BView *layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGroup(B_VERTICAL)
				.Add(fWholeScreen)
				.Add(fCustomArea)
				.Add(fWindow)
				.Add(fSelectArea)
				.AddStrut(20)
			.End()
			.Add(fRectView = new PreviewView())
		.End()
		.View();
	
	selectBox->AddChild(layoutView);
	
	fMediaFormatView = new MediaFormatView(fController);
	
	layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
			.AddGroup(B_HORIZONTAL, 0)
				.Add(fFileName)
				.Add(fFilePanelButton)
			.End()
			.Add(fMediaFormatView)
			.Add(fScaleSlider)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(fMinimizeOnStart)
		.End()	
		.View();

	outputBox->AddChild(layoutView);	
	
	fScaleSlider->SetValue(100);
	
	fMinimizeOnStart->SetValue(settings.MinimizeOnRecording() ? B_CONTROL_ON : B_CONTROL_OFF);

	fFileExtension = fController->MediaFileFormat().file_extension;
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
		fController->UnlockLooper();
	}
	
	fMinimizeOnStart->SetTarget(this);
	fFileName->SetTarget(this);
	fCustomArea->SetTarget(this);
	fWholeScreen->SetTarget(this);
	fWindow->SetTarget(this);
	fScaleSlider->SetTarget(this);
	fFilePanelButton->SetTarget(this);
	
	fScaleSlider->SetValue(Settings().Scale());	
		
	Settings().GetCaptureArea(fCustomCaptureRect);
	if (fCustomCaptureRect == BScreen().Frame())
		fWholeScreen->SetValue(B_CONTROL_ON);
	else {
		fCustomArea->SetValue(B_CONTROL_ON);
		fSelectArea->SetEnabled(true);
	}
			
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
				fSelectArea->SetEnabled(false);
			} else {
				fSelectArea->SetEnabled(true);
				if (fCustomArea->Value() == B_CONTROL_ON) {
					fSelectArea->SetLabel("Select Region");
					fSelectArea->SetMessage(new BMessage(kSelectArea));
				} else if (fWindow->Value() == B_CONTROL_ON) {
					fSelectArea->SetLabel("Select Window");
					fSelectArea->SetMessage(new BMessage(kSelectWindow));
				}
			}
			fController->SetCaptureArea(rect);
			break;	
		}
		
		case kFileNameChanged:
			fController->SetOutputFileName(fFileName->Text());
			break;
		
		case kMinimizeOnRecording:
			Settings().SetMinimizeOnRecording(fMinimizeOnStart->Value() == B_CONTROL_ON);
			break;
				
		case kScaleChanged:
		{
			int32 value;
			message->FindInt32("be:value", &value);
			fController->SetScale((float)value);
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
					break;
					
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


bool
OutputView::MinimizeOnStart() const
{
	return fMinimizeOnStart->Value() == B_CONTROL_ON;
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
	BRect rect = Settings().CaptureArea();
	if (!rect.IsValid())
		rect = BScreen().Frame();
	fRectView->Update(&rect);
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

