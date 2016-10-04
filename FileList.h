/*
 * FileList.h
 *
 *  Created on: 11/dic/2013
 *      Author: Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#ifndef __FILELIST_H_
#define __FILELIST_H_

#include <ObjectList.h>
#include <String.h>

class BBitmap;
class BitmapEntry {
public:
	BitmapEntry();
	BitmapEntry(BBitmap* bitmap);
	BitmapEntry(const BitmapEntry&);
	
	~BitmapEntry();
	
	BBitmap* Bitmap();
	
	void SaveToDisk(const char* path);
	
	BString file_name;
	bigtime_t frame_time;

private:
	BBitmap* fBitmap;
};


class FileList : private BObjectList<BitmapEntry> {
public:
	FileList();
	virtual ~FileList();

	void AddItem(BBitmap* bitmap, bigtime_t frameTime);
	void AddItem(const BString& fileName, bigtime_t frameTime);

	BitmapEntry* GetNextBitmap();

	int32 CountItems() const;
};


#endif /* __FILELIST_H_ */
