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
	
	void SetHideDeskbarIcon(const bool &use);
	void GetHideDeskbarIcon(bool &use) const;
	bool HideDeskbarIcon() const;
	
	void SetIncludeCursor(const bool &include);
	void GetIncludeCursor(bool &include) const;
	bool IncludeCursor() const;
	
	void SetMinimizeOnRecording(const bool &minimize);
	void GetMinimizeOnRecording(bool &minimize) const;
	bool MinimizeOnRecording() const;
	
	void SetOutputFileName(const char *name);
	void GetOutputFileName(const char **name) const;
	void GetOutputFileName(BString &name) const;
	
	void SetEncodingThreadPriority(const int32 &value);
	void GetEncodingThreadPriority(int32 &value) const;
	int32 EncodingThreadPriority() const;
	
private:
	static status_t SetDefaults();
	
	BMessage *fSettings;
	BLocker fLocker;	
};

#endif

