#include "Settings.h"

#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Message.h>
#include <Path.h>
#include <Screen.h>
#include <String.h>

#include <cstdio>
#include <cstring>

static Settings* sDefault;
static Settings* sCurrent;

const static char *kCaptureRect = "capture rect";
const static char *kClipDepth = "clip depth";
const static char *kClipScale = "clip scale";
const static char *kUseDirectWindow = "use DW";
const static char *kIncludeCursor = "cursor";
const static char *kMinimize = "minimize";
const static char *kOutputFile = "output file";
const static char *kOutputFileFormat = "output file format";
const static char *kOutputCodecName = "output codec";
const static char *kThreadPriority = "thread priority";
const static char *kWindowFrameBorderSize = "window frame border size";
const static char *kCaptureFrameDelay = "capture frame delay";
const static char *kDockingMode = "docking mode";



/* static */
status_t
Settings::Initialize()
{
	try {
		sDefault = new Settings;
		sDefault->_SetDefaults();
		sCurrent = new Settings(*sDefault);
		sCurrent->Load();
	} catch (...) {
		return B_ERROR;
	}

	return B_OK;
}


/* static */
void
Settings::Destroy()
{
	delete sDefault;
	delete sCurrent;
}


/* static */
Settings&
Settings::Default()
{
	return *sDefault;
}


/* static */
Settings&
Settings::Current()
{
	return *sCurrent;
}


/* static */
void
Settings::ResetToDefaults()
{
	delete sCurrent;
	sCurrent = new Settings(*sDefault);
}


status_t
Settings::Load()
{
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
		// TODO: Since we use Replace<type> methods, if there is
		// no default value, the setting won't be loaded.
		// This is okay for dropping old and incompatible settings,
		// but it's still inconvenient, since we need to add a default value
		// (in SetDefaults()) for every new setting we introduce
		BRect rect;
		bool boolean;
		int32 integer;
		float decimal;
		const char *string = NULL;
		if (tempMessage.FindRect(kCaptureRect, &rect) == B_OK)
			fSettings->ReplaceRect(kCaptureRect, rect);
		if (tempMessage.FindInt32(kClipDepth, &integer) == B_OK)
			fSettings->ReplaceInt32(kClipDepth, integer);
		if (tempMessage.FindFloat(kClipScale, &decimal) == B_OK)
			fSettings->ReplaceFloat(kClipScale, decimal);
		if (tempMessage.FindBool(kUseDirectWindow, &boolean) == B_OK)
			fSettings->ReplaceBool(kUseDirectWindow, boolean);
		if (tempMessage.FindBool(kIncludeCursor, &boolean) == B_OK)
			fSettings->ReplaceBool(kIncludeCursor, boolean);
		if (tempMessage.FindBool(kMinimize, &boolean) == B_OK)
			fSettings->ReplaceBool(kMinimize, boolean);
		if (tempMessage.FindString(kOutputFile, &string) == B_OK)
			fSettings->ReplaceString(kOutputFile, string);
		if (tempMessage.FindString(kOutputFileFormat, &string) == B_OK)
			fSettings->ReplaceString(kOutputFileFormat, string);
		if (tempMessage.FindString(kOutputCodecName, &string) == B_OK)
			fSettings->ReplaceString(kOutputCodecName, string);
		if (tempMessage.FindInt32(kThreadPriority, &integer) == B_OK)
			fSettings->ReplaceInt32(kThreadPriority, integer);
		if (tempMessage.FindInt32(kWindowFrameBorderSize, &integer) == B_OK)
			fSettings->ReplaceInt32(kWindowFrameBorderSize, integer);
		if (tempMessage.FindInt32(kCaptureFrameDelay, &integer) == B_OK)
			fSettings->ReplaceInt32(kCaptureFrameDelay, integer);
		if (tempMessage.FindBool(kDockingMode, &boolean) == B_OK)
			fSettings->ReplaceBool(kDockingMode, boolean);
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
		status = fSettings->Flatten(&file);
	
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
Settings::SetWindowFrameBorderSize(const int32 &size)
{
	if (!fSettings->HasInt32(kWindowFrameBorderSize))
		fSettings->AddInt32(kWindowFrameBorderSize, size);
	else
		fSettings->ReplaceInt32(kWindowFrameBorderSize, size);
}


void
Settings::GetWindowFrameBorderSize(int32 &size) const
{
	fSettings->FindInt32(kWindowFrameBorderSize, &size);
}


int32
Settings::WindowFrameBorderSize() const
{
	int32 size = 0;
	fSettings->FindInt32(kWindowFrameBorderSize, &size);
	return size;
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


BString
Settings::OutputFileName() const
{
	BString fileName;
	GetOutputFileName(fileName);
	return fileName;
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
Settings::SetOutputCodec(const char* codecName)
{
	if (!fSettings->HasString(kOutputCodecName))
		fSettings->AddString(kOutputCodecName, codecName);
	else
		fSettings->ReplaceString(kOutputCodecName, codecName);
}


void
Settings::GetOutputCodec(BString& string) const
{
	fSettings->FindString(kOutputCodecName, &string);
}


int32
Settings::CaptureFrameDelay() const
{
	int32 value;
	fSettings->FindInt32(kCaptureFrameDelay, &value);
	return value;
}


void
Settings::SetCaptureFrameDelay(const int32& value)
{
	if (!fSettings->HasInt32(kCaptureFrameDelay))
		fSettings->AddInt32(kCaptureFrameDelay, value);
	else
		fSettings->ReplaceInt32(kCaptureFrameDelay, value);
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


bool
Settings::DockingMode() const
{
	bool docking;
	fSettings->FindBool(kDockingMode, &docking);
	return docking;
}


void
Settings::SetDockingMode(const bool& value)
{
	if (!fSettings->HasBool(kDockingMode))
		fSettings->AddBool(kDockingMode, value);
	else
		fSettings->ReplaceBool(kDockingMode, value);	
}


void
Settings::PrintToStream()
{
	fSettings->PrintToStream();
	TargetRect().PrintToStream();
}


status_t
Settings::_SetDefaults()
{
	BRect rect = BScreen().Frame();
	fSettings->MakeEmpty();
	fSettings->AddRect(kCaptureRect, rect);
	fSettings->AddString(kOutputFile, "/boot/home/outputfile.avi");
	fSettings->AddFloat(kClipScale, 100);
	fSettings->AddInt32(kClipDepth, B_RGB32);
	fSettings->AddBool(kIncludeCursor, true);
	fSettings->AddInt32(kThreadPriority, B_NORMAL_PRIORITY);
	fSettings->AddBool(kMinimize, false);
	fSettings->AddString(kOutputFileFormat, "");
	fSettings->AddString(kOutputCodecName, "");
	fSettings->AddInt32(kWindowFrameBorderSize, 0);
	fSettings->AddInt32(kCaptureFrameDelay, 20);
	fSettings->AddBool(kDockingMode, false);
	
	return B_OK;
}


Settings::Settings()
{
	fSettings = new BMessage;
	fLocker.Lock();
}


Settings::Settings(const Settings& settings)
{
	fSettings = new BMessage;
	*fSettings = *settings.fSettings;
	fLocker.Lock();
}


Settings::~Settings()
{
	fLocker.Unlock();
	delete fSettings;
}
