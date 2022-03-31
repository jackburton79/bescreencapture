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
	BitmapEntry(BitmapEntry*);
	BitmapEntry(const BitmapEntry&);
	~BitmapEntry();
	
	BBitmap* Bitmap();
	void Replace(BBitmap* bitmap);
	bigtime_t TimeStamp() const;
	
	status_t SaveToDisk(const char* path);

	static status_t WriteFrame(const BBitmap* bitmap, const char* fileName);
	
private:
	BBitmap* fBitmap;
	BString fFileName;
	bigtime_t fFrameTime;
};


class BPath;
class FramesList : private BObjectList<BitmapEntry> {
public:
	FramesList(bool diskOnly = false);
	virtual ~FramesList();

	bool AddItem(BBitmap* bitmap, bigtime_t frameTime);

	BitmapEntry* Pop();
	BitmapEntry* ItemAt(int32 index) const;
	BitmapEntry* ItemAt(int32 index);
	int32 CountItems() const;
	const char* Path() const;

	status_t WriteFrames(const char* path);
private:
	char* fTemporaryPath;
	bool fDiskOnly;
};


#endif /* __FRAMESLIST_H_ */
