#ifndef __OUTPUTVIEW_H
#define __OUTPUTVIEW_H

#include <MediaDefs.h>
#include <MediaFormats.h>
#include <Path.h>
#include <Size.h>
#include <View.h>

class BButton;
class BCheckBox;
class BMenu;
class BMenuField;
class BOptionPopUp;
class BRadioButton;
class BTextControl;
class Controller;
class PreviewView;
class OutputView : public BView {
public:	
	OutputView(Controller *controller);

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);
	
	void UpdateSettings();
	
	BPath OutputFileName() const;
	
	bool MinimizeOnStart() const;
		
	void UpdatePreview();
		
	virtual	BSize	MinSize();		
private:
	Controller *fController;
	BTextControl *fFileName;	
	BOptionPopUp *fOutputFileType;
	
	BMenuField *fCodecMenu;
	BCheckBox *fMinimizeOnStart;
	
	BButton *fSelectArea;
	BRadioButton *fWholeScreen;
	BRadioButton *fCustomArea;
	PreviewView *fRectView;
	
	media_format fFormat;
	
	media_format_family FormatFamily() const;
	media_format Format() const;
			
	void BuildCodecMenu(const media_format_family &family);
	status_t GetCodecsForFamily(const media_format_family &,
				const int32 &width, const int32 &height,
				BMenu *, media_format &outFormat);

	void _UpdatePreview();
	static void SetInitialFormat(const int32 &width, const int32 &height,
				const color_space &space, const int32 &fieldRate,
				media_format &initialFormat);
	static BMenuItem *CreateCodecMenuItem(const media_codec_info &codec);
	
	
};


#endif
