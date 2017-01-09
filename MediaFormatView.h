/*
 * Copyright 2017 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __MEDIAFORMATVIEW_H
#define __MEDIAFORMATVIEW_H


#include <String.h>
#include <View.h>
#include <MediaFormats.h>

class BMenuField;
class BTextControl;
class BMenuField;
class BMenuItem;
class BButton;

class Controller;
class MediaFormatView : public BView {
public:
	MediaFormatView(Controller *controller);
	virtual ~MediaFormatView();
	
	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);
	void RequestMediaFormatUpdate();
								
private:
	Controller *fController;
	BTextControl *fFileName;
	BString fFileExtension;
	BMenuField *fOutputFileType;
	BMenuField *fCodecMenu;
	
	BButton* fFilePanelButton;

	void _BuildFileFormatsMenu();
	void _RebuildCodecsMenu();

	void _SetFileNameExtension(const char* extension);
	
	media_file_format _MediaFileFormat() const;
	void _SelectFileFormatMenuItem(const char* formatName);
	
	static BMenuItem *CreateCodecMenuItem(const media_codec_info &codec);
};


#endif // __MEDIAFORMATVIEW_H
