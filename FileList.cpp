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
	BStringList()
{

}


FileList::~FileList()
{
	for (int32 i = CountItems() - 1; i >= 0; i--) {
		BString fileName = StringAt(i);
		BEntry(fileName).Remove();
	}
}


void
FileList::AddItem(const BString& fileName)
{
	Add(fileName);
}


BString
FileList::ItemAt(int32 i) const
{
	return StringAt(i);
}


int32
FileList::CountItems() const
{
	return CountStrings();
}
