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
char* FramesList::sTemporaryPath = NULL;

const uint32 kBitmapFormat = 'BMP ';
const uint64 kMinFreeMemory = (1 * 1024 * 1024 * 1024); // 1GB

FramesList::FramesList(bool diskOnly)
	:
	BObjectList<BitmapEntry>(20, true)
{
}


/* virtual */
FramesList::~FramesList()
{
	std::cout << "~FramesList()" << std::endl;
	// Empty the list, which incidentally deletes the files
	// on disk. Must be done before deleting the folder
	BObjectList<BitmapEntry>::MakeEmpty(true);
	
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
	std::cout << "DeleteTempPath()" << std::endl;
	// Delete the folder on disk
	if (sTemporaryPath != NULL) {
		BEntry(sTemporaryPath).Remove();
		free(sTemporaryPath);
		sTemporaryPath = NULL;
	}
	return B_OK;
}


static int
CompareTimestamps(const BitmapEntry* A, const BitmapEntry* B)
{
	return A->TimeStamp() > B->TimeStamp();
}


status_t
FramesList::AddItemsFromDisk()
{
	std::cout << "AddItemsFromDisk: path: " << Path() << std::endl;
	BDirectory dir(Path());
	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		bigtime_t timeStamp = (bigtime_t)strtoull(entry.Name(), NULL, 10);
		BString fullName;
		fullName << Path() << "/" << entry.Name(); 
		std::cout << fullName.String() << std::endl;
		BitmapEntry* bitmapEntry =
			new (std::nothrow) BitmapEntry(fullName, timeStamp);
		if (bitmapEntry == NULL)
			std::cerr << "error adding bitmapentry" << std::endl; 
		BObjectList<BitmapEntry>::AddItem(bitmapEntry);
	}

	std::cout << "found " << CountItems() << " frames" << std::endl;
	SortItems(CompareTimestamps);
	return B_OK;
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


/* static */
const char*
FramesList::Path()
{
	return sTemporaryPath;
}


status_t
FramesList::WriteFrames(const char* path)
{
	std::cout << "WriteFrames()" << std::endl;
	uint32 i = 0;
	status_t status = B_OK;
	while (CountItems() > 0) {
		BString fileName;
		fileName.SetToFormat("frame_%07" B_PRIu32 ".bmp", i + 1);
		BitmapEntry* entry = Pop();
		BString fullPath(path);
		fullPath.Append("/").Append(fileName.String());
		BBitmap* bitmap = entry->Bitmap();
		//status = BitmapEntry::WriteFrame(bitmap, fullPath.String());
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
	std::cout << "BitmapEntry: *" << fileName << "*" << std::endl;
}


BitmapEntry::~BitmapEntry()
{	
	std::cout << "Delete " << fFileName << std::endl;
	if (fFileName != "")
		BEntry(fFileName).Remove();
}


BBitmap*
BitmapEntry::Bitmap()
{
	if (fFileName != "") {
		return BTranslationUtils::GetBitmapFile(fFileName);
	}
	return NULL;
}


void
BitmapEntry::Replace(BBitmap* bitmap)
{
	std::cout << "Replace()" << std::endl;
	if (fFileName != "") {
		//WriteFrame(bitmap, fFileName);
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
	std::cout << "SaveToDisk()" << std::endl;
	return B_ERROR;
	/*if (fBitmap == NULL && fFileName != "") {
		std::cerr << "BitmapEntry::SaveToDisk() called but bitmap already saved to disk. Loading from disk..." << std::endl;
		fBitmap = BTranslationUtils::GetBitmapFile(fFileName);
		if (fBitmap == NULL) {
			std::cerr << "failed to load bitmap from disk" << std::endl;
		}
		fFileName = "";
	}
*/
	BString name;
	name.SetToFormat("frame_%07" B_PRIu32 ".bmp", index + 1);
	fFileName = path;
	fFileName.Append("/").Append(name);
	status_t status = B_ERROR;//WriteFrame(fBitmap, fFileName.String());
	if (status != B_OK) {
		std::cerr << "BitmapEntry::SaveToDisk(): WriteFrame failed: " << ::strerror(status) << std::endl;
		return status;
	}

	/*delete fBitmap;
	fBitmap = NULL;
*/
	return B_OK;
}


/* static */
status_t
FramesList::WriteFrame(BBitmap* bitmap, bigtime_t frameTime, const BPath& path)
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

	// TODO: use path
	BString fileName;
	fileName << Path() << "/" << frameTime;
	std::cout << fileName << std::endl;

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
