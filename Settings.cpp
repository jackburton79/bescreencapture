#include "Settings.h"

#include <Autolock.h>
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
const static char *kCaptureFrameRate = "capture frame rate";
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
	BAutolock _(fLocker);
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
		if (tempMessage.FindInt32(kCaptureFrameRate, &integer) == B_OK)
			fSettings->ReplaceInt32(kCaptureFrameRate, integer);
		if (tempMessage.FindBool(kDockingMode, &boolean) == B_OK)
			fSettings->ReplaceBool(kDockingMode, boolean);
	}	
	
	return status;
}


status_t
Settings::Save()
{
	BAutolock _(fLocker);
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
	BAutolock _(fLocker);
	if (!fSettings->HasRect(kCaptureRect))
		fSettings->AddRect(kCaptureRect, rect);
	else
		fSettings->ReplaceRect(kCaptureRect, rect);
}


BRect
Settings::CaptureArea() const
{
	BAutolock _(fLocker);
	BRect rect;
	fSettings->FindRect(kCaptureRect, &rect);
	return rect;
}


BRect
Settings::TargetRect() const
{
	BAutolock _(fLocker);
	const float scale = Scale();
	BRect scaledRect = CaptureArea();

	scaledRect.OffsetTo(B_ORIGIN);
	scaledRect.right = roundf((scaledRect.right + 1) * scale / 100 - 1);
	scaledRect.bottom = roundf((scaledRect.bottom + 1) * scale / 100 - 1);

	return scaledRect;
}


void
Settings::SetClipDepth(const color_space &space)
{
	BAutolock _(fLocker);
	const int32 &spaceInt = (int32)space;
	if (!fSettings->HasInt32(kClipDepth))
		fSettings->AddInt32(kClipDepth, spaceInt);
	else
		fSettings->ReplaceInt32(kClipDepth, spaceInt);
}


color_space
Settings::ClipDepth() const
{
	BAutolock _(fLocker);
	color_space depth;
	fSettings->FindInt32(kClipDepth, (int32 *)&depth);
	return depth;
}


void
Settings::SetScale(const float &scale)
{
	BAutolock _(fLocker);
	if (!fSettings->HasFloat(kClipScale))
		fSettings->AddFloat(kClipScale, scale);
	else
		fSettings->ReplaceFloat(kClipScale, scale);
}


float
Settings::Scale() const
{
	BAutolock _(fLocker);
	float scale;
	fSettings->FindFloat(kClipScale, &scale);
	return scale;
}


void
Settings::SetUseDirectWindow(const bool &use)
{
	BAutolock _(fLocker);
	if (!fSettings->HasBool(kUseDirectWindow))
		fSettings->AddBool(kUseDirectWindow, use);
	else
		fSettings->ReplaceBool(kUseDirectWindow, use);
}


bool
Settings::UseDirectWindow() const
{
	BAutolock _(fLocker);
	bool useDW;
	fSettings->FindBool(kUseDirectWindow, &useDW);
	return useDW;
}


void
Settings::SetIncludeCursor(const bool &include)
{
	BAutolock _(fLocker);
	if (!fSettings->HasBool(kIncludeCursor))
		fSettings->AddBool(kIncludeCursor, include);
	else
		fSettings->ReplaceBool(kIncludeCursor, include);
}


bool
Settings::IncludeCursor() const
{
	BAutolock _(fLocker);
	bool include;
	fSettings->FindBool(kIncludeCursor, &include);
	return include;
}


void
Settings::SetWindowFrameEdgeSize(const int32 &size)
{
	BAutolock _(fLocker);
	if (!fSettings->HasInt32(kWindowFrameBorderSize))
		fSettings->AddInt32(kWindowFrameBorderSize, size);
	else
		fSettings->ReplaceInt32(kWindowFrameBorderSize, size);
}


int32
Settings::WindowFrameEdgeSize() const
{
	BAutolock _(fLocker);
	int32 size = 0;
	fSettings->FindInt32(kWindowFrameBorderSize, &size);
	return size;
}	
	
	
void
Settings::SetMinimizeOnRecording(const bool &mini)
{
	BAutolock _(fLocker);
	if (!fSettings->HasBool(kMinimize))
		fSettings->AddBool(kMinimize, mini);
	else
		fSettings->ReplaceBool(kMinimize, mini);
}


bool
Settings::MinimizeOnRecording() const
{
	BAutolock _(fLocker);
	bool mini;
	fSettings->FindBool(kMinimize, &mini);
	return mini;
}


void
Settings::SetOutputFileName(const char *name)
{
	BAutolock _(fLocker);
	if (!fSettings->HasString(kOutputFile))
		fSettings->AddString(kOutputFile, name);
	else
		fSettings->ReplaceString(kOutputFile, name);
}


BString
Settings::OutputFileName() const
{
	BAutolock _(fLocker);
	BString name;
	fSettings->FindString(kOutputFile, &name);
	return name;
}


BString
Settings::OutputFileFormat() const
{
	BAutolock _(fLocker);
	BString format;
	fSettings->FindString(kOutputFileFormat, &format);
	return format;
}


void
Settings::SetOutputFileFormat(const char* fileFormat)
{
	BAutolock _(fLocker);
	if (!fSettings->HasString(kOutputFileFormat))
		fSettings->AddString(kOutputFileFormat, fileFormat);
	else
		fSettings->ReplaceString(kOutputFileFormat, fileFormat);
}


BString
Settings::OutputCodec() const
{
	BAutolock _(fLocker);
	BString codec;
	fSettings->FindString(kOutputCodecName, &codec);
	return codec;
}


void
Settings::SetOutputCodec(const char* codecName)
{
	BAutolock _(fLocker);
	if (!fSettings->HasString(kOutputCodecName))
		fSettings->AddString(kOutputCodecName, codecName);
	else
		fSettings->ReplaceString(kOutputCodecName, codecName);
}


int32
Settings::CaptureFrameRate() const
{
	BAutolock _(fLocker);
	int32 value;
	fSettings->FindInt32(kCaptureFrameRate, &value);
	return value;
}


void
Settings::SetCaptureFrameRate(const int32& value)
{
	BAutolock _(fLocker);
	if (!fSettings->HasInt32(kCaptureFrameRate))
		fSettings->AddInt32(kCaptureFrameRate, value);
	else
		fSettings->ReplaceInt32(kCaptureFrameRate, value);
}


void
Settings::SetEncodingThreadPriority(const int32 &value)
{
	BAutolock _(fLocker);
	if (!fSettings->HasInt32(kThreadPriority))
		fSettings->AddInt32(kThreadPriority, value);
	else
		fSettings->ReplaceInt32(kThreadPriority, value);
}


int32
Settings::EncodingThreadPriority() const
{
	BAutolock _(fLocker);
	int32 prio = B_NORMAL_PRIORITY;
	fSettings->FindInt32(kThreadPriority, &prio);
	return prio;
}


bool
Settings::DockingMode() const
{
	BAutolock _(fLocker);
	bool docking;
	fSettings->FindBool(kDockingMode, &docking);
	return docking;
}


void
Settings::SetDockingMode(const bool& value)
{
	BAutolock _(fLocker);
	if (!fSettings->HasBool(kDockingMode))
		fSettings->AddBool(kDockingMode, value);
	else
		fSettings->ReplaceBool(kDockingMode, value);	
}


void
Settings::PrintToStream()
{
	BAutolock _(fLocker);
	fSettings->PrintToStream();
	TargetRect().PrintToStream();
}


status_t
Settings::_SetDefaults()
{
	BAutolock _(fLocker);

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
	fSettings->AddInt32(kCaptureFrameRate, 20);
	fSettings->AddBool(kDockingMode, false);
	
	return B_OK;
}


Settings::Settings()
	:
	fSettings(new BMessage),
	fLocker("settings locker")
{
}


Settings::Settings(const Settings& settings)
	:
	fSettings(new BMessage),
	fLocker("settings locker")
{
	*fSettings = *settings.fSettings;
}


Settings::~Settings()
{
	delete fSettings;
}
