#include "Settings.h"

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <String.h>

#include <cstdio>
#include <cstring>

static BMessage sSettings;

const static char *kCaptureRect = "capture rect";
const static char *kClipDepth = "clip depth";
const static char *kClipScale = "clip scale";
const static char *kUseDirectWindow = "use DW";
const static char *kIncludeCursor = "cursor";
const static char *kMinimize = "minimize";
const static char *kOutputFile = "output file";
const static char *kOutputFileFormat = "output file format";
const static char *kThreadPriority = "thread priority";
const static char *kHideDeskbarIcon = "hide Deskbar icon";


Settings::Settings()
{
	fSettings = &sSettings;
	fLocker.Lock();
}


Settings::~Settings()
{
	fLocker.Unlock();
	fSettings = NULL;
}


status_t
Settings::Load()
{
	SetDefaults();
	
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status == B_OK)
		status = path.Append("BeScreenCapture");
	
	BFile file;
	if (status == B_OK)
		status = file.SetTo(path.Path(), B_READ_ONLY);
	
	BMessage tempMessage;	
	if (status == B_OK)	
		status = tempMessage.Unflatten(&file);
	
	if (status == B_OK) {
		// Copy the loaded fields to the real settings message
		// TODO: Change this: if there is no default value,
		// the setting is never saved!
		BRect rect;
		bool boolean;
		int32 integer;
		float decimal;
		const char *string = NULL;
		if (tempMessage.FindRect(kCaptureRect, &rect) == B_OK)
			sSettings.ReplaceRect(kCaptureRect, rect);
		if (tempMessage.FindInt32(kClipDepth, &integer) == B_OK)
			sSettings.ReplaceInt32(kClipDepth, integer);
		if (tempMessage.FindFloat(kClipScale, &decimal) == B_OK)
			sSettings.ReplaceFloat(kClipScale, decimal);
		if (tempMessage.FindBool(kUseDirectWindow, &boolean) == B_OK)
			sSettings.ReplaceBool(kUseDirectWindow, boolean);
		if (tempMessage.FindBool(kIncludeCursor, &boolean) == B_OK)
			sSettings.ReplaceBool(kIncludeCursor, boolean);
		if (tempMessage.FindBool(kMinimize, &boolean) == B_OK)
			sSettings.ReplaceBool(kMinimize, boolean);
		if (tempMessage.FindBool(kHideDeskbarIcon, &boolean) == B_OK)
			sSettings.ReplaceBool(kHideDeskbarIcon, boolean);
		if (tempMessage.FindString(kOutputFile, &string) == B_OK)
			sSettings.ReplaceString(kOutputFile, string);
		if (tempMessage.FindString(kOutputFileFormat, &string) == B_OK)
			sSettings.ReplaceString(kOutputFileFormat, string);
		if (tempMessage.FindInt32(kThreadPriority, &integer) == B_OK)
			sSettings.ReplaceInt32(kThreadPriority, integer);	
	}	
	
	return status;
}


status_t
Settings::Save()
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status == B_OK)
		status = path.Append("BeScreenCapture");
	
	BFile file;
	if (status == B_OK)
		status = file.SetTo(path.Path(), B_WRITE_ONLY|B_CREATE_FILE);
		
	if (status == B_OK)	
		status = sSettings.Flatten(&file);
	
	return status;
}


void
Settings::SetCaptureArea(const BRect &rect)
{
	if (!fSettings->HasRect(kCaptureRect))
		fSettings->AddRect(kCaptureRect, rect);
	else
		fSettings->ReplaceRect(kCaptureRect, rect);
}


void
Settings::GetCaptureArea(BRect &rect) const
{
	fSettings->FindRect(kCaptureRect, &rect);
}


BRect
Settings::CaptureArea() const
{
	BRect rect;
	fSettings->FindRect(kCaptureRect, &rect);
	return rect;
}


void
Settings::GetTargetRect(BRect& rect) const
{
	rect = TargetRect();
}


BRect
Settings::TargetRect() const
{
	const float scale = Scale();
	BRect scaledRect = CaptureArea();
	
	scaledRect.OffsetTo(B_ORIGIN);
	scaledRect.right = (scaledRect.right + 1) * scale / 100 - 1;
	scaledRect.bottom = (scaledRect.bottom + 1) * scale / 100 - 1;	
	
	return scaledRect;
}


void
Settings::SetClipDepth(const color_space &space)
{
	const int32 &spaceInt = (int32)space;
	if (!fSettings->HasInt32(kClipDepth))
		fSettings->AddInt32(kClipDepth, spaceInt);
	else
		fSettings->ReplaceInt32(kClipDepth, spaceInt);
}


void
Settings::GetClipDepth(color_space &space) const
{
	fSettings->FindInt32(kClipDepth, (int32 *)&space);
}


color_space
Settings::ClipDepth() const
{
	color_space depth;
	fSettings->FindInt32(kClipDepth, (int32 *)&depth);
	return depth;
}


void
Settings::SetScale(const float &scale)
{
	if (!fSettings->HasFloat(kClipScale))
		fSettings->AddFloat(kClipScale, scale);
	else
		fSettings->ReplaceFloat(kClipScale, scale);
}


void
Settings::GetScale(float &scale) const
{
	fSettings->FindFloat(kClipScale, &scale);
}


float
Settings::Scale() const
{
	float scale;
	fSettings->FindFloat(kClipScale, &scale);
	return scale;
}


void
Settings::SetUseDirectWindow(const bool &use)
{
	if (!fSettings->HasBool(kUseDirectWindow))
		fSettings->AddBool(kUseDirectWindow, use);
	else
		fSettings->ReplaceBool(kUseDirectWindow, use);
}


void
Settings::GetUseDirectWindow(bool &use) const
{
	fSettings->FindBool(kUseDirectWindow, &use);
}


bool
Settings::UseDirectWindow() const
{
	bool useDW;
	fSettings->FindBool(kUseDirectWindow, &useDW);
	return useDW;
}


void
Settings::SetHideDeskbarIcon(const bool &use)
{
	if (!fSettings->HasBool(kHideDeskbarIcon))
		fSettings->AddBool(kHideDeskbarIcon, use);
	else
		fSettings->ReplaceBool(kHideDeskbarIcon, use);
}


void
Settings::GetHideDeskbarIcon(bool &use) const
{
	fSettings->FindBool(kHideDeskbarIcon, &use);
}


bool
Settings::HideDeskbarIcon() const
{
	bool hideDeskbar;
	fSettings->FindBool(kHideDeskbarIcon, &hideDeskbar);
	return hideDeskbar;
}


void
Settings::SetIncludeCursor(const bool &include)
{
	if (!fSettings->HasBool(kIncludeCursor))
		fSettings->AddBool(kIncludeCursor, include);
	else
		fSettings->ReplaceBool(kIncludeCursor, include);
}


void
Settings::GetIncludeCursor(bool &include) const
{
	fSettings->FindBool(kIncludeCursor, &include);
}


bool
Settings::IncludeCursor() const
{
	bool include;
	fSettings->FindBool(kIncludeCursor, &include);
	return include;
}


void
Settings::SetMinimizeOnRecording(const bool &mini)
{
	if (!fSettings->HasBool(kMinimize))
		fSettings->AddBool(kMinimize, mini);
	else
		fSettings->ReplaceBool(kMinimize, mini);
}


void
Settings::GetMinimizeOnRecording(bool &mini) const
{
	fSettings->FindBool(kMinimize, &mini);
}


bool
Settings::MinimizeOnRecording() const
{
	bool mini;
	fSettings->FindBool(kMinimize, &mini);
	return mini;
}


void
Settings::SetOutputFileName(const char *name)
{
	if (!fSettings->HasString(kOutputFile))
		fSettings->AddString(kOutputFile, name);
	else
		fSettings->ReplaceString(kOutputFile, name);
}



void
Settings::GetOutputFileName(BString &name) const
{
	fSettings->FindString(kOutputFile, &name);
}


void
Settings::SetOutputFileFormat(const char* fileFormat)
{
	if (!fSettings->HasString(kOutputFileFormat))
		fSettings->AddString(kOutputFileFormat, fileFormat);
	else
		fSettings->ReplaceString(kOutputFileFormat, fileFormat);
}


void
Settings::GetOutputFileFormat(BString& string) const
{
	fSettings->FindString(kOutputFileFormat, &string);
}


void
Settings::SetEncodingThreadPriority(const int32 &value)
{
	if (!fSettings->HasInt32(kThreadPriority))
		fSettings->AddInt32(kThreadPriority, value);
	else
		fSettings->ReplaceInt32(kThreadPriority, value);
}


void
Settings::GetEncodingThreadPriority(int32 &value) const
{
	fSettings->FindInt32(kThreadPriority, &value);
}


int32
Settings::EncodingThreadPriority() const
{
	int32 prio = B_NORMAL_PRIORITY;
	fSettings->FindInt32(kThreadPriority, &prio);
	return prio;
}


void
Settings::PrintToStream()
{
	fSettings->PrintToStream();
	TargetRect().PrintToStream();
}


status_t
Settings::SetDefaults()
{
	sSettings.MakeEmpty();
	sSettings.AddString(kOutputFile, "/boot/home/outputfile");
	sSettings.AddFloat(kClipScale, 100);
	sSettings.AddInt32(kClipDepth, B_RGB32);
	sSettings.AddBool(kIncludeCursor, true);
	sSettings.AddInt32(kThreadPriority, B_NORMAL_PRIORITY);
	sSettings.AddBool(kMinimize, false);
	sSettings.AddBool(kHideDeskbarIcon, false);
	sSettings.AddString(kOutputFileFormat, "");
	
	return B_OK;
}
