/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

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
	
	BFile file;
	status_t status = _LoadSettingsFile(file, B_READ_ONLY);
	BMessage tempMessage;
	if (status == B_OK)	
		status = tempMessage.Unflatten(&file);
	
	if (status == B_OK) {
		// Copy the loaded fields to the real settings message
		// N.B: We only copy "known" settings
		// This is okay for dropping old and incompatible settings,
		// but it's still inconvenient since we have to add the default
		// settings in _SetDefault() and a check here for every new setting
		// we introduce
		BRect rect;
		bool boolean;
		int32 integer;
		float decimal;
		const char *string = NULL;
		if (tempMessage.FindRect(kCaptureRect, &rect) == B_OK)
			fSettings->SetRect(kCaptureRect, rect);
		if (tempMessage.FindInt32(kClipDepth, &integer) == B_OK)
			fSettings->SetInt32(kClipDepth, integer);
		if (tempMessage.FindFloat(kClipScale, &decimal) == B_OK)
			fSettings->SetFloat(kClipScale, decimal);
		if (tempMessage.FindBool(kUseDirectWindow, &boolean) == B_OK)
			fSettings->SetBool(kUseDirectWindow, boolean);
		if (tempMessage.FindBool(kIncludeCursor, &boolean) == B_OK)
			fSettings->SetBool(kIncludeCursor, boolean);
		if (tempMessage.FindBool(kMinimize, &boolean) == B_OK)
			fSettings->SetBool(kMinimize, boolean);
		if (tempMessage.FindString(kOutputFile, &string) == B_OK)
			fSettings->SetString(kOutputFile, string);
		if (tempMessage.FindString(kOutputFileFormat, &string) == B_OK)
			fSettings->SetString(kOutputFileFormat, string);
		if (tempMessage.FindString(kOutputCodecName, &string) == B_OK)
			fSettings->SetString(kOutputCodecName, string);
		if (tempMessage.FindInt32(kThreadPriority, &integer) == B_OK)
			fSettings->SetInt32(kThreadPriority, integer);
		if (tempMessage.FindInt32(kWindowFrameBorderSize, &integer) == B_OK)
			fSettings->SetInt32(kWindowFrameBorderSize, integer);
		if (tempMessage.FindInt32(kCaptureFrameRate, &integer) == B_OK)
			fSettings->SetInt32(kCaptureFrameRate, integer);
		if (tempMessage.FindBool(kDockingMode, &boolean) == B_OK)
			fSettings->SetBool(kDockingMode, boolean);
	}	
	
	return status;
}


status_t
Settings::Save()
{
	BAutolock _(fLocker);
	
	BFile file;
	status_t status = _LoadSettingsFile(file, B_WRITE_ONLY|B_CREATE_FILE);

	if (status == B_OK)	
		status = fSettings->Flatten(&file);
	
	return status;
}


void
Settings::SetCaptureArea(const BRect &rect)
{
	BAutolock _(fLocker);
	fSettings->SetRect(kCaptureRect, rect);
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
	fSettings->SetInt32(kClipDepth, spaceInt);
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
	fSettings->SetFloat(kClipScale, scale);
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
	fSettings->SetBool(kUseDirectWindow, use);
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
	fSettings->SetBool(kIncludeCursor, include);
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
	fSettings->SetInt32(kWindowFrameBorderSize, size);
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
Settings::SetMinimizeOnRecording(const bool &minimize)
{
	BAutolock _(fLocker);
	fSettings->SetBool(kMinimize, minimize);
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
	fSettings->SetString(kOutputFile, name);
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
	fSettings->SetString(kOutputFileFormat, fileFormat);
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
	fSettings->SetString(kOutputCodecName, codecName);
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
	fSettings->SetInt32(kCaptureFrameRate, value);
}


void
Settings::SetEncodingThreadPriority(const int32 &value)
{
	BAutolock _(fLocker);
	fSettings->SetInt32(kThreadPriority, value);
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
	fSettings->SetBool(kDockingMode, value);
}


void
Settings::PrintToStream()
{
	BAutolock _(fLocker);
	fSettings->PrintToStream();
	TargetRect().PrintToStream();
}


status_t
Settings::_LoadSettingsFile(BFile& file, int mode)
{
	BPath path;
	status_t status = find_directory(B_USER_SETTINGS_DIRECTORY, &path);
	if (status == B_OK)
		status = path.Append("BeScreenCapture");
	if (status == B_OK)
		status = file.SetTo(path.Path(), mode);
	return status;
}


status_t
Settings::_SetDefaults()
{
	BAutolock _(fLocker);

	BRect rect = BScreen().Frame();
	fSettings->MakeEmpty();
	fSettings->SetRect(kCaptureRect, rect);
	fSettings->SetString(kOutputFile, "/boot/home/outputfile.mpg");
	fSettings->SetFloat(kClipScale, 100);
	fSettings->SetInt32(kClipDepth, B_RGB32);
	fSettings->SetBool(kIncludeCursor, true);
	fSettings->SetInt32(kThreadPriority, B_NORMAL_PRIORITY);
	fSettings->SetBool(kMinimize, false);
	fSettings->SetString(kOutputFileFormat, "");
	fSettings->SetString(kOutputCodecName, "");
	fSettings->SetInt32(kWindowFrameBorderSize, 0);
	fSettings->SetInt32(kCaptureFrameRate, 20);
	fSettings->SetBool(kDockingMode, false);
	
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
