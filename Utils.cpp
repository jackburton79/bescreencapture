#include "Settings.h"
#include "Utils.h"

// Private Haiku header
#include "WindowInfo.h"


#include <Entry.h>
#include <Path.h>
#include <String.h>
#include <Window.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <iostream>


BString
GetUniqueFileName(const BString fileName, const char *extension)
{
	BString newName = fileName;
	int32 suffix = 1;
	BEntry entry(newName);
	while (entry.Exists()) {
		newName = fileName;
		newName.RemoveLast(extension);
		newName.RemoveLast(".");
		newName << '_' << suffix << '.' << extension;		
		entry.SetTo(newName);
		suffix++;
	}

	return newName;
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


status_t
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

	status_t status;
	status = get_pixel_size_for(colorSpace, &pixelChunk,
			&rowAlign, &pixelPerChunk);
	if (status != B_OK)
		return status;

	initialFormat.u.raw_video.display.bytes_per_row = width * rowAlign;			
	initialFormat.u.raw_video.display.format = colorSpace;
	initialFormat.u.raw_video.interlace = 1;	
	
	// TODO: Calculate this in some way
	initialFormat.u.raw_video.field_rate = fieldRate; //Frames per second
	initialFormat.u.raw_video.pixel_width_aspect = 1;	// square pixels
	initialFormat.u.raw_video.pixel_height_aspect = 1;
	
	return B_OK;
}



const int kFrameBorderSize = 10;


void
GetWindowsFrameList(BObjectList<BRect> &framesList, int32 border)
{
	int32 tokenCount = 0;
	// Haiku does not have a public API to retrieve windows from other teams, 
	// so we have to use this.
	int32 *tokenList = NULL;
	status_t status = BPrivate::get_window_order(current_workspace(), &tokenList, &tokenCount);
	if (status == B_OK) {
		for (int32 i = 0; i < tokenCount; i++) {
			client_window_info* info = get_window_info(tokenList[i]);
			if (info != NULL && info->layer >= 3 && !info->is_mini && info->show_hide_level == 0) {
				BRect* rect = new BRect(info->window_left, info->window_top, info->window_right, info->window_bottom);
				rect->InsetBy(-border, -border);
				framesList.AddItem(rect);
				free(info);
			}
		}
		free(tokenList);
	}
}


int32
GetWindowTokenForFrame(BRect frame, int32 border)
{
	int32 tokenCount = 0;
	int32 *tokenList = NULL;
	int32 token = -1;
	frame.InsetBy(border, border);
	status_t status = BPrivate::get_window_order(current_workspace(), &tokenList, &tokenCount);
	if (status == B_OK) {
		for (int32 i = 0; i < tokenCount && token == -1; i++) {
			client_window_info* info = get_window_info(tokenList[i]);
			if (info != NULL && info->layer >= 3 && !info->is_mini && info->show_hide_level == 0) {
				BRect rect(info->window_left, info->window_top, info->window_right, info->window_bottom);
				if (rect == frame)
					token = tokenList[i];
				free(info);
			}
		}
		free(tokenList);
	}
	
	return token;
}


BRect
GetWindowFrameForToken(int32 token, int32 border)
{
	BRect rect;
	client_window_info* info = get_window_info(token);
	if (info != NULL) {
		rect.Set(info->window_left, info->window_top, info->window_right, info->window_bottom);
		rect.InsetBy(-border, -border);
		free(info);
	}
	return rect;
}

