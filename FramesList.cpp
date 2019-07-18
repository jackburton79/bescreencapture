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
	char* pathName = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathName == NULL)
		throw B_NO_MEMORY;
	::snprintf(pathName, B_PATH_NAME_LENGTH, "%s/_BSCXXXXXX", path.Path());
	fTemporaryPath = ::mkdtemp(pathName);
	if (fTemporaryPath == NULL)
		throw B_ERROR;
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
BitmapEntry::SaveToDisk(const char* path)
{
	char tempFileName[B_PATH_NAME_LENGTH];
	::snprintf(tempFileName, sizeof(tempFileName), "%s/frame_XXXXXXX", path);

	// use mkstemp, but then we close the fd immediately,
	// because we need only the file name.
	int tempFile = ::mkstemp(tempFileName);
	fFileName = tempFileName;
	close(tempFile);

	status_t status = SaveToDisk(fBitmap, fFileName.String());

	if (status != B_OK)
		return status;

	delete fBitmap;
	fBitmap = NULL;

	return B_OK;
}


/* static */
status_t
BitmapEntry::SaveToDisk(const BBitmap* bitmap, const char* fileName)
{
	if (sTranslatorRoster == NULL)
		sTranslatorRoster = BTranslatorRoster::Default();

	BBitmap *tempBitmap = new BBitmap(*bitmap);
	BBitmapStream bitmapStream(tempBitmap);

	translator_info translatorInfo;
	sTranslatorRoster->Identify(&bitmapStream, NULL,
			&translatorInfo, 0, NULL, 'BMP ');

	BFile outFile;
	outFile.SetTo(fileName, B_WRITE_ONLY|B_CREATE_FILE);
	status_t status = sTranslatorRoster->Translate(&bitmapStream,
		&translatorInfo, NULL, &outFile, 'BMP ');

	return status;
}
