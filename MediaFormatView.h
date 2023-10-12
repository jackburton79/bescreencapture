/*
 * Copyright 2017-2023 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __MEDIAFORMATVIEW_H
#define __MEDIAFORMATVIEW_H

#include <MediaFormats.h>
#include <String.h>
#include <View.h>

class BMenuField;
class MediaFormatView : public BView {
public:
	MediaFormatView();
	virtual ~MediaFormatView();

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);

	void RequestMediaFormatUpdate();

private:
	BMenuField *fOutputFileType;
	BMenuField *fCodecMenu;

	void _BuildFileFormatsMenu();
	void _RebuildCodecsMenu(const char* currentCodec = NULL);

	void _SetFileNameExtension(const char* extension);

	media_file_format _MediaFileFormat() const;
	void _SelectFileFormatMenuItem(const char* formatName);
};


#endif // __MEDIAFORMATVIEW_H
