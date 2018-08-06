/*
 * Copyright 2017 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "MediaFormatView.h"

#include "Constants.h"
#include "Controller.h"
#include "ControllerObserver.h"

#include <Button.h>
#include <LayoutBuilder.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <PopUpMenu.h>
#include <TextControl.h>

#include <iostream>

const static int32 kLocalCodecChanged = 'CdCh';
const static int32 kLocalFileTypeChanged = 'FtyC';

class MediaFileFormatMenuItem : public BMenuItem {
public:
	MediaFileFormatMenuItem(const media_file_format& fileFormat);
	media_file_format MediaFileFormat() const;
private:
	media_file_format fFileFormat;
};


MediaFormatView::MediaFormatView(Controller *controller)
	:
	BView("Media Options", B_WILL_DRAW),
	fController(controller),
	fOutputFileType(NULL),
	fCodecMenu(NULL)
{
	SetLayout(new BGroupLayout(B_VERTICAL));
	
	const char *kOutputMenuLabel = "File format:";
	BPopUpMenu *fileFormatPopUp = new BPopUpMenu("Format");
	fOutputFileType = new BMenuField("OutFormat",
			kOutputMenuLabel, fileFormatPopUp);
						
	const char *kCodecMenuLabel = "Media codec:";
	BPopUpMenu *popUpMenu = new BPopUpMenu("Codecs");
	fCodecMenu = new BMenuField("OutCodec", kCodecMenuLabel, popUpMenu);
	
	BView *layoutView = BLayoutBuilder::Group<>()
		.SetInsets(0, 0, 0, 0)
		.AddGroup(B_VERTICAL, B_USE_DEFAULT_SPACING)
			.Add(fOutputFileType)
			.Add(fCodecMenu)
			.SetInsets(0)
		.End()	
		.View();

	AddChild(layoutView);	
}


MediaFormatView::~MediaFormatView()
{
}


void
MediaFormatView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	// Watch for these from Controller
	if (fController->LockLooper()) {
		fController->StartWatching(this, kMsgControllerSourceFrameChanged);
		fController->StartWatching(this, kMsgControllerTargetFrameChanged);
		fController->StartWatching(this, kMsgControllerCodecListUpdated);
		fController->StartWatching(this, kMsgControllerMediaFileFormatChanged);
		fController->StartWatching(this, kMsgControllerVideoDepthChanged);
		fController->StartWatching(this, kMsgControllerCodecChanged);
		fController->UnlockLooper();
	}
		
	_BuildFileFormatsMenu();
	
	BString currentFileFormat = fController->MediaFileFormatName();
	BMenuItem* item = NULL;
	if (currentFileFormat != "")
		item = fOutputFileType->Menu()->FindItem(currentFileFormat.String());
	if (item != NULL)
		item->SetMarked(true);
	else {
		// TODO: This means there is no working media encoder.;
		// do something smart (like showing an alert and disable
		// the "Start" button
		// TODO: this should be done at the Controller level
	}
	
	fOutputFileType->Menu()->SetTargetForItems(this);
	BString codecName = fController->MediaCodecName();
	if (codecName != "") {
		BMenuItem* item = fCodecMenu->Menu()->FindItem(codecName);
		if (item != NULL)
			item->SetMarked(true);
	}
	_RebuildCodecsMenu(fController->MediaCodecName());
}


void
MediaFormatView::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case kLocalFileTypeChanged:
		{
			fController->SetMediaFileFormat(_MediaFileFormat());
			fController->SetMediaFormatFamily(_MediaFileFormat().family);
			fController->UpdateMediaFormatAndCodecsForCurrentFamily();
			break;
		}
		case kLocalCodecChanged:
		{
			BMenuItem* marked = fCodecMenu->Menu()->FindMarked();
			if (marked != NULL)
				fController->SetMediaCodec(marked->Label());
			break;				
		}
					
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32("be:observe_change_what", &code);
			switch (code) {
				case kMsgControllerCodecListUpdated:
					_RebuildCodecsMenu();
					break;
				
				case kMsgControllerMediaFileFormatChanged:
				{
					const char* formatName = NULL;
					message->FindString("format_name", &formatName);
					_SelectFileFormatMenuItem(formatName);					
					break;
				}
				case kMsgControllerCodecChanged:
				{
					const char* codecName = NULL;
					message->FindString("codec_name", &codecName);
					for (int32 i = 0; i < fCodecMenu->Menu()->CountItems(); i++) {
						BMenuItem* item = fCodecMenu->Menu()->ItemAt(i);
						if (strcmp(item->Label(), codecName) == 0) {
							item->SetMarked(true);
							break;
						}
					}
					break;
				}	
				case kMsgControllerVideoDepthChanged:
				case kMsgControllerTargetFrameChanged:
					fController->UpdateMediaFormatAndCodecsForCurrentFamily();
					break;
				default:
					break;
			}
			break;
		}

		default:
			BView::MessageReceived(message);
			break;
	}
}


void
MediaFormatView::_BuildFileFormatsMenu()
{
	BMenu* menu = fOutputFileType->Menu();
	if (menu == NULL)
		return;

	const int32 numItems = menu->CountItems();
	if (numItems > 0)
		menu->RemoveItems(0, numItems);

#if 1
	const uint32 mediaFormatMask = media_file_format::B_KNOWS_ENCODED_VIDEO
								| media_file_format::B_WRITABLE;
	media_file_format mediaFileFormat;
	int32 cookie = 0;
	while (get_next_file_format(&cookie, &mediaFileFormat) == B_OK) {
		if ((mediaFileFormat.capabilities & mediaFormatMask) == mediaFormatMask) {
			MediaFileFormatMenuItem* item = new MediaFileFormatMenuItem(
					mediaFileFormat);
			menu->AddItem(item);
		}
	}
#endif
	media_file_format fakeFormat;
	strncpy(fakeFormat.pretty_name, "Export frames as Bitmaps", sizeof(fakeFormat.pretty_name));
	strncpy(fakeFormat.short_name, FAKE_FORMAT_SHORT_NAME, sizeof(fakeFormat.short_name));
	fakeFormat.capabilities = media_file_format::B_KNOWS_OTHER;
	fakeFormat.file_extension[0] = '\0';
	fakeFormat.family = B_ANY_FORMAT_FAMILY;
	MediaFileFormatMenuItem* item = new MediaFileFormatMenuItem(fakeFormat);
	menu->AddItem(item);
}


void 
MediaFormatView::_RebuildCodecsMenu(const char* codec)
{
	BMenu* codecsMenu = fCodecMenu->Menu();
	
	BString currentCodec;
	if (codec != NULL)
		currentCodec = codec;
	else {
		BMenuItem* item = codecsMenu->FindMarked();
		if (item != NULL)
			currentCodec = item->Label();
	}		
	Window()->BeginViewTransaction();
		
	codecsMenu->RemoveItems(0, codecsMenu->CountItems(), true);
	
	BObjectList<media_codec_info> codecList(1, true);	
	if (fController->GetCodecsList(codecList) == B_OK) {
		for (int32 i = 0; i < codecList.CountItems(); i++) {
			media_codec_info* codec = codecList.ItemAt(i);
			BMenuItem* item = new BMenuItem(codec->pretty_name, new BMessage(kLocalCodecChanged));
			codecsMenu->AddItem(item);
			if (codec->pretty_name == currentCodec)
				item->SetMarked(true);
		}			
		// Make the app object the menu's message target
		fCodecMenu->Menu()->SetTargetForItems(this);
	}
	
	if (codecsMenu->FindMarked() == NULL) {
		BMenuItem *item = codecsMenu->ItemAt(0);
		if (item != NULL)
			item->SetMarked(true);
	}
	
	Window()->EndViewTransaction();
	
	if (codecsMenu->FindMarked() == NULL) {
		codecsMenu->SetEnabled(false);
	} else {
		if (currentCodec != codecsMenu->FindMarked()->Label())
			fController->SetMediaCodec(codecsMenu->FindMarked()->Label());
		codecsMenu->SetEnabled(true);
	}
}


// convenience method
media_file_format
MediaFormatView::_MediaFileFormat() const
{
	MediaFileFormatMenuItem* item = static_cast<MediaFileFormatMenuItem*>(
			fOutputFileType->Menu()->FindMarked());
	return item->MediaFileFormat();
}


void
MediaFormatView::_SelectFileFormatMenuItem(const char* formatName)
{
	if (formatName != NULL) {
		for (int32 i = 0; i < fOutputFileType->Menu()->CountItems(); i++) {
			MediaFileFormatMenuItem* item = static_cast<MediaFileFormatMenuItem*>(fOutputFileType->Menu()->ItemAt(i));
			if (strcmp(item->MediaFileFormat().pretty_name, formatName) == 0)
				item->SetMarked(true);
		}
	}
}




// MediaFileFormatMenuItem
MediaFileFormatMenuItem::MediaFileFormatMenuItem(const media_file_format& fileFormat)
	:
	BMenuItem(fileFormat.pretty_name, new BMessage(kLocalFileTypeChanged)),
	fFileFormat(fileFormat)
{

}


media_file_format
MediaFileFormatMenuItem::MediaFileFormat() const
{
	return fFileFormat;
}

