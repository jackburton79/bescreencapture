#ifndef __SETTINGS_H
#define __SETTINGS_H

#include <Locker.h>
#include <GraphicsDefs.h>
#include <Rect.h>

class BMessage;
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
	void SetOutputFileFormat(const char* format);

	BString OutputCodec() const;
	void SetOutputCodec(const char* codecName);

	int32 CaptureFrameRate() const;
	void SetCaptureFrameRate(const int32& value);

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

	status_t _SetDefaults();

	BMessage *fSettings;
	mutable BLocker fLocker;
};

#endif

