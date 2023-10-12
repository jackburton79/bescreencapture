/*
 * Copyright 2017-2023 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "OptionsWindow.h"

#include "FrameRateView.h"
#include "MediaFormatView.h"

#include <Box.h>
#include <GroupLayoutBuilder.h>
#include <GroupView.h>
#include <LayoutBuilder.h>

const static BRect kWindowRect(0, 0, 300, 200);

OptionsWindow::OptionsWindow()
	:
	BWindow(kWindowRect, "Encoding Settings", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS|B_AUTO_UPDATE_SIZE_LIMITS)
{
	BBox *encodingBox = new BBox("encoding options");
	BBox* frameBox = new BBox("frame rate");
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(encodingBox)
		.Add(frameBox)
		.End();

	encodingBox->SetLabel("Encoding options");
	frameBox->SetLabel("Frame rate");

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

	CenterOnScreen();
}
