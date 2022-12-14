/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#ifndef __SETTINGS_H
#define __SETTINGS_H

#include <Locker.h>
#include <GraphicsDefs.h>
#include <Rect.h>

class BFile;
class BMessage;
class BPath;
class BString;

class Settings {
public:
	static status_t Initialize();
	static void Destroy();

	static Settings& Default();
	static Settings& Current();
	static void ResetToDefaults();

	status_t Load();
	status_t Save();

	BRect CaptureArea() const;
	void SetCaptureArea(const BRect &rect);

	BRect TargetRect() const;
	void SetTargetRect(const BRect& rect) const;

	color_space ClipDepth() const;
	void SetClipDepth(const color_space &space);

	float Scale() const;
	void SetScale(const float &scale);

	bool UseDirectWindow() const;
	void SetUseDirectWindow(const bool &use);

	bool IncludeCursor() const;
	void SetIncludeCursor(const bool &include);

	int32 WindowFrameEdgeSize() const;
	void SetWindowFrameEdgeSize(const int32 &size);

	bool MinimizeOnRecording() const;
	void SetMinimizeOnRecording(const bool &minimize);

	BString OutputFileName() const;
	void SetOutputFileName(const char *name);

	BString OutputFileFormat() const;
	void SetOutputFileFormat(const char* fileFormat);

	BString OutputCodec() const;
	void SetOutputCodec(const char* codecName);

	int32 CaptureFrameRate() const;
	void SetCaptureFrameRate(const int32& value);

	void SetWarnOnQuit(const bool& warn);
	bool WarnOnQuit() const;
	
	int32 EncodingThreadPriority() const;
	void SetEncodingThreadPriority(const int32 &value);

	bool DockingMode() const;
	void SetDockingMode(const bool& value);

	void PrintToStream();

private:
	Settings();
	Settings(const Settings& settings);
	~Settings();

	Settings& operator=(const Settings& settings);

	status_t _LoadSettingsFile(BFile& file, int mode);
	status_t _SetDefaults();
	status_t _GetSettingsPath(BPath& path);

	BMessage* fSettings;
	mutable BLocker fLocker;
};

#endif

