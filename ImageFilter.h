/*
 * Copyright 2018, Your Name <your@email.address>
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
	BBitmap* fBitmap;
	BView* fView;
};


class ImageFilterScale : public ImageFilter {
public:
	ImageFilterScale(BRect frame, color_space colorSpace);
	virtual ~ImageFilterScale();

	virtual BBitmap* ApplyFilter(BBitmap* bitmap);
};

#endif // IMAGEFILTER_H
