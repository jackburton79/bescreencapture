/*
 * FramesList.h
 *
 *  Created on: 11/dic/2013
 *      Author: Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#ifndef __FRAMESLIST_H_
#define __FRAMESLIST_H_

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
	
	status_t SaveToDisk(const char* path, bool deleteBitmap = true);
	void Detach();
	
private:
	BBitmap* fBitmap;
	BString fFileName;
	bigtime_t fFrameTime;

};


class BPath;
class FramesList : private BObjectList<BitmapEntry> {
public:
	FramesList();
	virtual ~FramesList();

	bool AddItem(BBitmap* bitmap, bigtime_t frameTime);

	BitmapEntry* Pop();
	BitmapEntry* ItemAt(int32 index) const;
	int32 CountItems() const;
	
private:
	char* fTemporaryPath;
};


#endif /* __FRAMESLIST_H_ */
