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
#include <RadioButton.h>
#include <Screen.h>
#include <StringView.h>

#include <cstdio>
#include <cstdlib>

const static uint32 kUseDirectWindow = 'UsDW';
const static uint32 kDepthChanged = 'DeCh';


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
	
	BRadioButton *normalSizeRB 
		= new BRadioButton("100 Original", "100\% (Original size)",
			new BMessage(kClipSizeChanged));
			
	BView *layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(new BRadioButton("200", "200\%", new BMessage(kClipSizeChanged)))
			.Add(normalSizeRB)
			.Add(new BRadioButton("50", "50\%", new BMessage(kClipSizeChanged)))
			.Add(new BRadioButton("25", "25\%", new BMessage(kClipSizeChanged)))
		.End()
		.View();
	
	clipSizeBox->AddChild(layoutView);

	normalSizeRB->SetValue(B_CONTROL_ON);
	
	BBox *advancedBox = new BBox("Advanced");
	advancedBox->SetLabel("Advanced options");
	advancedBox->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_TOP));
	AddChild(advancedBox);
	
	layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fUseDirectWindow = new BCheckBox("Use DW",
					"Use BDirectWindow (allows less CPU usage)",
					new BMessage(kUseDirectWindow)))
			//.Add(new BCheckBox("FixWidth", "Fix clip width",
			//	new BMessage))
			//.Add(new BCheckBox("FixHeight", "Fix clip height",
			//	new BMessage))
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
	fUseDirectWindow->SetTarget(this);
	fDepthControl->SetTarget(this);
	fPriorityControl->SetTarget(this);
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BView *container = FindView("container");
	if (container != NULL) {
		BControl *child = dynamic_cast<BControl *>(container->ChildAt(0));
		while (child != NULL) {
			child->SetTarget(this);
			child = dynamic_cast<BControl *>(child->NextSibling());
		}
	}
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
			BRadioButton *source = NULL;
			message->FindPointer("source", (void **)&source);
			if (source != NULL) {
				float num;
				sscanf(source->Name(), "%1f", &num);
				Settings().SetClipSize(num);
				Window()->PostMessage(kClipSizeChanged);
			}
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

