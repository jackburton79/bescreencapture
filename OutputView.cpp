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
#include <MediaDefs.h>
#include <MediaFormats.h>
#include <MenuField.h>
#include <MenuItem.h>
#include <OptionPopUp.h>
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
const static char *kCodecData = "Codec";

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
	fOutputFileType = new BOptionPopUp("OutFormat",
			kOutputMenuLabel, new BMessage(kFileTypeChanged));
						
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
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
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
	bool firstFound = true;
	while (get_next_file_format(&cookie, &mff) == B_OK) {
		if (mff.capabilities & media_file_format::B_KNOWS_ENCODED_VIDEO) {
			fOutputFileType->AddOption(mff.pretty_name, mff.family);
			if (firstFound) {
				fOutputFileType->MenuField()->Menu()->ItemAt(0)->SetMarked(true);
				firstFound = false;
			}
		}	
	}
	
	fWholeScreen->SetValue(B_CONTROL_ON);
	
	UpdateSettings();
	
	fController->SetCaptureArea(BScreen(Window()).Frame());
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
	fOutputFileType->SetTarget(this);
	fCustomArea->SetTarget(this);
	fWholeScreen->SetTarget(this);
	fFilePanelButton->SetTarget(this);
	
	UpdatePreviewFromSettings();
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
			fController->SetMediaFormatFamily(FormatFamily());
			UpdateSettings();
			break;
				
		case kFileNameChanged:
			fController->SetOutputFileName(fFileName->Text());
			break;
		
		case kMinimizeOnRecording:
			Settings().SetMinimizeOnRecording(fMinimizeOnStart->Value() == B_CONTROL_ON);
			break;
		
		case kCodecChanged:
		{
			media_codec_info *info;
			ssize_t size;
			if (message->FindData(kCodecData, B_SIMPLE_DATA,
					(const void**)&info, &size) == B_OK)
				fController->SetMediaCodecInfo(*info);
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
		
	BRect captureRect = settings.CaptureArea();
	
	UpdatePreviewFromSettings();
	
	BRect destFrame = GetScaledRect(captureRect, Settings().TargetSize());
	BuildCodecMenu(destFrame, FormatFamily());
	
	fController->SetMediaFormat(fFormat);
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
OutputView::BuildCodecMenu(const BRect& destFrame, const media_format_family &family)
{
	BRect rect = destFrame;
	rect.right++;
	rect.bottom++;
		
	GetCodecsForFamily(family, (const int32 &)rect.IntegerWidth(),
		(const int32 &)rect.IntegerHeight(), fCodecMenu->Menu(), fFormat);
		
	// Make the app object the menu's message target
	fCodecMenu->Menu()->SetTargetForItems(this);
}


status_t
OutputView::GetCodecsForFamily(const media_format_family &family,
					const int32 &width, const int32 &height,
					BMenu *codecs, media_format &initialFormat)
{
	SetInitialFormat(width, height, Settings().ClipDepth(),
		10, initialFormat);
	
	// find the full media_file_format corresponding to
	// the given format family (e.g. AVI)
	media_file_format fileFormat;
	if (!GetMediaFileFormat(family, fileFormat))
		return B_ERROR;
	
	BString currentCodec;
	BMenuItem *marked = codecs->FindMarked();
	if (marked != NULL)
		currentCodec = marked->Label();
	
	// suspend updates while we're rebuilding this menu in order to
	// reduce window flicker and other annoyances
	Window()->BeginViewTransaction();
		
	codecs->RemoveItems(0, codecs->CountItems(), true);
	
	int32 cookie = 0;
	media_codec_info codec;
	media_format dummyFormat;
	while (get_next_encoder(&cookie, &fileFormat, &initialFormat,
			&dummyFormat, &codec) == B_OK) {
		BMenuItem *item = CreateCodecMenuItem(codec);
		if (item != NULL)
			codecs->AddItem(item);
				
		if (codec.pretty_name == currentCodec)
			item->SetMarked(true);
	}
	
	if (codecs->FindMarked() == NULL) {
		BMenuItem *item = codecs->ItemAt(0);
		if (item != NULL)
			item->SetMarked(true);
	}
	
	Window()->EndViewTransaction();
	
	marked = codecs->FindMarked();
	BMessage *message = marked->Message();
	media_codec_info *info;
	ssize_t size;
	if (message->FindData(kCodecData, B_SIMPLE_DATA,
			(const void **)&info, &size) == B_OK)
		fController->SetMediaCodecInfo(*info);
		
	return B_OK;
}


/* virtual */
BSize
OutputView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(620, 300));
}


media_format_family
OutputView::FormatFamily() const
{
	return (media_format_family)fOutputFileType->Value();
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
	

/* static */
void
OutputView::SetInitialFormat(const int32 &width, const int32 &height,
	const color_space &colorSpace, const int32 &fieldRate,
	media_format &initialFormat)
{
	memset(&initialFormat, 0, sizeof(media_format));
		
	initialFormat.type = B_MEDIA_RAW_VIDEO;
	initialFormat.u.raw_video.display.line_width = width;
	initialFormat.u.raw_video.display.line_count = height;
	initialFormat.u.raw_video.last_active = initialFormat.u.raw_video.display.line_count - 1;
	
	size_t pixelChunk;
	size_t rowAlign;
	size_t pixelPerChunk;
	get_pixel_size_for(colorSpace, &pixelChunk, &rowAlign, &pixelPerChunk);
	initialFormat.u.raw_video.display.bytes_per_row = width * rowAlign;			
	initialFormat.u.raw_video.display.format = colorSpace;
	initialFormat.u.raw_video.interlace = 1;	
	
	// TODO: Calculate this in some way
	initialFormat.u.raw_video.field_rate = fieldRate; //Frames per second
	initialFormat.u.raw_video.pixel_width_aspect = 1;	// square pixels
	initialFormat.u.raw_video.pixel_height_aspect = 1;
	
}


/* static */
BMenuItem *
OutputView::CreateCodecMenuItem(const media_codec_info &codec)
{
	BMessage *message = new BMessage(kCodecChanged);
	if (message == NULL)
		return NULL;
	message->AddData(kCodecData, B_SIMPLE_DATA, &codec, sizeof(media_codec_info));
	BMenuItem *item = new BMenuItem(codec.pretty_name, message);
	return item;
}
