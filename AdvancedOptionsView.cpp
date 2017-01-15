#include "AdvancedOptionsView.h"
#include "Controller.h"
#include "DeskbarControlView.h"
#include "FrameRateView.h"
#include "MediaFormatView.h"
#include "Messages.h"
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



AdvancedOptionsView::AdvancedOptionsView(Controller *controller)
	:
	BView("Advanced", B_WILL_DRAW),
	fController(controller)
{
	BGroupLayout* groupLayout = new BGroupLayout(B_VERTICAL);
	SetLayout(groupLayout);
	
	BBox *encodingBox = new BBox("encoding options");
	BBox* frameBox = new BBox("frame rate");
	BBox *advancedBox = new BBox("Advanced");
	
	/*BView* layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(encodingBox)
		.Add(frameBox)
		.Add(advancedBox)
		.View();
	
	AddChild(layoutView);
	*/
	encodingBox->SetLabel("Encoding options");
	frameBox->SetLabel("Frame rate");
	advancedBox->SetLabel("Advanced options");
	
	BView* layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(new MediaFormatView(controller))
		.View();
	
	encodingBox->AddChild(layoutView);
	
	layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(new FrameRateView(controller))
		.View();
	
	frameBox->AddChild(layoutView);
	
	//advancedBox->SetExplicitAlignment(BAlignment(B_ALIGN_HORIZONTAL_CENTER,
	//	B_ALIGN_TOP));
	AddChild(encodingBox);
	AddChild(frameBox);
	AddChild(advancedBox);
	
	layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fUseDirectWindow = new BCheckBox("Use DW",
					"Use BDirectWindow (allows less CPU usage)",
					new BMessage(kUseDirectWindow)))
			.Add(fDepthControl = new BOptionPopUp("DepthControl", "Clip color depth:",
				new BMessage(kDepthChanged)))
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
		
	fDepthControl->AddOption("8 bit", B_CMAP8);
	fDepthControl->AddOption("15 bit", B_RGB15);
	fDepthControl->AddOption("16 bit", B_RGB16);
	fDepthControl->AddOption("32 bit", B_RGB32);
	fDepthControl->SelectOptionFor(B_RGB32);	
	fDepthControl->SetEnabled(false);
	
	/*Settings settings;	
	int32 priority = settings.EncodingThreadPriority();
	if (fPriorityControl->SelectOptionFor(priority) != B_OK)
		fPriorityControl->SetValue(0);*/
	
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
	//fPriorityControl->SetTarget(this);
	
	fHideDeskbarIcon->SetTarget(this);
	
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
		
/*		case kPriorityChanged:
		{
			int32 value;
			const char *name = NULL;
			fPriorityControl->SelectedOption(&name, &value);
			Settings().SetEncodingThreadPriority(value);
			break;
		}*/
		
		case kHideDeskbar:
		{
			Settings().SetHideDeskbarIcon(fHideDeskbarIcon->Value() == B_CONTROL_ON);
			BDeskbar deskbar;
			if (deskbar.IsRunning()) {
				if (fHideDeskbarIcon->Value() == B_CONTROL_ON) { 
					while (deskbar.HasItem("BSC Control"))
						deskbar.RemoveItem("BSC Control");
				} else if (!deskbar.HasItem("BSC Control")) {
					deskbar.AddItem(new DeskbarControlView(BRect(0, 0, 15, 15),
						"BSC Control"));
				}
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
		

		default:
			BView::MessageReceived(message);
			break;
	}
}
