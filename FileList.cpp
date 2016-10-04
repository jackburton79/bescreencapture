/*
 * FileList.cpp
 *
 *  Created on: 11/dic/2013
 *      Author: Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#include "FileList.h"

#include <Directory.h>
#include <Entry.h>
#include <Path.h>

FileList::FileList()
	:
	BObjectList<BitmapEntry>(1, true)
{
}


FileList::~FileList()
{
	for (int32 i = CountItems() - 1; i >= 0; i--) {
		BString fileName = ItemAt(i)->file_name;
		BEntry(fileName).Remove();
	}
}


void
FileList::AddItem(const BString& fileName, bigtime_t time)
{
	BitmapEntry* entry = new BitmapEntry;
	entry->file_name = fileName;
	entry->frame_time = time;
	BObjectList<BitmapEntry>::AddItem(entry);
}


BitmapEntry*
FileList::ItemAt(int32 i) const
{
	return BObjectList<BitmapEntry>::ItemAt(i);
}


int32
FileList::CountItems() const
{
	return BObjectList<BitmapEntry>::CountItems();
}
