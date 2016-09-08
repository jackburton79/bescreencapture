#ifndef __UTILS_H
#define __UTILS_H

#include <MediaDefs.h>
#include <StorageDefs.h>
#include <SupportDefs.h>

#include <ObjectList.h>

class BPath;
class BRect;

void MakeUniqueName(const char *name, char *newName, size_t length = B_FILE_NAME_LENGTH);
void FixRect(BRect &rect, const bool fixWidth = false, const bool fixHeight = false);
status_t UpdateMediaFormat(const int32 &width, const int32 &height,
	const color_space &colorSpace, const int32 &fieldRate,
	media_format &mediaFormat);

status_t GetWindowsFrameList(BObjectList<BRect> &framesList);

#endif

