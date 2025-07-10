/*
 * Copyright 2013-2023 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
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
char* FramesList::sTemporaryPath = NULL;

const uint32 kBitmapFormat = 'BMP ';

FramesList::FramesList(bool diskOnly)
{
}


/* virtual */
FramesList::~FramesList()
{
	// Empty the list, which incidentally deletes the files
	// on disk. Must be done before deleting the folder
	fList.clear();

	DeleteTempPath();
}


/* static */
status_t
FramesList::CreateTempPath()
{
	BPath path;
	status_t status = find_directory(B_SYSTEM_TEMP_DIRECTORY, &path);
	if (status != B_OK)
		return status;
	char* pathName = (char*)malloc(B_PATH_NAME_LENGTH);
	if (pathName == NULL)
		return B_NO_MEMORY;
	::snprintf(pathName, B_PATH_NAME_LENGTH, "%s/_BSCXXXXXX", path.Path());
	sTemporaryPath = ::mkdtemp(pathName);
	if (sTemporaryPath == NULL)
		return B_ERROR;
	return B_OK;
}


/* static */
status_t
FramesList::DeleteTempPath()
{
	// Delete the folder on disk
	if (sTemporaryPath != NULL) {
		BEntry(sTemporaryPath).Remove();
		free(sTemporaryPath);
		sTemporaryPath = NULL;
	}
	return B_OK;
}


struct
{
	bool operator()(BitmapEntry* A, BitmapEntry* B)
	{
		return A->TimeStamp() < B->TimeStamp();
	}
} TimeStampLess;


status_t
FramesList::AddItemsFromDisk()
{
	BDirectory dir(Path());
	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		bigtime_t timeStamp = (bigtime_t)strtoull(entry.Name(), NULL, 10);
		BString fullName;
		fullName << Path() << "/" << entry.Name();
		BitmapEntry* bitmapEntry =
			new (std::nothrow) BitmapEntry(fullName, timeStamp);
		fList.push_back(bitmapEntry);
	}

	// Sort items based on timestamps
	fList.sort(TimeStampLess);
	return B_OK;
}


BitmapEntry*
FramesList::Pop()
{
	BitmapEntry* entry = fList.front();
	fList.pop_front();

	return entry;
}


BitmapEntry*
FramesList::FirstItem() const
{
	return fList.front();
}


BitmapEntry*
FramesList::LastItem() const
{
	return fList.back();
}


const bitmap_list*
FramesList::List() const
{
	return &fList;
}


int32
FramesList::CountItems() const
{
	return fList.size();
}


/* static */
const char*
FramesList::Path()
{
	return sTemporaryPath;
}


status_t
FramesList::WriteFrames(const char* path)
{
	uint32 i = 0;
	status_t status = B_OK;
	while (CountItems() > 0) {
		BString fileName;
		fileName.SetToFormat("frame_%07" B_PRIu32 ".bmp", i + 1);
		BitmapEntry* entry = Pop();
		BString fullPath(path);
		fullPath.Append("/").Append(fileName.String());
		BBitmap* bitmap = entry->Bitmap();
		status = FramesList::WriteFrame(bitmap, entry->TimeStamp(), fullPath);
		delete bitmap;
		delete entry;
		if (status != B_OK)
			break;
		i++;
	}
	return status;
}


// BitmapEntry
BitmapEntry::BitmapEntry(const BString& fileName, bigtime_t time)
	:
	fFileName(fileName),
	fFrameTime(time)
{
}


BitmapEntry::~BitmapEntry()
{
	if (fFileName != "")
		BEntry(fFileName).Remove();
}


BBitmap*
BitmapEntry::Bitmap()
{
	if (fFileName == "")
		return NULL;
	return BTranslationUtils::GetBitmapFile(fFileName);
}


void
BitmapEntry::Replace(BBitmap* bitmap)
{
	if (fFileName != "") {
		FramesList::WriteFrame(bitmap, TimeStamp(), fFileName);
		delete bitmap;
	}
}


bigtime_t
BitmapEntry::TimeStamp() const
{
	return fFrameTime;
}


/* static */
status_t
FramesList::WriteFrame(BBitmap* bitmap, bigtime_t frameTime, const BString& fileName)
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
			&translatorInfo, 0, NULL, kBitmapFormat);
	if (status != B_OK) {
		std::cerr << "BitmapEntry::WriteFrame(): cannot identify bitmap stream: " << ::strerror(status) << std::endl;
		return status;
	}

	BFile outFile;
	status = outFile.SetTo(fileName.String(), B_WRITE_ONLY|B_CREATE_FILE);
	if (status != B_OK) {
		std::cerr << "BitmapEntry::WriteFrame(): cannot create file" << ::strerror(status) << std::endl;
		return status;
	}

	status = sTranslatorRoster->Translate(&bitmapStream,
		&translatorInfo, NULL, &outFile, kBitmapFormat);
	if (status != B_OK)
		std::cerr << "BitmapEntry::WriteFrame(): cannot translate bitmap: " << ::strerror(status) << std::endl;

	return status;
}
