/*
 * FramesList.cpp
 *
 *  Created on: 11/dic/2013
 *      Author: Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#include "FramesList.h"

#include "Utils.h"

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <new>

static BTranslatorRoster* sTranslatorRoster = NULL;

const uint64 kMinFreeMemory = (1 * 1024 * 1024 * 1024); // 1GB

FramesList::FramesList(bool diskOnly)
	:
	BObjectList<BitmapEntry>(1, true),
	fTemporaryPath(NULL),
	fDiskOnly(diskOnly)
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


/* virtual */
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
FramesList::AddItem(BBitmap* bitmap, bigtime_t frameTime)
{
	BitmapEntry* entry = new (std::nothrow) BitmapEntry(bitmap, frameTime);
	if (entry == NULL) {
		std::cerr << "FramesList::AddItem(): cannot create BitmapEntry" << std::endl;
		return false;
	}

	// TODO: Use a smarter check
	const int32 numItems = CountItems();
	if (fDiskOnly || numItems >= 100 || GetFreeMemory() < kMinFreeMemory) {
		status_t status = entry->SaveToDisk(fTemporaryPath, numItems);
		if (status != B_OK) {
			std::cerr << "FramesList::AddItem(): cannot save to disk: " << strerror(status) << std::endl;
			delete entry;
			return false;
		}
	}

	return BObjectList<BitmapEntry>::AddItem(entry);
}


BitmapEntry*
FramesList::Pop()
{
	return BObjectList<BitmapEntry>::RemoveItemAt(int32(0));
}


BitmapEntry*
FramesList::ItemAt(int32 index) const
{
	return BObjectList<BitmapEntry>::ItemAt(index);
}


BitmapEntry*
FramesList::ItemAt(int32 index)
{
	return BObjectList<BitmapEntry>::ItemAt(index);
}


int32
FramesList::CountItems() const
{
	return BObjectList<BitmapEntry>::CountItems();
}


const char*
FramesList::Path() const
{
	return fTemporaryPath;
}


status_t
FramesList::WriteFrames(const char* path)
{
	uint32 i = 0;
	status_t status = B_OK;
	while (CountItems() > 0) {
		BString fileName;
		fileName.SetToFormat("frame_%07" B_PRIu32 ".png", i + 1);
		BitmapEntry* entry = Pop();
		BString fullPath(path);
		fullPath.Append("/").Append(fileName.String());
		BBitmap* bitmap = entry->Bitmap();
		status = BitmapEntry::WriteFrame(bitmap, fullPath.String());
		delete bitmap;
		delete entry;
		if (status != B_OK)
			break;
		i++;
	}
	return status;
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
		return new (std::nothrow) BBitmap(fBitmap);

	if (fFileName != "")
		return BTranslationUtils::GetBitmapFile(fFileName);
	
	return NULL;
}


void
BitmapEntry::Replace(BBitmap* bitmap)
{
	if (fBitmap != NULL) {
		delete fBitmap;
		fBitmap = bitmap;
	} else if (fFileName != "") {
		WriteFrame(bitmap, fFileName);
		delete bitmap;
	}
}


bigtime_t
BitmapEntry::TimeStamp() const
{
	return fFrameTime;
}


status_t
BitmapEntry::SaveToDisk(const char* path, const int32 index)
{
	if (fBitmap == NULL && fFileName != "") {
		std::cerr << "BitmapEntry::SaveToDisk() called but bitmap already saved to disk. Loading from disk..." << std::endl;
		fBitmap = BTranslationUtils::GetBitmapFile(fFileName);
		if (fBitmap == NULL) {
			std::cerr << "failed to load bitmap from disk" << std::endl;
		}
		fFileName = "";
	}

	BString name;
	name.SetToFormat("frame_%07" B_PRIu32 ".png", index + 1);
	fFileName = path;
	fFileName.Append("/").Append(name);
	status_t status = WriteFrame(fBitmap, fFileName.String());
	if (status != B_OK) {
		std::cerr << "BitmapEntry::SaveToDisk(): WriteFrame failed: " << ::strerror(status) << std::endl;
		return status;
	}

	delete fBitmap;
	fBitmap = NULL;

	return B_OK;
}


/* static */
status_t
BitmapEntry::WriteFrame(const BBitmap* bitmap, const char* fileName)
{
	// Does not take ownership of the passed BBitmap.
	if (sTranslatorRoster == NULL) {
		sTranslatorRoster = BTranslatorRoster::Default();
	}

	if (sTranslatorRoster == NULL) {
		std::cerr << "BitmapEntry::WriteFrame(): BTranslatorRoster::Default() returned NULL" << std::endl;
		return B_NO_MEMORY;
	}

	BBitmap *tempBitmap = new (std::nothrow) BBitmap(*bitmap);
	if (tempBitmap == NULL) {
		std::cerr << "BitmapEntry::WriteFrame(): cannot create bitmap" << std::endl;
		return B_NO_MEMORY;
	}

	BBitmapStream bitmapStream(tempBitmap);
	translator_info translatorInfo;
	status_t status = sTranslatorRoster->Identify(&bitmapStream, NULL,
			&translatorInfo, 0, NULL, 'PNG ');
	if (status != B_OK) {
		std::cerr << "BitmapEntry::WriteFrame(): cannot identify bitmap stream: " << ::strerror(status) << std::endl;
		return status;
	}

	BFile outFile;
	status = outFile.SetTo(fileName, B_WRITE_ONLY|B_CREATE_FILE);
	if (status != B_OK) {
		std::cerr << "BitmapEntry::WriteFrame(): cannot create file" << ::strerror(status) << std::endl;
		return status;
	}
	
	status = sTranslatorRoster->Translate(&bitmapStream,
		&translatorInfo, NULL, &outFile, 'PNG ');
	if (status != B_OK)
		std::cerr << "BitmapEntry::WriteFrame(): cannot translate bitmap: " << ::strerror(status) << std::endl;

	return status;
}
