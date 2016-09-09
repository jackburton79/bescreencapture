/*
 * Copyright 2016 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "InfoView.h"

#include <GroupLayoutBuilder.h>
#include <LayoutBuilder.h>
#include <StringView.h>

InfoView::InfoView(Controller* controller)
	:
	BView("Info", B_WILL_DRAW),
	fController(controller)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	BView* layoutView = BLayoutBuilder::Group<>()
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
		.SetInsets(B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING,
			B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
			.Add(fSourceSize = new BStringView("source size", ""))
			.Add(fClipSize = new BStringView("clip size", ""))
			.Add(fCodec = new BStringView("codec", ""))
		.End()
		.View();
	AddChild(layoutView);
}
