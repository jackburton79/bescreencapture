/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __UTILS_H
#define __UTILS_H

#include <MediaDefs.h>
#include <StorageDefs.h>
#include <SupportDefs.h>

#include <vector>

typedef std::vector<BRect> rect_list;

class BPath;
class BRect;
class BString;
void PrintMediaFormat(const media_format& format);
bool IsFileFormatUsable(const media_file_format&);
bool IsFFMPEGAvailable();
bool GetMediaFileFormat(const BString& prettyName, media_file_format* outFormat);
void MakeGIFMediaFileFormat(media_file_format& outFormat);
void MakeNULLMediaFileFormat(media_file_format& outFormat);

BPath GetUniqueFileName(const BPath& fileName);
void FixRect(BRect &rect, const BRect& maxRect, const bool fixWidth = false, const bool fixHeight = false);

void GetWindowsFrameList(rect_list &framesList, int32 border = 0);
BRect GetWindowFrameForToken(int32 token, int32 border = 0);
int32 GetWindowTokenForFrame(BRect frame, int32 border = 0);

float CalculateFPS(const uint32& numFrames, const bigtime_t& elapsedTime);

uint64 GetFreeMemory();


#endif // __UTILS_H
