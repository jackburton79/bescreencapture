/*
 * Copyright 2017 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "MediaFormatView.h"

#include "Controller.h"
#include "ControllerObserver.h"
#include "Messages.h"

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


static void
BuildFileFormatsMenu(BMenu* menu)
{
	const int32 numItems = menu->CountItems();
	if (numItems > 0)
		menu->RemoveItems(0, numItems);

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
}


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
		fController->UnlockLooper();
	}
		
	BuildFileFormatsMenu(fOutputFileType->Menu());
	
	BString savedFileFormat = fController->MediaFileFormatName();
	std::cout << savedFileFormat.String() << std::endl;
	BMenuItem* item = NULL;
	if (savedFileFormat != "")
		item = fOutputFileType->Menu()->FindItem(savedFileFormat.String());

	if (item == NULL)
		item = fOutputFileType->Menu()->ItemAt(0);

	if (item != NULL)
		item->SetMarked(true);
	else {
		// TODO: This means there is no working media encoder.;
		// do something smart (like showing an alert and disable
		// the "Start" button
		// TODO: this should be done at the Controller level
	}
	
	fOutputFileType->Menu()->SetTargetForItems(this);
	
	_RebuildCodecsMenu();
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
	const int32 numItems = fOutputFileType->Menu()->CountItems();
	if (numItems > 0)
		fOutputFileType->Menu()->RemoveItems(0, numItems);

	const uint32 mediaFormatMask = media_file_format::B_KNOWS_ENCODED_VIDEO
								| media_file_format::B_WRITABLE;
	media_file_format mediaFileFormat;
	int32 cookie = 0;
	while (get_next_file_format(&cookie, &mediaFileFormat) == B_OK) {
		if ((mediaFileFormat.capabilities & mediaFormatMask) == mediaFormatMask) {
			MediaFileFormatMenuItem* item = new MediaFileFormatMenuItem(
					mediaFileFormat);
			fOutputFileType->Menu()->AddItem(item);
		}
	}
}


void 
MediaFormatView::_RebuildCodecsMenu()
{
	BMenu* codecsMenu = fCodecMenu->Menu();
	
	BString currentCodec;
	BMenuItem *marked = codecsMenu->FindMarked();
	if (marked != NULL)
		currentCodec = marked->Label();
		
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
	
	if (currentCodec != codecsMenu->FindMarked()->Label())
		fController->SetMediaCodec(codecsMenu->FindMarked()->Label());
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

