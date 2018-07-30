/*
 * FramesList.cpp
 *
 *  Created on: 11/dic/2013
 *      Author: Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#include "FramesList.h"

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>

#include <stdio.h>
#include <stdlib.h>

#include <new>

static BTranslatorRoster* sTranslatorRoster = NULL;

FramesList::FramesList()
	:
	BObjectList<BitmapEntry>(1, true),
	fTemporaryPath(NULL)
{
	BPath path;
	status_t status = find_directory(B_SYSTEM_TEMP_DIRECTORY, &path);
	if (status != B_OK)
		throw status;
	fTemporaryPath = tempnam((char*)path.Path(), (char*)"_BSC");
	status = create_directory(fTemporaryPath, 0777);
	if (status != B_OK)
		throw status;
}


FramesList::~FramesList()
{
	// Empty the list, which incidentally deletes the files
	// on disk. Must be done before deleting the folder below
	BObjectList<BitmapEntry>::MakeEmpty(true);

	// Delete the folder on disk
	if (fTemporaryPath != NULL) {
		BEntry(fTemporaryPath).Remove();
		free(fTemporaryPath);
		fTemporaryPath = NULL;
	}
}


bool
FramesList::AddItem(BBitmap* bitmap, bigtime_t time)
{
	BitmapEntry* entry = new (std::nothrow) BitmapEntry(bitmap, time);
	if (entry == NULL)
		return false;
	if ((CountItems() < 10)
		|| (entry->SaveToDisk(fTemporaryPath) == B_OK)) {
		return BObjectList<BitmapEntry>::AddItem(entry);
	}
	return false;
}


BitmapEntry*
FramesList::Pop()
{
	return BObjectList<BitmapEntry>::RemoveItemAt(0);
}


BitmapEntry*
FramesList::ItemAt(int32 index) const
{
	return BObjectList<BitmapEntry>::ItemAt(index);
}


int32
FramesList::CountItems() const
{
	return BObjectList<BitmapEntry>::CountItems();
}


// BitmapEntry
BitmapEntry::BitmapEntry()
	:
	fBitmap(NULL)
{
}


BitmapEntry::BitmapEntry(BBitmap* bitmap, bigtime_t time)
	:
	fBitmap(bitmap),
	fFrameTime(time)
{
}


BitmapEntry::~BitmapEntry()
{	
	if (fFileName != "")
		BEntry(fFileName).Remove();
	if (fBitmap != NULL)
		delete fBitmap;
}


BBitmap*
BitmapEntry::Bitmap()
{
	if (fBitmap != NULL)
		return new BBitmap(fBitmap);

	if (fFileName != "")
		return BTranslationUtils::GetBitmapFile(fFileName);
	
	return NULL;
}


bigtime_t
BitmapEntry::TimeStamp() const
{
	return fFrameTime;
}


status_t
BitmapEntry::SaveToDisk(const char* path, bool deleteBitmap)
{
	if (sTranslatorRoster == NULL)
		sTranslatorRoster = BTranslatorRoster::Default();

	BBitmapStream bitmapStream(fBitmap);
	fBitmap = NULL;

	translator_info translatorInfo;
	sTranslatorRoster->Identify(&bitmapStream, NULL,
			&translatorInfo, 0, NULL, 'BMP ');
			
	char* string = tempnam(path, "frame_");
	fFileName = string;
	free(string);

	BFile outFile;
	outFile.SetTo(fFileName, B_WRITE_ONLY|B_CREATE_FILE);
	status_t error = sTranslatorRoster->Translate(&bitmapStream,
		&translatorInfo, NULL, &outFile, 'BMP ');
	
	if (error != B_OK || !deleteBitmap) {
		bitmapStream.DetachBitmap(&fBitmap);
		return error;
	}
			
	return B_OK;
}


void
BitmapEntry::Detach()
{
	fBitmap = NULL;
	fFileName = "";
}
