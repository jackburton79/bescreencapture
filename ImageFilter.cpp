/*
 * Copyright 2018-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "ImageFilter.h"

#include <Bitmap.h>
#include <View.h>

ImageFilter::ImageFilter(BRect frame, color_space colorSpace)
	:
	fBitmap(NULL),
	fView(NULL)
{
	// Bitmap and view used to convert the source bitmap
	// to the correct size and depth
	fBitmap = new BBitmap(frame, colorSpace, true);
	fView = new BView(frame, "drawing view", B_FOLLOW_NONE, 0);
	if (fBitmap->Lock()) {
		fBitmap->AddChild(fView);
		fBitmap->Unlock();
	}
}


ImageFilter::~ImageFilter()
{
	delete fBitmap;
}


BBitmap*
ImageFilter::Bitmap()
{
	return fBitmap;
}


BView*
ImageFilter::View()
{
	return fView;
}


// ImageFilterScale
ImageFilterScale::ImageFilterScale(BRect frame, color_space colorSpace)
	:
	ImageFilter(frame, colorSpace)
{
}


ImageFilterScale::~ImageFilterScale()
{
}


/* virtual */
BBitmap*
ImageFilterScale::ApplyFilter(BBitmap* bitmap)
{
	// Draw scaled
	if (bitmap != NULL) {
		Bitmap()->Lock();
		View()->DrawBitmap(bitmap, bitmap->Bounds().OffsetToCopy(B_ORIGIN),
									View()->Bounds());
		View()->Sync();
		Bitmap()->Unlock();
		delete bitmap;
	}

	return new BBitmap(*Bitmap());
}
