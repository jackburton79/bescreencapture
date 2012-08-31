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
	SetLayout(new BGroupLayout(B_HORIZONTAL));
	
	BBox *advancedBox = new BBox("Advanced");
	advancedBox->SetLabel("Advanced options");
	advancedBox->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
		B_ALIGN_TOP));
	AddChild(advancedBox);
	
	BView *layoutView = BLayoutBuilder::Group<>()
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

