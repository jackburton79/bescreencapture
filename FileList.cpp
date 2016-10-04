/*
 * FileList.cpp
 *
 *  Created on: 11/dic/2013
 *      Author: Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#include "FileList.h"

#include <Bitmap.h>
#include <BitmapStream.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Path.h>
#include <TranslationUtils.h>
#include <TranslatorRoster.h>

#include <stdio.h>

static BTranslatorRoster* sTranslatorRoster = NULL;

FileList::FileList()
	:
	BObjectList<BitmapEntry>(1, true)
{
}


FileList::~FileList()
{
}


void
FileList::AddItem(BBitmap* bitmap, bigtime_t time)
{
	BitmapEntry* entry = new BitmapEntry(bitmap);
	entry->frame_time = time;
	BObjectList<BitmapEntry>::AddItem(entry);
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
FileList::GetNextBitmap()
{
	BitmapEntry* entry = BObjectList<BitmapEntry>::RemoveItemAt(0);
	return entry;
}


int32
FileList::CountItems() const
{
	return BObjectList<BitmapEntry>::CountItems();
}


// BitmapEntry
BitmapEntry::BitmapEntry()
	:
	fBitmap(NULL)
{
}


BitmapEntry::BitmapEntry(BBitmap* bitmap)
	:
	fBitmap(bitmap)
{
}


BitmapEntry::~BitmapEntry()
{	
	if (file_name != "")
		BEntry(file_name).Remove();
	else if (fBitmap != NULL)
		delete fBitmap;
}


BBitmap*
BitmapEntry::Bitmap()
{
	if (fBitmap != NULL) {
		BBitmap* bitmap = fBitmap;
		fBitmap = NULL;
		return bitmap;
	}
	if (file_name != "")
		return BTranslationUtils::GetBitmapFile(file_name);
	
	return NULL;
}


void
BitmapEntry::SaveToDisk(const char* path)
{
	if (sTranslatorRoster == NULL)
		sTranslatorRoster = BTranslatorRoster::Default();

	//Save bitmap to disk
	translator_info translatorInfo;
	BBitmapStream bitmapStream(fBitmap);
	
	sTranslatorRoster->Identify(&bitmapStream, NULL,
			&translatorInfo, 0, NULL, 'BMP ');
			
	char* string = tempnam(path, "frame_");
	BFile outFile;
	outFile.SetTo(string, B_WRITE_ONLY|B_CREATE_FILE);
	status_t error = sTranslatorRoster->Translate(&bitmapStream,
		&translatorInfo, NULL, &outFile, 'BMP ');
	
	file_name = string;
	
	fBitmap = NULL;
}
