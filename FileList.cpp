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

FileList::FileList(const bool ownsFiles)
	:
	BStringList(),
	fOwnsFiles(ownsFiles)
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


/* static */
FileList*
FileList::CreateFileList(const BPath &path)
{
	FileList* list = new FileList(true);

	BDirectory directory(path.Path());
	BEntry entry;
	BString string;
	char entryName[B_FILE_NAME_LENGTH];
	while (directory.GetNextEntry(&entry) == B_OK) {
		if (entry.GetName(entryName) == B_OK) {
			string.SetTo(path.Path());
			string << "/" << entryName;
			list->AddItem(string);
		}
	}

	return list;
}
