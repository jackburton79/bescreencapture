#include <Bitmap.h>
#include <Debug.h>
#include <List.h>
#include <Screen.h>
#include <TranslationUtils.h>
#include <View.h>

#include <cstring>

#include "BitmapMovie.h"
#include "messages.h"
#include "MovieEncoder.h"
#include "Utils.h"


MovieEncoder::MovieEncoder()
	:
	fFileList(NULL),
	fCursorQueue(NULL),
	fColorSpace(B_NO_COLOR_SPACE)
{
}


MovieEncoder::~MovieEncoder()
{
	DisposeData();
}


void
MovieEncoder::DisposeData()
{
	if (fFileList != NULL) {
		for (int32 i = fFileList->CountItems() - 1; i >= 0; i--) {
			char* fileName = static_cast<char*>(fFileList->ItemAtFast(i));
			BEntry(fileName).Remove();
			free(fileName);
		}
		delete fFileList;
		fFileList = NULL;
	}
}

 
status_t
MovieEncoder::SetSource(BList *fileList, const bool ownsList)
{
	fFileList = fileList;
	return B_OK;
}


status_t
MovieEncoder::SetOutputFile(const char *fileName)
{
	fOutputFile.SetTo(fileName);
	return B_OK;
}


status_t
MovieEncoder::SetDestFrame(const BRect &rect)
{
	fDestFrame = rect;
	fDestFrame.OffsetTo(B_ORIGIN);
	
	return B_OK;
}


void
MovieEncoder::SetColorSpace(const color_space &space)
{
	fColorSpace = space;
}


status_t
MovieEncoder::SetQuality(const float &quality)
{
	return B_OK;
}


status_t
MovieEncoder::SetCursorQueue(std::queue<BPoint> *queue)
{
	fCursorQueue = queue;
	return B_OK;
}


status_t
MovieEncoder::SetMessenger(const BMessenger &messenger)
{
	if (!messenger.IsValid())
		return B_BAD_VALUE;
	
	fMessenger = messenger;
	return B_OK;
}


status_t
MovieEncoder::Encode()
{
	int32 framesLeft = fFileList->CountItems();
	int32 framesWritten = 0;
	
	if (framesLeft <= 0) {
		DisposeData();
		BMessage message(kEncodingFinished);
		message.AddInt32("status", (int32)B_ERROR);
		fMessenger.SendMessage(&message);
		return B_ERROR;
	}
	
	// Create movie
	char movieName[B_FILE_NAME_LENGTH];
	MakeUniqueName(fOutputFile.Path(), movieName, B_FILE_NAME_LENGTH);
	entry_ref movieRef;
	get_ref_for_path(movieName, &movieRef);
	
	media_file_format fileFormat;
	if (!GetMediaFileFormat(fFamily, fileFormat)) {
		BMessage message(kEncodingFinished);
		message.AddInt32("status", (int32)B_ERROR);
		fMessenger.SendMessage(&message);
		return B_ERROR;
	}
	
	BBitmap* bitmap = BTranslationUtils::GetBitmapFile((const char*)fFileList->ItemAtFast(0));
	BRect sourceFrame = bitmap->Bounds();
	delete bitmap;
		
	if (!fDestFrame.IsValid())
		fDestFrame = sourceFrame.OffsetToCopy(B_ORIGIN);
				
	BitmapMovie* movie = new BitmapMovie(fDestFrame.IntegerWidth() + 1,
					fDestFrame.IntegerHeight() + 1, fColorSpace);
				
	status_t error = movie->CreateFile(movieRef, fileFormat, fFormat, fCodecInfo);
	if (error < B_OK) {
		delete movie;
		DisposeData();
		BMessage message(kEncodingFinished);
		message.AddInt32("status", (int32)error);
		fMessenger.SendMessage(&message);		
		return error;
	}
		
	// Bitmap and view used to convert the source bitmap
	// to the correct size and depth	
	BBitmap* destBitmap = new BBitmap(fDestFrame, fColorSpace, true);
	BView* destDrawer = new BView(fDestFrame, "drawing view", B_FOLLOW_NONE, 0);
	if (destBitmap->Lock()) {
		destBitmap->AddChild(destDrawer);
		destBitmap->Unlock();
	}
	
	const uint32 keyFrameFrequency = 10;
	
	BMessage progressMessage(B_UPDATE_STATUS_BAR);
	progressMessage.AddFloat("delta", 1.0);
	bool keyFrame = true;		
	for (int32 i = 0; i < fFileList->CountItems(); i++) {
		if (framesWritten % keyFrameFrequency == 0)
			keyFrame = true;
		const char* fileName = reinterpret_cast<const char*>(fFileList->ItemAtFast(i));
		BBitmap* frame = BTranslationUtils::GetBitmapFile(fileName);
		if (frame == NULL)
			continue;
						
		// Draw scaled
		if (error == B_OK) {
			destBitmap->Lock();
			destDrawer->DrawBitmap(frame, frame->Bounds(), destDrawer->Bounds());
			destDrawer->Sync();
			destBitmap->Unlock();
		}
		
		delete frame;
			
		if (error == B_OK)
			error = movie->WriteFrame(destBitmap, keyFrame);
		
		if (error == B_OK) {
			framesWritten++;
			keyFrame = false;				
		} else {
			printf("%s\n", strerror(error));
			break;
		}
		
		if (fMessenger.IsValid())
			fMessenger.SendMessage(new BMessage(progressMessage));		
	}
	
	//printf("%ld frames written\n", framesWritten);
		
	delete movie;
	delete destBitmap;
	//delete cursor;
	
	DisposeData();
	
	BMessage message(kEncodingFinished);
	message.AddInt32("status", (int32)B_OK);
	fMessenger.SendMessage(&message);
		
	return B_OK;
}


status_t
MovieEncoder::Encode(const media_format_family& family,
					const media_format& format,
					const media_codec_info& info,
					const color_space& space)
{
	SetMediaFormatFamily(family);
	SetMediaFormat(format);
	SetMediaCodecInfo(info);
	SetColorSpace(space);
	
	return Encode();
}


void
MovieEncoder::ResetConfiguration()
{
	fColorSpace = B_NO_COLOR_SPACE;
}


media_format_family
MovieEncoder::MediaFormatFamily() const
{
	return fFamily;
}


void
MovieEncoder::SetMediaFormatFamily(const media_format_family& family)
{
	fFamily = family;
}


void
MovieEncoder::SetMediaFormat(const media_format& format)
{
	fFormat = format;
}


void
MovieEncoder::SetMediaCodecInfo(const media_codec_info& info)
{
	fCodecInfo = info;
}


// private methods
BBitmap*
MovieEncoder::GetCursorBitmap(const uint8* cursor)
{
	uint8 size = *cursor;
	
	BBitmap* cursorBitmap = new BBitmap(BRect(0, 0, size - 1, size - 1), B_RGBA32);
	
	uint32 black = 0xFF000000;
	uint32 white = 0xFFFFFFFF;
	
	uint8* castCursor = const_cast<uint8*>(cursor);
	uint16* cursorPtr = reinterpret_cast<uint16*>(castCursor + 4);
	uint16* maskPtr = reinterpret_cast<uint16*>(castCursor + 36);
	uint8* buffer = static_cast<uint8*>(cursorBitmap->Bits());
	uint16 cursorFlip, maskFlip;
	uint16 cursorVal, maskVal;
	for (uint8 row = 0; row < size; row++) {
		uint32* bits = (uint32*)(buffer + (row * cursorBitmap->BytesPerRow()));
		cursorFlip = (cursorPtr[row] & 0xFF) << 8;
		cursorFlip |= (cursorPtr[row] & 0xFF00) >> 8;
		
		maskFlip = (maskPtr[row] & 0xFF) << 8;
		maskFlip |= (maskPtr[row] & 0xFF00) >> 8;
		
		for (uint8 column = 0; column < size; column++) {
			uint16 posVal = 1 << (15 - column);
			cursorVal = cursorFlip & posVal;
			maskVal = maskFlip & posVal;
			bits[column] = (cursorVal != 0 ? black : white) &
							(maskVal > 0 ? white : 0x00FFFFFF);
							
		}
	}
		
	return cursorBitmap;
}


status_t
MovieEncoder::PopCursorPosition(BPoint& point)
{
	ASSERT(fCursorQueue != NULL);
	//point = fCursorQueue->front();
	//fCursorQueue->pop();
	
	return B_OK;
}
