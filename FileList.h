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

class BitmapEntry {
public:
	BString file_name;
	bigtime_t frame_time;
};

class FileList : private BObjectList<BitmapEntry> {
public:
	FileList();
	virtual ~FileList();

	void AddItem(const BString& fileName, bigtime_t frameTime);
	BitmapEntry* ItemAt(int32 i) const;

	int32 CountItems() const;
};


#endif /* __FILELIST_H_ */
