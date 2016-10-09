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
			.Add(fBorderSlider = new SizeControl("border_slider", "Window border size",
					new BMessage(kWindowBorderFrameChanged), 0, 40, 1, "pixels", B_HORIZONTAL))
			.Add(fHideDeskbarIcon = new BCheckBox("hideDeskbar",
					"Hide Deskbar icon (ctrl+command+shift+r to stop recording)",
					new BMessage(kHideDeskbar)))
		.End()
		.View();
	
	advancedBox->AddChild(layoutView);
	
	if (static_cast<BDirectWindow *>(Window())->SupportsWindowMode()) {
		fUseDirectWindow->SetEnabled(true);
		fUseDirectWindow->SetValue(B_CONTROL_ON);
	} else
		fUseDirectWindow->SetEnabled(false);
	
	Settings settings;
	fController->SetVideoDepth(B_RGB32);
	fController->SetUseDirectWindow(fUseDirectWindow->Value() == B_CONTROL_ON);
}


void
AdvancedOptionsView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fUseDirectWindow->SetTarget(this);
	fHideDeskbarIcon->SetTarget(this);
	fBorderSlider->SetTarget(this);
	
	fBorderSlider->SetValue(Settings().WindowFrameBorderSize());
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
		
		case kWindowBorderFrameChanged:
		{
			const int32 size = fBorderSlider->Value();
			Settings().SetWindowFrameBorderSize(size);	
			break;
		}	
		default:
			BView::MessageReceived(message);
			break;
	}
}
