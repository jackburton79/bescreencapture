/*
 * Copyright 2017 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
 
#include "MediaFormatView.h"
#include "OptionsWindow.h"

#include <GroupLayoutBuilder.h>
#include <GroupView.h>
#include <LayoutBuilder.h>

const static BRect kWindowRect(0, 0, 300, 200);

OptionsWindow::OptionsWindow(Controller* controller)
	:
	BWindow(kWindowRect, "Options", B_TITLED_WINDOW,
		B_ASYNCHRONOUS_CONTROLS|B_AUTO_UPDATE_SIZE_LIMITS)
{
	BLayoutBuilder::Group<>(this, B_VERTICAL)
		.Add(new MediaFormatView(controller))
		.End();
	CenterOnScreen();
}


