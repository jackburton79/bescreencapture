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
BRect GetScaledRect(const BRect &rect, const float factor);
void FixRect(BRect &rect, const bool fixWidth = false, const bool fixHeight = false);
bool GetMediaFileFormat(const media_format_family &family, media_file_format &fileFormat);


#endif

