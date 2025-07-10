/*
 * Copyright 2013-2023 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef __FRAMESLIST_H_
#define __FRAMESLIST_H_

#include <ObjectList.h>
#include <String.h>

#include <list>

class BBitmap;
class BitmapEntry {
public:
	BitmapEntry(const BString& fileName, bigtime_t time);
	BitmapEntry(BitmapEntry*);
	BitmapEntry(const BitmapEntry&);
	~BitmapEntry();

	BBitmap* Bitmap();
	void Replace(BBitmap* bitmap);
	bigtime_t TimeStamp() const;
private:
	BString fFileName;
	bigtime_t fFrameTime;
};


typedef std::list<BitmapEntry*> bitmap_list;

class BPath;
class FramesList {
public:
	FramesList(bool diskOnly = false);
	virtual ~FramesList();

	// TODO: Move this away from here
	static status_t CreateTempPath();
	static status_t DeleteTempPath();

	status_t AddItemsFromDisk();

	BitmapEntry* Pop();
	BitmapEntry* LastItem() const;
	BitmapEntry* FirstItem() const;
	const bitmap_list* List() const;
	int32 CountItems() const;
	static const char* Path();

	status_t WriteFrames(const char* path);
	static status_t WriteFrame(BBitmap* bitmap, bigtime_t frameTime, const BString& fileName);
private:
	static char* sTemporaryPath;

	bitmap_list fList;
};


#endif /* __FRAMESLIST_H_ */
