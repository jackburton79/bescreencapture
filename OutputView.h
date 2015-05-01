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
class SizeControl;
class BTextControl;
class Controller;
class PreviewView;
class OutputView : public BView {
public:	
	OutputView(Controller *controller);

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);
	
	void RequestMediaFormatUpdate();
	
	BPath OutputFileName() const;

	media_file_format FileFormat() const;
	media_format_family FormatFamily() const;
	
	bool MinimizeOnStart() const;
		
	void UpdatePreviewFromSettings();
			
private:
	Controller *fController;
	BTextControl *fFileName;
	BString fFileExtension;
	BMenuField *fOutputFileType;
	BMenuField *fCodecMenu;
	BCheckBox *fMinimizeOnStart;
	
	BButton *fSelectArea;
	BRadioButton *fWholeScreen;
	BRadioButton *fCustomArea;
	PreviewView *fRectView;
	BRect fCustomCaptureRect;
	BButton* fFilePanelButton;
	SizeControl* fSizeSlider;

	void _BuildFileFormatsMenu();
	void _RebuildCodecsMenu();

	void _SetFileNameExtension(const char* extension);
	void _UpdatePreview(BRect* rect, BBitmap* bitmap = NULL);
	
	static BMenuItem *CreateCodecMenuItem(const media_codec_info &codec);
};


#endif
