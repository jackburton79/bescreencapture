/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "AdvancedOptionsView.h"

#include "BSCApp.h"
#include "Constants.h"
#include "ControllerObserver.h"
#include "DeskbarControlView.h"
#include "FrameRateView.h"
#include "MediaFormatView.h"
#include "Settings.h"

#include <Box.h>
#include <Catalog.h>
#include <CheckBox.h>
#include <LayoutBuilder.h>


const static uint32 kLocalUseDirectWindow = 'UsDW';
const static uint32 kLocalHideDeskbar = 'HiDe';
const static uint32 kLocalEnableShortcut = 'EnSh';
const static uint32 kLocalSelectOnStart = 'SeSt';
const static uint32 kLocalMinimizeOnRecording = 'MiRe';
const static uint32 kLocalQuitWhenFinished = 'QuFi';


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "AdvancedOptionsView"


AdvancedOptionsView::AdvancedOptionsView()
	:
	BView("options", B_WILL_DRAW)
{
	BGroupLayout* groupLayout = new BGroupLayout(B_VERTICAL);
	SetLayout(groupLayout);

	BBox *encodingBox = new BBox("encoding_options");
	BBox* frameBox = new BBox("frame_rate");
	BBox *advancedBox = new BBox("advanced");

	encodingBox->SetLabel(B_TRANSLATE("Encoding options"));
	frameBox->SetLabel(B_TRANSLATE("Frame rate"));
	advancedBox->SetLabel(B_TRANSLATE("Advanced"));

	BView* layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(new MediaFormatView())
		.View();

	encodingBox->AddChild(layoutView);

	layoutView = BLayoutBuilder::Group<>()
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(new FrameRateView())
		.View();

	frameBox->AddChild(layoutView);

	AddChild(encodingBox);
	AddChild(frameBox);
	AddChild(advancedBox);

	layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
				B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fUseDirectWindow = new BCheckBox("use_dw",
					B_TRANSLATE("Use less CPU (BDirectWindow)"),
					new BMessage(kLocalUseDirectWindow)))
			.Add(fMinimizeOnStart = new BCheckBox("hide_when_Recording",
					B_TRANSLATE("Hide window when recording"),
					new BMessage(kLocalMinimizeOnRecording)))
			.Add(fHideDeskbarIcon = new BCheckBox("hide_deskbar",
					B_TRANSLATE("Incognito mode: Hide window and Deskbar tray icon"),
					new BMessage(kLocalHideDeskbar)))
			.AddGroup(B_HORIZONTAL, B_USE_DEFAULT_SPACING)
				.Add(fUseShortcut = new BCheckBox("use_shortcut",
					B_TRANSLATE("Enable CTRL+ALT+SHIFT+R shortcut"),
					new BMessage(kLocalEnableShortcut)))
				.Add(fSelectOnStart = new BCheckBox("select_on_start",
					B_TRANSLATE("Select a region on start"),
					new BMessage(kLocalSelectOnStart)))
			.End()
			.Add(fQuitWhenFinished = new BCheckBox("quit_when_finished",
					B_TRANSLATE("Quit when finished"),
					new BMessage(kLocalQuitWhenFinished)))
		.End()
		.View();

	fHideDeskbarIcon->SetToolTip(B_TRANSLATE(
		"Stop recording with with CTRL+ALT+SHIFT+R,\n"
		"or define a key combination with the Shortcuts preferences."));

	advancedBox->AddChild(layoutView);

	_EnableDirectWindowIfSupported();

	BSCApp* app = dynamic_cast<BSCApp*>(be_app);
	app->SetVideoDepth(B_RGB32);
	app->SetUseDirectWindow(fUseDirectWindow->Value() == B_CONTROL_ON);

	const Settings& settings = Settings::Current();
	fMinimizeOnStart->SetValue(settings.MinimizeOnRecording() ? B_CONTROL_ON : B_CONTROL_OFF);
	if (settings.EnableShortcut()) {
		fHideDeskbarIcon->SetEnabled(true);
		fHideDeskbarIcon->SetValue(settings.HideDeskbarIcon() ? B_CONTROL_ON : B_CONTROL_OFF);
	} else {
		fHideDeskbarIcon->SetEnabled(false);
		fHideDeskbarIcon->SetValue(B_CONTROL_OFF);
	}
	fQuitWhenFinished->SetValue(settings.QuitWhenFinished() ? B_CONTROL_ON : B_CONTROL_OFF);
	fSelectOnStart->SetEnabled(settings.EnableShortcut());
	fUseShortcut->SetValue(settings.EnableShortcut() ? B_CONTROL_ON : B_CONTROL_OFF);
	fSelectOnStart->SetValue(settings.SelectOnStart() ? B_CONTROL_ON : B_CONTROL_OFF);
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
	fUseShortcut->SetTarget(this);
	fSelectOnStart->SetTarget(this);
	fQuitWhenFinished->SetTarget(this);
}


/* virtual */
void
AdvancedOptionsView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kLocalUseDirectWindow:
		{
			BSCApp* app = dynamic_cast<BSCApp*>(be_app);
			app->SetUseDirectWindow(fUseDirectWindow->Value() == B_CONTROL_ON);
			break;
		}
		case kLocalHideDeskbar:
		{
			bool hide = fHideDeskbarIcon->Value() == B_CONTROL_ON;
			Settings::Current().SetHideDeskbarIcon(hide);
			if (hide) {
				// Save the current minimize setting, since we are going to override
				// it when "hide deskbar" is set
				fCurrentMinimizeValue = Settings::Current().MinimizeOnRecording();
				fMinimizeOnStart->SetValue(B_CONTROL_ON);
				fMinimizeOnStart->SetEnabled(false);
				BSCApp* app = dynamic_cast<BSCApp*>(be_app);
				if (app != NULL)
					app->RemoveDeskbarReplicant();
			} else {
				fMinimizeOnStart->SetValue(fCurrentMinimizeValue ? B_CONTROL_ON : B_CONTROL_OFF);
				fMinimizeOnStart->SetEnabled(true);
				BSCApp* app = dynamic_cast<BSCApp*>(be_app);
				if (app != NULL)
					app->InstallDeskbarReplicant();
			}
		}
		// Fall through
		case kLocalMinimizeOnRecording:
			Settings::Current().SetMinimizeOnRecording(fMinimizeOnStart->Value() == B_CONTROL_ON);
			break;

		case kLocalQuitWhenFinished:
			Settings::Current().SetQuitWhenFinished(fQuitWhenFinished->Value() == B_CONTROL_ON);
			break;

		case kLocalEnableShortcut:
			Settings::Current().SetEnableShortcut(fUseShortcut->Value() == B_CONTROL_ON);
			fSelectOnStart->SetEnabled(fUseShortcut->Value() == B_CONTROL_ON);
			if (Settings::Current().EnableShortcut()) {
				fHideDeskbarIcon->SetEnabled(true);
			} else {
				fHideDeskbarIcon->SetEnabled(false);
				fHideDeskbarIcon->SetValue(B_CONTROL_OFF);
			}
			// fall through

		case kLocalSelectOnStart:
			Settings::Current().SetSelectOnStart(fSelectOnStart->Value() == B_CONTROL_ON);
			// Save settings immediately, so the input server addon picks the change
			Settings::Current().Save();
			break;

		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
			switch (code) {
				case kMsgControllerResetSettings:
				{
					fCurrentMinimizeValue = Settings::Current().MinimizeOnRecording();
					fMinimizeOnStart->SetValue(fCurrentMinimizeValue ? B_CONTROL_ON : B_CONTROL_OFF);
					fMinimizeOnStart->SetEnabled(true);
					fUseShortcut->SetValue(B_CONTROL_OFF);
					fSelectOnStart->SetValue(B_CONTROL_OFF);
					fSelectOnStart->SetEnabled(false);
					fHideDeskbarIcon->SetValue(B_CONTROL_OFF);
					fQuitWhenFinished->SetValue(B_CONTROL_OFF);
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
