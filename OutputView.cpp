#include "ControllerObserver.h"
#include "messages.h"
#include "OutputView.h"
#include "PreviewView.h"
#include "Settings.h"
#include "Utils.h"

#include <Bitmap.h>
#include <Box.h>
#include <Button.h>
#include <CheckBox.h>
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

const static int32 kAreaSelectionChanged = 'CaCh';
const static int32 kFileTypeChanged = 'FtyC';
const static int32 kCodecChanged = 'CdCh';
const static char *kCodecData = "Codec";

OutputView::OutputView(Controller *controller)
	:
	BView("Capture Options", B_WILL_DRAW),
	fController(controller)
{
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	
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
		new BMessage(kAreaSelectionChanged));
	fCustomArea = new BRadioButton("custom area",
		"Custom Area", new BMessage(kAreaSelectionChanged));
	fSelectArea = new BButton("select area", "Select", new BMessage(kSelectArea));
	fSelectArea->SetEnabled(false);
	
	fMinimizeOnStart = new BCheckBox("Minimize on start",
		"Minimize on recording", new BMessage(kMinimizeOnRecording));
	
	fRectView = new PreviewView();
	
	BView *layoutView = BLayoutBuilder::Group<>()
		.SetInsets(15, 15, 15, 15)
		.AddGroup(B_VERTICAL, 15)
			.Add(fFileName)
			.Add(fOutputFileType)
			.Add(fCodecMenu)
			.Add(fMinimizeOnStart)
		.End()	
		.View();

	outputBox->AddChild(layoutView);

	layoutView = BLayoutBuilder::Group<>()
		.SetInsets(15, 15, 15, 15)
		.AddGroup(B_VERTICAL)
			.AddGroup(B_HORIZONTAL)
				.AddGroup(B_VERTICAL)
					.Add(fWholeScreen)
					.Add(fCustomArea)
				.End()
				.Add(fSelectArea)
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
	
	UpdatePreview();
}


void
OutputView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kMsgControllerCaptureFinished:
		case kMsgControllerCaptureStarted:
		case kMsgControllerEncodeStarted:
		case kMsgControllerEncodeFinished:
		
			break;
			
		case kAreaSelectionChanged:
		case kMsgControllerAreaSelectionChanged:
		{
			fSelectArea->SetEnabled(fCustomArea->Value() == B_CONTROL_ON);
			BRect screenFrame = BScreen(Window()).Frame();
			Settings settings;
			BRect captureArea;
			settings.GetCaptureArea(captureArea);
			if (captureArea == screenFrame)
				break;
			if (fWholeScreen->Value() == B_CONTROL_ON)
				settings.SetCaptureArea(screenFrame);
			
			UpdatePreview();
			
			// the size of the destination
			// clip maybe isn't supported by the codec	
			UpdateSettings();
			break;
		}	
		
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
					(const void **)&info, &size) == B_OK)
				fController->SetMediaCodecInfo(*info);
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
	const float factor = settings.ClipSize();
	settings.SetClipFrame(GetScaledRect(captureRect, factor));
	
	UpdatePreview();
	
	BuildCodecMenu(FormatFamily());
	
	fController->SetMediaFormat(fFormat);
}


BPath
OutputView::OutputFileName() const
{
	BPath path = fFileName->Text();
	return path;
}


void
OutputView::UpdatePreview()
{
	BRect rect;
	Settings settings;
	
	settings.GetCaptureArea(rect);
	
	fRectView->SetRect(rect);
}


void 
OutputView::BuildCodecMenu(const media_format_family &family)
{
	Settings settings;
	BRect rect = settings.ClipFrame();
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
