#include "ControllerObserver.h"
#include "messages.h"
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
const static int32 kFileTypeChanged = 'FtyC';
const static int32 kCodecChanged = 'CdCh';
const static int32 kOpenFilePanel = 'OpFp';
const static int32 kMsgTextControlSizeChanged = 'TCSC';
const static int32 kScaleChanged = 'ScCh';

class MediaFileFormatMenuItem : public BMenuItem {
public:
	MediaFileFormatMenuItem(const media_file_format& fileFormat);
	media_file_format MediaFileFormat() const;
private:
	media_file_format fFileFormat;
};



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
	BString fileName;
	settings.GetOutputFileName(fileName);
	fFileName = new BTextControl("file name",
			kTCLabel, fileName.String(), new BMessage(kFileNameChanged));

	const char *kOutputMenuLabel = "File format:";
	BPopUpMenu *fileFormatPopUp = new BPopUpMenu("Format");
	fOutputFileType = new BMenuField("OutFormat",
			kOutputMenuLabel, fileFormatPopUp);
						
	const char *kCodecMenuLabel = "Media codec:";
	BPopUpMenu *popUpMenu = new BPopUpMenu("Codecs");
	fCodecMenu = new BMenuField("OutCodec", kCodecMenuLabel, popUpMenu);
	
	fWholeScreen = new BRadioButton("screen frame", "Whole screen",
		new BMessage(kCheckBoxAreaSelectionChanged));
	fCustomArea = new BRadioButton("region",
		"Region", new BMessage(kCheckBoxAreaSelectionChanged));
	fSelectArea = new BButton("select region", "Select region", new BMessage(kSelectArea));
	fSelectArea->SetExplicitAlignment(BAlignment(B_ALIGN_LEFT, B_ALIGN_MIDDLE));
	fSelectArea->SetEnabled(false);
	
	fFilePanelButton = new BButton("...", new BMessage(kOpenFilePanel));
	fFilePanelButton->SetExplicitMaxSize(BSize(35, 25));
	fFilePanelButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	fMinimizeOnStart = new BCheckBox("Minimize on start",
		"Minimize on recording", new BMessage(kMinimizeOnRecording));
	
	fSizeSlider = new SizeControl("size_slider", "Resize",
		new BMessage(kScaleChanged), 25, 200, B_HORIZONTAL);
				
	BView *layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
			.AddGroup(B_VERTICAL)
				.Add(fWholeScreen)
				.Add(fCustomArea)
				.Add(fSelectArea)
				.AddStrut(20)
			.End()
			.Add(fRectView = new PreviewView())
		.End()
		.View();
	
	selectBox->AddChild(layoutView);
	
	layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
			.AddGroup(B_HORIZONTAL, 0)
				.Add(fFileName)
				.Add(fFilePanelButton)
			.End()
			.Add(fOutputFileType)
			.Add(fCodecMenu)
			.Add(fSizeSlider)
			.SetInsets(B_USE_DEFAULT_SPACING)
			.Add(fMinimizeOnStart)
		.End()	
		.View();

	outputBox->AddChild(layoutView);	
	
	fSizeSlider->SetValue(100);
	
	fMinimizeOnStart->SetValue(settings.MinimizeOnRecording() ? B_CONTROL_ON : B_CONTROL_OFF);

	_BuildFileFormatsMenu();
	
	BString savedFileFormat;
	settings.GetOutputFileFormat(savedFileFormat);

	BMenuItem* item = NULL;
	if (savedFileFormat != "")
		item = fOutputFileType->Menu()->FindItem(savedFileFormat.String());

	if (item == NULL)
		item = fOutputFileType->Menu()->ItemAt(0);

	if (item != NULL)
		item->SetMarked(true);
	else {
		// TODO: This means there is no working media encoder.;
		// do something smart (like showing an alert and disable
		// the "Start" button
	}

	fFileExtension = FileFormat().file_extension;

	fWholeScreen->SetValue(B_CONTROL_ON);
	
	RequestMediaFormatUpdate();
	
	fController->SetCaptureArea(BScreen(Window()).Frame());
	fController->SetMediaFileFormat(FileFormat());
	fController->SetMediaFormatFamily(FormatFamily());
	fController->SetOutputFileName(fFileName->Text());
	
	fCustomCaptureRect = BScreen().Frame();
}


void
OutputView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fMinimizeOnStart->SetTarget(this);
	fFileName->SetTarget(this);
	fOutputFileType->Menu()->SetTargetForItems(this);
	fCustomArea->SetTarget(this);
	fWholeScreen->SetTarget(this);
	fSizeSlider->SetTarget(this);
	fFilePanelButton->SetTarget(this);
	
	fSizeSlider->SetValue(Settings().Scale());		
	
	UpdatePreviewFromSettings();
	_RebuildCodecsMenu();
}


void
OutputView::WindowActivated(bool active)
{
	if (fWholeScreen->Value() == B_CONTROL_ON)
		_UpdatePreview(NULL);
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
			} else
				fSelectArea->SetEnabled(true);
			fController->SetCaptureArea(rect);
			break;	
		}
			
		case kRebuildCodec:
		case kFileTypeChanged:
			fController->SetMediaFileFormat(FileFormat());
			fController->SetMediaFormatFamily(FormatFamily());
			_SetFileNameExtension(FileFormat().file_extension);
			RequestMediaFormatUpdate();
			// Fall through
		case kFileNameChanged:
			fController->SetOutputFileName(fFileName->Text());
			break;
		
		case kMinimizeOnRecording:
			Settings().SetMinimizeOnRecording(fMinimizeOnStart->Value() == B_CONTROL_ON);
			break;
		
		case kCodecChanged:
		{
			BMenuItem* marked = fCodecMenu->Menu()->FindMarked();
			if (marked != NULL)
				fController->SetMediaCodec(marked->Label());
			break;				
		}
		
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
				case kMsgControllerCodecListUpdated:
					_RebuildCodecsMenu();
					break;
				case kMsgControllerVideoDepthChanged:
					RequestMediaFormatUpdate();
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

			_SetFileNameExtension(FileFormat().file_extension);

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


void
OutputView::RequestMediaFormatUpdate()
{
	fController->UpdateMediaFormatAndCodecsForCurrentFamily();
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
	const BRect rect = Settings().CaptureArea();
	fRectView->Update(&rect);
}


void
OutputView::_BuildFileFormatsMenu()
{
	const int32 numItems = fOutputFileType->Menu()->CountItems();
	if (numItems > 0)
		fOutputFileType->Menu()->RemoveItems(0, numItems);

	const uint32 mediaFormatMask = media_file_format::B_KNOWS_ENCODED_VIDEO
								| media_file_format::B_WRITABLE;
	media_file_format mediaFileFormat;
	int32 cookie = 0;
	while (get_next_file_format(&cookie, &mediaFileFormat) == B_OK) {
		if ((mediaFileFormat.capabilities & mediaFormatMask) == mediaFormatMask) {
			MediaFileFormatMenuItem* item = new MediaFileFormatMenuItem(
					mediaFileFormat);
			fOutputFileType->Menu()->AddItem(item);
		}
	}
}


void 
OutputView::_RebuildCodecsMenu()
{
	BMenu* codecsMenu = fCodecMenu->Menu();
	
	BString currentCodec;
	BMenuItem *marked = codecsMenu->FindMarked();
	if (marked != NULL)
		currentCodec = marked->Label();
		
	Window()->BeginViewTransaction();
		
	codecsMenu->RemoveItems(0, codecsMenu->CountItems(), true);
	
	BObjectList<media_codec_info> codecList(1, true);	
	if (fController->GetCodecsList(codecList) == B_OK) {
		for (int32 i = 0; i < codecList.CountItems(); i++) {
			media_codec_info* codec = codecList.ItemAt(i);
			BMenuItem* item = new BMenuItem(codec->pretty_name, new BMessage(kCodecChanged));
			codecsMenu->AddItem(item);
			if (codec->pretty_name == currentCodec)
				item->SetMarked(true);
		}			
		// Make the app object the menu's message target
		fCodecMenu->Menu()->SetTargetForItems(this);
	}
	
	if (codecsMenu->FindMarked() == NULL) {
		BMenuItem *item = codecsMenu->ItemAt(0);
		if (item != NULL)
			item->SetMarked(true);
	}
	
	Window()->EndViewTransaction();
	
	if (currentCodec != codecsMenu->FindMarked()->Label())
		fController->SetMediaCodec(codecsMenu->FindMarked()->Label());
		
}


media_file_format
OutputView::FileFormat() const
{
	MediaFileFormatMenuItem* item = dynamic_cast<MediaFileFormatMenuItem*>(
			fOutputFileType->Menu()->FindMarked());
	return item->MediaFileFormat();
}


media_format_family
OutputView::FormatFamily() const
{
	return FileFormat().family;
}


void
OutputView::_SetFileNameExtension(const char* newExtension)
{
	BString fileName = fFileName->Text();
	fileName.RemoveLast(fFileExtension);
	fileName.RemoveLast(".");
	fileName.Append(".");
	fFileExtension = newExtension;
	fileName.Append(fFileExtension);
	fFileName->SetText(fileName.String());
}


void
OutputView::_UpdatePreview(BRect* rect, BBitmap* bitmap)
{
	fRectView->Update(rect, bitmap);
}


// MediaFileFormatMenuItem
MediaFileFormatMenuItem::MediaFileFormatMenuItem(const media_file_format& fileFormat)
	:
	BMenuItem(fileFormat.pretty_name, new BMessage(kFileTypeChanged)),
	fFileFormat(fileFormat)
{

}


media_file_format
MediaFileFormatMenuItem::MediaFileFormat() const
{
	return fFileFormat;
}





