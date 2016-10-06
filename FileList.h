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
	BitmapEntry(BBitmap* bitmap, bigtime_t time);
	BitmapEntry(const BitmapEntry&);
	
	~BitmapEntry();
	
	BBitmap* Bitmap();
	bigtime_t TimeStamp() const;
	
	status_t SaveToDisk(const char* path);
	
private:
	BBitmap* fBitmap;
	BString fFileName;
	bigtime_t fFrameTime;

};


class BPath;
class FileList : private BObjectList<BitmapEntry> {
public:
	FileList();
	virtual ~FileList();

	bool AddItem(BBitmap* bitmap, bigtime_t frameTime);

	BitmapEntry* Pop();
	BitmapEntry* ItemAt(int32 index) const;
	int32 CountItems() const;
	
private:
	char* fTemporaryPath;
};


#endif /* __FILELIST_H_ */
