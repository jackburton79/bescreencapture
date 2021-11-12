/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "AdvancedOptionsView.h"

#include "Constants.h"
#include "Controller.h"
#include "ControllerObserver.h"
#include "DeskbarControlView.h"
#include "FrameRateView.h"
#include "MediaFormatView.h"
#include "Settings.h"

#include <Box.h>
#include <CheckBox.h>
#include <Deskbar.h>
#include <LayoutBuilder.h>


const static uint32 kLocalUseDirectWindow = 'UsDW';
const static uint32 kLocalHideDeskbar = 'HiDe';
const static uint32 kLocalMinimizeOnRecording = 'MiRe';


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
	
	encodingBox->SetLabel("Encoding options");
	frameBox->SetLabel("Frame rate");
	advancedBox->SetLabel("Advanced");
	
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
	
	AddChild(encodingBox);
	AddChild(frameBox);
	AddChild(advancedBox);
	
	layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fUseDirectWindow = new BCheckBox("Use DW",
					"Use less CPU (BDirectWindow)",
					new BMessage(kLocalUseDirectWindow)))
			.Add(fMinimizeOnStart = new BCheckBox("HideWhenRecording",
				"Hide window when recording", new BMessage(kLocalMinimizeOnRecording)))
			.Add(fHideDeskbarIcon = new BCheckBox("hideDeskbar",
					"Incognito mode: Hide window and Deskbar icon", new BMessage(kLocalHideDeskbar)))
		.End()
		.View();
	
	fHideDeskbarIcon->SetToolTip("Stop recording with with CTRL+ALT+SHIFT+R,\n"
								"or define a key combination with the Shortcuts preferences");
	
	advancedBox->AddChild(layoutView);

	_EnableDirectWindowIfSupported();

	fController->SetVideoDepth(B_RGB32);
	fController->SetUseDirectWindow(fUseDirectWindow->Value() == B_CONTROL_ON);
	
	const Settings& settings = Settings::Current();
	fMinimizeOnStart->SetValue(settings.MinimizeOnRecording() ? B_CONTROL_ON : B_CONTROL_OFF);
	fHideDeskbarIcon->SetValue(B_CONTROL_OFF);
}


/* virtual */
void
AdvancedOptionsView::AttachedToWindow()
{
	BView::AttachedToWindow();
	
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	fUseDirectWindow->SetTarget(this);
	fMinimizeOnStart->SetTarget(this);
	fHideDeskbarIcon->SetTarget(this);
}


/* virtual */
void
AdvancedOptionsView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kLocalUseDirectWindow:
			fController->SetUseDirectWindow(fUseDirectWindow->Value() == B_CONTROL_ON);
			break;
		
		case kLocalHideDeskbar:
		{
			bool hide = fHideDeskbarIcon->Value() == B_CONTROL_ON;
			BDeskbar deskbar;
			if (deskbar.IsRunning()) {
				if (hide) {
					// Save the current minimize setting, since we are going to override
					// it when "hide deskbar" is set
					fCurrentMinimizeValue = Settings::Current().MinimizeOnRecording();
					fMinimizeOnStart->SetValue(B_CONTROL_ON);
					fMinimizeOnStart->SetEnabled(false);
					while (deskbar.HasItem(BSC_DESKBAR_VIEW))
						deskbar.RemoveItem(BSC_DESKBAR_VIEW);
				} else if (!deskbar.HasItem(BSC_DESKBAR_VIEW)) {
					fMinimizeOnStart->SetValue(fCurrentMinimizeValue ? B_CONTROL_ON : B_CONTROL_OFF);
					fMinimizeOnStart->SetEnabled(true);
					deskbar.AddItem(new DeskbarControlView(BRect(0, 0, 15, 15),
						BSC_DESKBAR_VIEW));
				}
			}
		}
		// Fall through
		case kLocalMinimizeOnRecording:
			Settings::Current().SetMinimizeOnRecording(fMinimizeOnStart->Value() == B_CONTROL_ON);
			break;
		
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32("be:observe_change_what", &code);
			switch (code) {
				case kMsgControllerResetSettings:
				{
					fCurrentMinimizeValue = Settings::Current().MinimizeOnRecording();
					fMinimizeOnStart->SetValue(fCurrentMinimizeValue ? B_CONTROL_ON : B_CONTROL_OFF);
					fMinimizeOnStart->SetEnabled(true);
					fHideDeskbarIcon->SetValue(B_CONTROL_OFF);
					_EnableDirectWindowIfSupported();
					break;
				}
				default:
					break;
			}
			break;
		}
		default:
			BView::MessageReceived(message);
			break;
	}
}


void
AdvancedOptionsView::_EnableDirectWindowIfSupported()
{
	if (static_cast<BDirectWindow *>(Window())->SupportsWindowMode()) {
		fUseDirectWindow->SetEnabled(true);
		fUseDirectWindow->SetValue(B_CONTROL_ON);
	} else
		fUseDirectWindow->SetEnabled(false);
}
