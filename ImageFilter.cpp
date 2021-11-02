/*
 * Copyright 2018-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */


#include "ImageFilter.h"

#include <Bitmap.h>
#include <View.h>

ImageFilter::ImageFilter(BRect frame, color_space colorSpace)
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


ImageFilter::ImageFilter(const ImageFilter& other)
{
}


ImageFilter::~ImageFilter()
{
	delete fBitmap;
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
		fBitmap->Lock();
		fView->DrawBitmap(bitmap, bitmap->Bounds().OffsetToCopy(B_ORIGIN), fView->Bounds());
		fView->Sync();
		fBitmap->Unlock();
	}

	delete bitmap;
	
	return new BBitmap(*fBitmap);
}
