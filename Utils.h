#ifndef __UTILS_H
#define __UTILS_H

#include <MediaDefs.h>
#include <StorageDefs.h>
#include <SupportDefs.h>

class BList;
class BPath;
class BRect;


int32 BuildFileList(const BPath &path, BList &list);
void MakeUniqueName(const char *name, char *newName, size_t length = B_FILE_NAME_LENGTH);
void FixRect(BRect &rect, const bool fixWidth = false, const bool fixHeight = false);
bool GetMediaFileFormat(const media_format_family &family, media_file_format &fileFormat);
void UpdateMediaFormat(const int32 &width, const int32 &height,
	const color_space &colorSpace, const int32 &fieldRate,
	media_format &mediaFormat);

#endif

