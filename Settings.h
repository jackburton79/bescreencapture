#ifndef __SETTINGS_H
#define __SETTINGS_H

#include <Locker.h>
#include <GraphicsDefs.h>
#include <Rect.h>

class BMessage;
class BString;

class Settings {
public:
	Settings();
	~Settings();
	
	static status_t Load();
	static status_t Save();
		
	void SetCaptureArea(const BRect &rect);
	void GetCaptureArea(BRect &rect) const;
	BRect CaptureArea() const;
	
	void GetTargetRect(BRect& rect) const;
	BRect TargetRect() const;
		
	void SetClipDepth(const color_space &space);
	void GetClipDepth(color_space &space) const;
	color_space ClipDepth() const;
	
	void SetScale(const float &scale);
	void GetScale(float &scale) const;
	float Scale() const;
	
	void SetUseDirectWindow(const bool &use);
	void GetUseDirectWindow(bool &use) const;
	bool UseDirectWindow() const;
		
	void SetIncludeCursor(const bool &include);
	void GetIncludeCursor(bool &include) const;
	bool IncludeCursor() const;
	
	void SetWindowFrameBorderSize(const int32 &size);
	void GetWindowFrameBorderSize(int32 &size) const;
	int32 WindowFrameBorderSize() const;
	
	void SetMinimizeOnRecording(const bool &minimize);
	void GetMinimizeOnRecording(bool &minimize) const;
	bool MinimizeOnRecording() const;
	
	BString OutputFileName() const;
	void SetOutputFileName(const char *name);
	void GetOutputFileName(BString &name) const;
	
	void SetOutputFileFormat(const char* format);
	void GetOutputFileFormat(BString& format) const;

	void SetOutputCodec(const char* codecName);
	void GetOutputCodec(BString& codecName) const;
	
	int32 CaptureFrameDelay() const;
	void SetCaptureFrameDelay(const int32& value);
	
	void SetEncodingThreadPriority(const int32 &value);
	void GetEncodingThreadPriority(int32 &value) const;
	int32 EncodingThreadPriority() const;
	
	bool DockingMode() const;
	void SetDockingMode(const bool& value);
	
	void PrintToStream();

	static status_t SetDefaults();

private:
	BMessage *fSettings;
	BLocker fLocker;	
};

#endif

