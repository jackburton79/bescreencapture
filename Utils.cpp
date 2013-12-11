#include "Settings.h"
#include "Utils.h"

#include <Entry.h>
#include <Path.h>
#include <String.h>

#include <cstring>
#include <cstdio>


void
MakeUniqueName(const char *name, char *newName, size_t length)
{
	int32 suffix = 1;
	BEntry entry(name);
	snprintf(newName, length, "%s", name);
	while (entry.Exists()) {
		snprintf(newName, length, "%s %ld", name, suffix);
		entry.SetTo(newName);
		suffix++;
	}
}


void
FixRect(BRect &rect, const bool fixWidth, const bool fixHeight)
{
	// Adjust width and/or height to be a multiple of 16
	// as some codecs create bad movies otherwise
	int32 diffHorizontal = 16 - (rect.IntegerWidth() + 1) % 16;
	if (fixWidth && diffHorizontal != 16) { 
		if (rect.left < diffHorizontal) {
			diffHorizontal -= (int32)rect.left;
			rect.left = 0;
		} else {
			rect.left -= diffHorizontal;
			diffHorizontal = 0;
		}	
			
		rect.right += diffHorizontal;
	}
	
	int32 diffVertical = 16 - (rect.IntegerHeight() + 1) % 16;
	if (fixHeight && diffVertical != 16) { 
		if (rect.top < diffVertical) {
			diffVertical -= (int32)rect.top;
			rect.top = 0;
		} else {
			rect.top -= diffVertical;
			diffVertical = 0;
		}	
			
		rect.bottom += diffVertical;
	}
}


bool
GetMediaFileFormat(const media_format_family &family,
	media_file_format &format)
{
	int32 cookie = 0;
	while (get_next_file_format(&cookie, &format) == B_OK) {
		if (format.family == family)
			return true;
	}
	
	memset(&format, 0, sizeof(format));
	
	return false;
}


void
UpdateMediaFormat(const int32 &width, const int32 &height,
	const color_space &colorSpace, const int32 &fieldRate,
	media_format &initialFormat)
{
	memset(&initialFormat, 0, sizeof(media_format));
		
	initialFormat.type = B_MEDIA_RAW_VIDEO;
	initialFormat.u.raw_video.display.line_width = width;
	initialFormat.u.raw_video.display.line_count = height;
	initialFormat.u.raw_video.last_active = initialFormat.u.raw_video.display.line_count - 1;
	
	size_t pixelChunk;
	size_t rowAlign;
	size_t pixelPerChunk;
	get_pixel_size_for(colorSpace, &pixelChunk, &rowAlign, &pixelPerChunk);
	initialFormat.u.raw_video.display.bytes_per_row = width * rowAlign;			
	initialFormat.u.raw_video.display.format = colorSpace;
	initialFormat.u.raw_video.interlace = 1;	
	
	// TODO: Calculate this in some way
	initialFormat.u.raw_video.field_rate = fieldRate; //Frames per second
	initialFormat.u.raw_video.pixel_width_aspect = 1;	// square pixels
	initialFormat.u.raw_video.pixel_height_aspect = 1;
	
}
