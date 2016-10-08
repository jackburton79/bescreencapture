#include "AdvancedOptionsView.h"
#include "Controller.h"
#include "DeskbarControlView.h"
#include "messages.h"
#include "PriorityControl.h"
#include "Settings.h"
#include "SizeControl.h"

#include <Box.h>
#include <CheckBox.h>
#include <Deskbar.h>
#include <DirectWindow.h>
#include <GroupLayout.h>
#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <LayoutUtils.h>
#include <Screen.h>
#include <String.h>
#include <StringView.h>

#include <cstdio>
#include <cstdlib>

const static uint32 kUseDirectWindow = 'UsDW';
const static uint32 kDepthChanged = 'DeCh';
const static uint32 kHideDeskbar = 'HiDe';
const static uint32 kWindowBorderFrameChanged = 'WbFc';


AdvancedOptionsView::AdvancedOptionsView(Controller *controller)
	:
	BView("Advanced", B_WILL_DRAW),
	fController(controller)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	
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
			.Add(fHideDeskbarIcon = new BCheckBox("hideDeskbar",
					"Hide Deskbar icon (ctrl+command+shift+r to stop recording)",
					new BMessage(kHideDeskbar)))
			.Add(fBorderSlider = new SizeControl("border_slider", "Window border size",
					new BMessage(kWindowBorderFrameChanged), 0, 40, 1, "pixels", B_HORIZONTAL))
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
	
	fHideDeskbarIcon->SetTarget(this);
	fBorderSlider->SetTarget(this);
	
	fHideDeskbarIcon->SetValue(Settings().HideDeskbarIcon()
		? B_CONTROL_ON : B_CONTROL_OFF);
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
		
		case kHideDeskbar:
		{
			Settings().SetHideDeskbarIcon(fHideDeskbarIcon->Value() == B_CONTROL_ON);
			BDeskbar deskbar;
			if (deskbar.IsRunning()) { 
				while (deskbar.HasItem("BSC Control")
					&& fHideDeskbarIcon->Value() == B_CONTROL_ON)
					deskbar.RemoveItem("BSC Control");
				if(fHideDeskbarIcon->Value() != B_CONTROL_ON
					&& !deskbar.HasItem("BSC Control"))
					deskbar.AddItem(new DeskbarControlView(BRect(0, 0, 15, 15),
						"BSC Control"));
			}
			break;
		}
		
		case kDepthChanged:
		{
			color_space depth;
			const char *name = NULL;
			fDepthControl->SelectedOption(&name, (int32*)&depth);
			fController->SetVideoDepth(depth);
			break;
		}
		
		case kWindowBorderFrameChanged:
			break;
			
		default:
			BView::MessageReceived(message);
			break;
	}
}
