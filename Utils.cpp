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


void
PrintMediaFormat(const media_format& format)
{
	std::cout << "media format" << std::endl;
	std::cout << "width: " << format.Width() << std::endl;
	std::cout << "height: " << format.Height() << std::endl;
}


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
FixRect(BRect &rect, const BRect maxRect,
	const bool fixWidth, const bool fixHeight)
{
	const static int kAlignAmount = 16;
	
	if (rect.left < 0)
		rect.left = 0;
	if (rect.top < 0)
		rect.top = 0;
	if (rect.right > maxRect.right)
		rect.right = maxRect.right;
	if (rect.bottom > maxRect.bottom)
		rect.bottom = maxRect.bottom;

	// Adjust width and/or height to be a multiple of 16
	// as some codecs create bad movies otherwise
	int32 diffHorizontal = kAlignAmount - (rect.IntegerWidth() + 1) % kAlignAmount;
	if (fixWidth && diffHorizontal != kAlignAmount) { 
		if (rect.left < diffHorizontal) {
			diffHorizontal -= (int32)rect.left;
			rect.left = 0;
		} else {
			rect.left -= diffHorizontal;
			diffHorizontal = 0;
		}	
			
		rect.right += diffHorizontal;
	}
	
	int32 diffVertical = kAlignAmount - (rect.IntegerHeight() + 1) % kAlignAmount;
	if (fixHeight && diffVertical != kAlignAmount) { 
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
			if (info != NULL) {
				if (info->layer >= 3 && !info->is_mini && info->show_hide_level == 0) {
					BRect* rect = new BRect(info->window_left, info->window_top, info->window_right, info->window_bottom);
					rect->InsetBy(-border, -border);
					framesList.AddItem(rect);
				}
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
			if (info != NULL) {
				if (info->layer >= 3 && !info->is_mini && info->show_hide_level == 0) {
					BRect rect(info->window_left, info->window_top, info->window_right, info->window_bottom);
					if (rect == frame)
						token = tokenList[i];
				}
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

