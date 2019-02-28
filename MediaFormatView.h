/*
 * Copyright 2017 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __MEDIAFORMATVIEW_H
#define __MEDIAFORMATVIEW_H


#include <MediaFormats.h>
#include <String.h>
#include <View.h>


class BMenuField;
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
	BMenuField *fOutputFileType;
	BMenuField *fCodecMenu;
	
	void _BuildFileFormatsMenu();
	void _RebuildCodecsMenu(const char* currentCodec = NULL);

	void _SetFileNameExtension(const char* extension);
	
	media_file_format _MediaFileFormat() const;
	void _SelectFileFormatMenuItem(const char* formatName);
};


#endif // __MEDIAFORMATVIEW_H
