/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "Utils.h"

#include "Constants.h"
#include "Settings.h"

// Private Haiku header
#include "WindowInfo.h"

#include <Directory.h>
#include <Entry.h>
#include <Path.h>
#include <String.h>
#include <Window.h>

#include <cstring>
#include <cstdio>
#include <cstdlib>

#include <iostream>

// TODO: Refactor

void
PrintMediaFormat(const media_format& format)
{
	std::cout << "media format" << std::endl;
	std::cout << "width: " << format.Width() << std::endl;
	std::cout << "height: " << format.Height() << std::endl;
}


bool
IsFileFormatUsable(const media_file_format& format)
{
	const uint32 neededMask = media_file_format::B_KNOWS_ENCODED_VIDEO
								| media_file_format::B_WRITABLE;

	if ((format.capabilities & neededMask) != neededMask)
		return false;

	// Known broken file formats in Haiku
	static BString brokenFileFormats[] = {
		"DV Movie",
		"Ogg Audio",
		"Ogg Video",
		// TODO: blacklist AVI and Matroska since, as today, Haiku can't encode
		// these formats correctly. Remove when it's fixed
		"AVI",
		"Matroska"
	};
	
	BString formatString = format.pretty_name;
	for (size_t i = 0; i < sizeof(brokenFileFormats) / sizeof(brokenFileFormats[i]); i++) {
		if (formatString.StartsWith(brokenFileFormats[i]))
			return false;
	};

	return true;
}


bool
IsFFMPEGAvailable()
{
	int result = system("type ffmpeg > /dev/null 2>&1");
	return WEXITSTATUS(result) == 0;
}


// If prettyName is NULL, returns the first found media_file_format,
// otherwise returns the media_file_format with the same prettyName
// TODO: Use the short_name instead
bool
GetMediaFileFormat(const BString& prettyName, media_file_format* outFormat)
{
	media_file_format mediaFileFormat;
	int32 cookie = 0;
	while (get_next_file_format(&cookie, &mediaFileFormat) == B_OK) {
		if (!IsFileFormatUsable(mediaFileFormat))
			continue;
		if (prettyName == "" || prettyName == mediaFileFormat.pretty_name) {
			*outFormat = mediaFileFormat;
			return true;
		}
	}
	
	if (prettyName == NULL_FORMAT_PRETTY_NAME) {
		MakeNULLMediaFileFormat(*outFormat);
		return true;
	} else if (prettyName == GIF_FORMAT_PRETTY_NAME) {
		MakeGIFMediaFileFormat(*outFormat);
		return true;
	}
	return false;
}


void
MakeGIFMediaFileFormat(media_file_format& outFormat)
{
	::strlcpy(outFormat.pretty_name, "GIF", sizeof(outFormat.pretty_name));
	::strlcpy(outFormat.short_name, GIF_FORMAT_SHORT_NAME, sizeof(outFormat.short_name));
	outFormat.capabilities = media_file_format::B_KNOWS_OTHER;
	::strlcpy(outFormat.file_extension, "gif", sizeof(outFormat.file_extension));
	outFormat.family = B_ANY_FORMAT_FAMILY;
}


void
MakeNULLMediaFileFormat(media_file_format& outFormat)
{
	::strlcpy(outFormat.pretty_name, "Export frames as Bitmaps", sizeof(outFormat.pretty_name));
	::strlcpy(outFormat.short_name, NULL_FORMAT_SHORT_NAME, sizeof(outFormat.short_name));
	outFormat.capabilities = media_file_format::B_KNOWS_OTHER;
	::strlcpy(outFormat.file_extension, "png", sizeof(outFormat.file_extension));
	outFormat.family = B_ANY_FORMAT_FAMILY;
}


BPath
GetUniqueFileName(const BPath& filePath)
{
	BString extension = filePath.Leaf();
	int32 extensionIndex = extension.FindLast(".");
	if (extensionIndex != -1)
		extension.Remove(0, extensionIndex);

	char timeSuffix[128];
	time_t currentTime = ::time(NULL);
	struct tm timeStruct;
	struct tm* localTime = ::localtime_r(&currentTime, &timeStruct);
	strftime(timeSuffix, sizeof(timeSuffix), "%Y-%m-%d", localTime);

	BPath parent;
	filePath.GetParent(&parent);
	BEntry parentEntry(parent.Path());
	BDirectory directory(&parentEntry);
	BEntry newEntry(filePath.Path());
	BString newName = filePath.Leaf();
	int32 suffix = 0;
	while (BEntry(&directory, newName).Exists()) {
		newName = filePath.Leaf();
		// remove extension (if any)
		if (extension != "")
			newName.RemoveLast(extension);
		// remove suffix
		newName.RemoveLast(timeSuffix);
		newName.RemoveLast("-");

		// add back time suffix
		newName << '-' << timeSuffix;

		// add suffix
		if (suffix != 0)
			newName << "-" << suffix;
		// add back extension
		if (extension != "")
			newName	<< extension;
		newEntry.SetTo(&directory, newName);
		suffix++;
	}

	return BPath(&newEntry);
}


void
FixRect(BRect &rect, const BRect& maxRect,
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
				::free(info);
			}
		}
		::free(tokenList);
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
				::free(info);
			}
		}
		::free(tokenList);
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
		::free(info);
	}
	return rect;
}


float
CalculateFPS(const uint32& numFrames, const bigtime_t& elapsedTime)
{
	return float(1000000 * uint64(numFrames)) / elapsedTime;
}


uint64
GetFreeMemory()
{
	system_info info;
	if (get_system_info(&info) != B_OK)
		return 0;

	return info.free_memory;
}
