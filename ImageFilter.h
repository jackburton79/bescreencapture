/*
 * Copyright 2018-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef IMAGEFILTER_H
#define IMAGEFILTER_H

#include <GraphicsDefs.h>
#include <Rect.h>
#include <SupportDefs.h>

class BBitmap;
class BView;
class ImageFilter {
public:
	ImageFilter(BRect frame, color_space colorSpace);
	virtual ~ImageFilter();

	virtual BBitmap* ApplyFilter(BBitmap* bitmap) = 0;

protected:
	BBitmap* Bitmap();
	BView* View();

private:
	BBitmap* fBitmap;
	BView* fView;

	ImageFilter& operator=(const ImageFilter& other) = delete ;
	ImageFilter(const ImageFilter& other) = delete ;
};


class ImageFilterScale : public ImageFilter {
public:
	ImageFilterScale(BRect frame, color_space colorSpace);
	virtual ~ImageFilterScale();

	virtual BBitmap* ApplyFilter(BBitmap* bitmap);
};

#endif // IMAGEFILTER_H
