#ifndef __OUTPUTVIEW_H
#define __OUTPUTVIEW_H

#include <MediaDefs.h>
#include <MediaFormats.h>
#include <Path.h>
#include <Size.h>
#include <String.h>
#include <View.h>

class BButton;
class BCheckBox;
class BMenu;
class BMenuField;
class BMessageRunner;
class BRadioButton;
class BStringView;
class BTextControl;
class Controller;
class MediaFormatView;
class PreviewView;
class SizeControl;
class OutputView : public BView {
public:	
	OutputView(Controller *controller);

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);
	virtual void WindowActivated(bool active);
	
	BPath OutputFileName() const;
	
	void UpdatePreviewFromSettings();
			
private:
	Controller *fController;
	
	BTextControl *fFileName;
	BString fFileExtension;
	BButton* fFilePanelButton;
	
	BButton *fSelectAreaButton;
	BRadioButton *fWholeScreen;
	BRadioButton *fCustomArea;
	BRadioButton *fWindow;
	
	PreviewView *fRectView;
	BRect fCustomCaptureRect;

	SizeControl* fScaleSlider;
	SizeControl* fBorderSlider;
	
	void _LayoutView();
	void _SetFileNameExtension(const char* extension);
	void _UpdatePreview(BRect* rect, BBitmap* bitmap = NULL);
	void _HandleExistingFileName(const char* fileName);
	
	BRect _CaptureRect() const;
};


#endif
