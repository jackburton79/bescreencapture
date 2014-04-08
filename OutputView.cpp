#include "ControllerObserver.h"
#include "messages.h"
#include "OutputView.h"
#include "PreviewView.h"
#include "Settings.h"
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
#include <SplitView.h>
#include <SplitLayoutBuilder.h>
#include <String.h>
#include <StringView.h>
#include <TextControl.h>
#include <Window.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>

const static int32 kCheckBoxAreaSelectionChanged = 'CaCh';
const static int32 kFileTypeChanged = 'FtyC';
const static int32 kCodecChanged = 'CdCh';
const static int32 kOpenFilePanel = 'OpFp';

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
	
	BBox *selectBox = new BBox("selection");
	selectBox->SetLabel("Selection");
	AddChild(selectBox);
	
	BBox *outputBox = new BBox("output");
	outputBox->SetLabel("Output");
	AddChild(outputBox);

	Settings settings;
	
	const char *kTCLabel = "File name:"; 
	const char *fileName = NULL;
	settings.GetOutputFileName(&fileName);
	fFileName = new BTextControl("file name",
			kTCLabel, fileName, new BMessage(kFileNameChanged));

	const char *kOutputMenuLabel = "Output Format:";
	BPopUpMenu *fileFormatPopUp = new BPopUpMenu("Format");
	fOutputFileType = new BMenuField("OutFormat",
			kOutputMenuLabel, fileFormatPopUp);
						
	const char *kCodecMenuLabel = "Codec:";
	BPopUpMenu *popUpMenu = new BPopUpMenu("Codecs");
	fCodecMenu = new BMenuField("OutCodec", kCodecMenuLabel, popUpMenu);
	
	fWholeScreen = new BRadioButton("screen frame", "Whole screen",
		new BMessage(kCheckBoxAreaSelectionChanged));
	fCustomArea = new BRadioButton("custom area",
		"Custom Area", new BMessage(kCheckBoxAreaSelectionChanged));
	fSelectArea = new BButton("select area", "Select", new BMessage(kSelectArea));
	fSelectArea->SetEnabled(false);
	
	fFilePanelButton = new BButton("...", new BMessage(kOpenFilePanel));
	fFilePanelButton->SetExplicitMaxSize(BSize(35, 25));
	fFilePanelButton->SetExplicitAlignment(BAlignment(B_ALIGN_RIGHT, B_ALIGN_MIDDLE));
	fMinimizeOnStart = new BCheckBox("Minimize on start",
		"Minimize on recording", new BMessage(kMinimizeOnRecording));
	
	fRectView = new PreviewView();
	
	BView *layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
			.AddGroup(B_HORIZONTAL, 0)
				.Add(fFileName)
				.Add(fFilePanelButton)
			.End()
			.Add(fOutputFileType)
			.Add(fCodecMenu)
			.Add(fMinimizeOnStart)
		.End()	
		.View();

	outputBox->AddChild(layoutView);

	layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.AddGroup(B_VERTICAL)
			.AddGroup(B_HORIZONTAL)
				.AddGroup(B_VERTICAL, 0)
					.Add(fWholeScreen)
					.Add(fCustomArea)
				.End()
				.AddGroup(B_VERTICAL)
					.AddGlue()
					.Add(fSelectArea)
				.End()
			.End()
			.Add(fRectView)
		.End()
		.View();
	
	selectBox->AddChild(layoutView);

	fMinimizeOnStart->SetValue(settings.MinimizeOnRecording() ? B_CONTROL_ON : B_CONTROL_OFF);
	
	// fill in the list of available file formats
	media_file_format mff;
	int32 cookie = 0;
	while (get_next_file_format(&cookie, &mff) == B_OK) {
		if (mff.capabilities &
				(media_file_format::B_KNOWS_ENCODED_VIDEO
				| media_file_format::B_WRITABLE)) {
			MediaFileFormatMenuItem* item = new MediaFileFormatMenuItem(
					mff);
			fOutputFileType->Menu()->AddItem(item);
		}	
	}
	
	BString savedFileFormat;
	settings.GetOutputFileFormat(savedFileFormat);

	BMenuItem* item = fOutputFileType->Menu()->ItemAt(0);
	if (savedFileFormat != "") {
		item = fOutputFileType->Menu()->FindItem(savedFileFormat.String());
	}

	if (item != NULL)
		item->SetMarked(true);

	_SetFileNameExtension(FileFormat().file_extension);

	fWholeScreen->SetValue(B_CONTROL_ON);
	
	UpdateSettings();
	
	fController->SetCaptureArea(BScreen(Window()).Frame());
	fController->SetMediaFileFormat(FileFormat());
	fController->SetMediaFormatFamily(FormatFamily());
	fController->SetOutputFileName(fFileName->Text());
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
	fFilePanelButton->SetTarget(this);
	
	UpdatePreviewFromSettings();
	_RebuildCodecsMenu();
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
			_UpdatePreview(NULL);
			break;	
		
		case kMsgControllerVideoDepthChanged:
			UpdateSettings();
			break;
			
		case kRebuildCodec:
		case kFileTypeChanged:
		{
			fController->SetMediaFileFormat(FileFormat());
			fController->SetMediaFormatFamily(FormatFamily());
			_SetFileNameExtension(FileFormat().file_extension);
			UpdateSettings();
		}
			break;
				
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
		
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32("be:observe_change_what", &code);
			switch (code) {
				case kSelectionWindowClosed:
				case kMsgControllerTargetFrameChanged:
				case kClipSizeChanged:
					_UpdatePreview(message);
					break;
				case kMsgControllerCodecListUpdated:
					_RebuildCodecsMenu();
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
OutputView::UpdateSettings()
{
	Settings settings;
	
	if (fWholeScreen->Value() == B_CONTROL_ON)
		settings.SetCaptureArea(BScreen().Frame());
			
	UpdatePreviewFromSettings();
	
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
	
	fRectView->SetRect(rect);
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


/* virtual */
BSize
OutputView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(620, 300));
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
OutputView::_UpdatePreview(BMessage* message)
{
	fSelectArea->SetEnabled(fCustomArea->Value() == B_CONTROL_ON);
	BRect screenFrame = BScreen(Window()).Frame();
	Settings settings;
	//BRect captureArea;
	//settings.GetCaptureArea(captureArea);
	//if (captureArea == screenFrame)
	//	return;
	if (fWholeScreen->Value() == B_CONTROL_ON)
		settings.SetCaptureArea(screenFrame);
	
	//UpdatePreview();
	if (message != NULL) {
		BRect rect;
		message->FindRect("selection", &rect);
		fRectView->SetRect(rect);
		
		BBitmap* bitmap = NULL;
		if (message->FindPointer("bitmap", (void**)&bitmap) == B_OK) {
			fRectView->UpdateBitmap(bitmap);
			delete bitmap;
		}
	} else {
		BRect captureArea;
		settings.GetCaptureArea(captureArea);
		fRectView->SetRect(captureArea);
		fRectView->UpdateBitmap(NULL);
	}
			
	// the size of the destination
	// clip maybe isn't supported by the codec	
	UpdateSettings();
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
