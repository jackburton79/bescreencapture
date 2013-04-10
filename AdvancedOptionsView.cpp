#include "AdvancedOptionsView.h"
#include "Controller.h"
#include "messages.h"
#include "PriorityControl.h"
#include "Settings.h"

#include <Box.h>
#include <CheckBox.h>
#include <DirectWindow.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Screen.h>
#include <Slider.h>
#include <String.h>
#include <StringView.h>

#include <cstdio>
#include <cstdlib>

const static uint32 kUseDirectWindow = 'UsDW';
const static uint32 kDepthChanged = 'DeCh';
const static uint32 kMsgTextControlSizeChanged = 'TCSC';

class SizeSlider : public BSlider {
public:
	SizeSlider(const char* name, const char* label,
		BMessage* message, int32 minValue,
		int32 maxValue, orientation posture,
		thumb_style thumbType = B_BLOCK_THUMB,
		uint32 flags = B_NAVIGABLE | B_WILL_DRAW
							| B_FRAME_EVENTS);

	virtual void SetValue(int32 value);
};


AdvancedOptionsView::AdvancedOptionsView(Controller *controller)
	:
	BView("Advanced", B_WILL_DRAW),
	fController(controller)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	
	BBox *clipSizeBox = new BBox("container");
	clipSizeBox->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_TOP));
	AddChild(clipSizeBox);
	clipSizeBox->SetLabel("Clip size");

	fSizeSlider = new SizeSlider("size_slider", "",
		new BMessage(kClipSizeChanged), 25, 200, B_HORIZONTAL);

	fSizeTextControl = new BTextControl("%", "", new BMessage(kMsgTextControlSizeChanged));
	fSizeTextControl->SetModificationMessage(new BMessage(kMsgTextControlSizeChanged));
	fSizeTextControl->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_TOP));
	fSizeTextControl->SetExplicitMaxSize(
		BSize(50, 25));
	fSizeTextControl->SetAlignment(B_ALIGN_RIGHT, B_ALIGN_LEFT);
	BView* sliderView = BLayoutBuilder::Group<>()
		.AddGroup(B_HORIZONTAL, 1)
			.Add(fSizeSlider)
			.Add(fSizeTextControl)
		.End()
		.View();
		
			
	clipSizeBox->AddChild(sliderView);

	fSizeSlider->SetValue(100);
	fSizeSlider->SetHashMarks(B_HASH_MARKS_BOTTOM);
	fSizeSlider->SetHashMarkCount(8);
	fSizeSlider->SetLimitLabels("25%", "200%");
	
	BBox *advancedBox = new BBox("Advanced");
	advancedBox->SetLabel("Advanced options");
	advancedBox->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_TOP));
	AddChild(advancedBox);
	
	BView* layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fUseDirectWindow = new BCheckBox("Use DW",
					"Use BDirectWindow (allows less CPU usage)",
					new BMessage(kUseDirectWindow)))
	
			.Add(fDepthControl = new BOptionPopUp("DepthControl", "Clip color depth:",
				new BMessage(kDepthChanged)))
			.Add(fPriorityControl = new PriorityControl("PriorityControl",
				"Encoding thread priority:"))
		.End()
		.View();
	
	advancedBox->AddChild(layoutView);
	
	if (static_cast<BDirectWindow *>(Window())->SupportsWindowMode()) {
		fUseDirectWindow->SetEnabled(true);
		fUseDirectWindow->SetValue(B_CONTROL_ON);
	} else
		fUseDirectWindow->SetEnabled(false);
		
	fDepthControl->AddOption("8 bit", B_CMAP8);
	fDepthControl->AddOption("15 bit", B_RGB15);
	fDepthControl->AddOption("16 bit", B_RGB16);
	fDepthControl->AddOption("32 bit", B_RGB32);
	fDepthControl->SelectOptionFor(B_RGB32);	
	fDepthControl->SetEnabled(false);
	
	Settings settings;	
	int32 priority = settings.EncodingThreadPriority();
	if (fPriorityControl->SelectOptionFor(priority) != B_OK)
		fPriorityControl->SetValue(0);
	
	settings.SetClipDepth(B_RGB32);
	fController->SetVideoDepth(B_RGB32);
	fController->SetUseDirectWindow(fUseDirectWindow->Value() == B_CONTROL_ON);
}


void
AdvancedOptionsView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fUseDirectWindow->SetTarget(this);
	fDepthControl->SetTarget(this);
	fPriorityControl->SetTarget(this);
	fSizeSlider->SetTarget(this);
	fSizeTextControl->SetTarget(this);
	fSizeSlider->SetValue(Settings().TargetSize());
}


void
AdvancedOptionsView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kUseDirectWindow:
			fController->SetUseDirectWindow(fUseDirectWindow->Value() == B_CONTROL_ON);
			break;
		
		case kPriorityChanged:
		{
			int32 value;
			const char *name = NULL;
			fPriorityControl->SelectedOption(&name, &value);
			Settings().SetEncodingThreadPriority(value);
			break;
		}
		
		case kDepthChanged:
		{
			color_space depth;
			const char *name = NULL;
			fDepthControl->SelectedOption(&name, (int32 *)&depth);
			fController->SetVideoDepth(depth);
			break;
		}
		
		case kClipSizeChanged:
		{
			float num = fSizeSlider->Value();
			
			BString sizeString;
			sizeString << (int32)num;
			fSizeTextControl->SetText(sizeString);
			Settings().SetTargetSize(num);
			
			SendNotices(kClipSizeChanged);
			
			break;
		}
		
		case kMsgTextControlSizeChanged:
		{
			int32 value = atoi(fSizeTextControl->TextView()->Text());
			fSizeSlider->SetValue(value);
			break;
		}
			
		default:
			BView::MessageReceived(message);
			break;
	}
}


BSize
AdvancedOptionsView::MinSize()
{
	return BLayoutUtils::ComposeSize(ExplicitMinSize(), BSize(300, 400));
}


// SizeSlider
SizeSlider::SizeSlider(const char* name, const char* label,
		BMessage* message, int32 minValue,
		int32 maxValue, orientation posture,
		thumb_style thumbType,
		uint32 flags)
	:
	BSlider(name, label, message, minValue, maxValue, posture, thumbType, flags)
{
}


/* virtual */
void
SizeSlider::SetValue(int32 value)
{
	// TODO: Not really, nice, should not have a fixed list of values
	const int32 validValues[] = { 25, 50, 75, 100, 125, 150, 175, 200 };
	int32 numValues = sizeof(validValues) / sizeof(int32);
	for (int32 i = 0; i < numValues - 1; i++) {
		if (value > validValues[i] && value < validValues[i + 1]) {
			value = value > validValues[i] + (validValues[i + 1] - validValues[i]) / 2
				? validValues[i + 1] : validValues[i];
		}
	}
	
	BSlider::SetValue(value);
}
