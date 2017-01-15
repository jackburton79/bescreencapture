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
class BRadioButton;
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
	
	void SetClassicLayout(bool classic);
	
	BPath OutputFileName() const;
	
	bool MinimizeOnStart() const;

	void UpdatePreviewFromSettings();
			
private:
	Controller *fController;
	
	BTextControl *fFileName;
	BString fFileExtension;
	BButton* fFilePanelButton;
	
	BCheckBox *fMinimizeOnStart;
	
	BButton *fSelectArea;
	BRadioButton *fWholeScreen;
	BRadioButton *fCustomArea;
	BRadioButton *fWindow;
	
	PreviewView *fRectView;
	BRect fCustomCaptureRect;

	SizeControl* fScaleSlider;
	SizeControl* fBorderSlider;

	bool fClassicLayout;
	
	void _LayoutView(bool classic);
	void _SetFileNameExtension(const char* extension);
	void _UpdatePreview(BRect* rect, BBitmap* bitmap = NULL);
	
	BRect _CaptureRect() const;
};


#endif
