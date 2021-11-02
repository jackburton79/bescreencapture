/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __UTILS_H
#define __UTILS_H

#include <MediaDefs.h>
#include <StorageDefs.h>
#include <SupportDefs.h>

#include <ObjectList.h>

class BPath;
class BRect;
class BString;
void PrintMediaFormat(const media_format& format);
bool IsFileFormatUsable(const media_file_format&);
bool IsFFMPEGAvailable();
bool GetMediaFileFormat(const BString& prettyName, media_file_format* outFormat);
void MakeGIFMediaFileFormat(media_file_format& outFormat);
void MakeNULLMediaFileFormat(media_file_format& outFormat);

BString GetUniqueFileName(const BString& fileName, const char *extension);
void FixRect(BRect &rect, const BRect& maxRect, const bool fixWidth = false, const bool fixHeight = false);

void GetWindowsFrameList(BObjectList<BRect> &framesList, int32 border = 0);
BRect GetWindowFrameForToken(int32 token, int32 border = 0);
int32 GetWindowTokenForFrame(BRect frame, int32 border = 0);

uint64 GetFreeMemory();


#endif // __UTILS_H
