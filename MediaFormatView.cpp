/*
 * Copyright 2017-2021 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */

#include "MediaFormatView.h"
#include "BSCApp.h"
#include "Constants.h"
#include "ControllerObserver.h"
#include "Utils.h"

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


MediaFormatView::MediaFormatView()
	:
	BView("Media Options", B_WILL_DRAW),
	fOutputFileType(NULL),
	fCodecMenu(NULL)
{
	const char *kOutputMenuLabel = "File format:";
	BPopUpMenu *fileFormatPopUp = new BPopUpMenu("Format");
	fOutputFileType = new BMenuField("OutFormat",
			kOutputMenuLabel, fileFormatPopUp);
						
	const char *kCodecMenuLabel = "Media codec:";
	BPopUpMenu *popUpMenu = new BPopUpMenu("Codecs");
	fCodecMenu = new BMenuField("OutCodec", kCodecMenuLabel, popUpMenu);
	
	BLayoutBuilder::Grid<>(this, B_USE_DEFAULT_SPACING, B_USE_DEFAULT_SPACING)
		.Add(fOutputFileType->CreateLabelLayoutItem(), 0, 0)
		.Add(fOutputFileType->CreateMenuBarLayoutItem(), 1, 0)
		.Add(fCodecMenu->CreateLabelLayoutItem(), 0, 1)
		.Add(fCodecMenu->CreateMenuBarLayoutItem(), 1, 1)
		.AddGlue(0, 3)
		.SetInsets(0, 0);
}


MediaFormatView::~MediaFormatView()
{
}


void
MediaFormatView::AttachedToWindow()
{
	BView::AttachedToWindow();
	SetViewColor(ui_color(B_PANEL_BACKGROUND_COLOR));
	
	BSCApp* app = dynamic_cast<BSCApp*>(be_app);

	// Watch for these from Controller
	if (be_app->LockLooper()) {
		be_app->StartWatching(this, kMsgControllerEncodeStarted);
		be_app->StartWatching(this, kMsgControllerEncodeFinished);
		be_app->StartWatching(this, kMsgControllerSourceFrameChanged);
		be_app->StartWatching(this, kMsgControllerTargetFrameChanged);
		be_app->StartWatching(this, kMsgControllerCodecListUpdated);
		be_app->StartWatching(this, kMsgControllerMediaFileFormatChanged);
		be_app->StartWatching(this, kMsgControllerVideoDepthChanged);
		be_app->StartWatching(this, kMsgControllerCodecChanged);
		be_app->UnlockLooper();
	}
		
	_BuildFileFormatsMenu();
	
	BString currentFileFormat = app->MediaFileFormatName();
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
	BString codecName = app->MediaCodecName();
	if (codecName != "") {
		BMenuItem* codecItem = fCodecMenu->Menu()->FindItem(codecName);
		if (codecItem != NULL)
			codecItem->SetMarked(true);
	}
	_RebuildCodecsMenu(app->MediaCodecName());
}


void
MediaFormatView::MessageReceived(BMessage *message)
{
	BSCApp* app = dynamic_cast<BSCApp*>(be_app);

	switch (message->what) {
		case kLocalFileTypeChanged:
		{
			app->SetMediaFileFormat(_MediaFileFormat());
			app->SetMediaFormatFamily(_MediaFileFormat().family);
			app->UpdateMediaFormatAndCodecsForCurrentFamily();
			break;
		}
		case kLocalCodecChanged:
		{
			BMenuItem* marked = fCodecMenu->Menu()->FindMarked();
			if (marked != NULL)
				app->SetMediaCodec(marked->Label());
			break;				
		}
		case B_OBSERVER_NOTICE_CHANGE:
		{
			int32 code;
			message->FindInt32(B_OBSERVE_WHAT_CHANGE, &code);
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
					app->UpdateMediaFormatAndCodecsForCurrentFamily();
					break;
				case kMsgControllerEncodeStarted:
					fCodecMenu->SetEnabled(false);
					fOutputFileType->SetEnabled(false);
					break;
				case kMsgControllerEncodeFinished:
					fCodecMenu->SetEnabled(true);
					fOutputFileType->SetEnabled(true);
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

	media_file_format mediaFileFormat;
	int32 cookie = 0;
	while (get_next_file_format(&cookie, &mediaFileFormat) == B_OK) {
		if (IsFileFormatUsable(mediaFileFormat)) {
			MediaFileFormatMenuItem* item = new MediaFileFormatMenuItem(
					mediaFileFormat);
			menu->AddItem(item);
		}
	}

	media_file_format nullFormat;
	MakeNULLMediaFileFormat(nullFormat);
	MediaFileFormatMenuItem* nullItem = new MediaFileFormatMenuItem(nullFormat);
	menu->AddItem(nullItem);
	
	// TODO: Maybe Haiku could support this by enabling this in the ffmpeg addon ?
	if (IsFFMPEGAvailable()) {
		media_file_format gifFormat;
		MakeGIFMediaFileFormat(gifFormat);
		MediaFileFormatMenuItem* gifItem = new MediaFileFormatMenuItem(gifFormat);
		menu->AddItem(gifItem);
	}
}


void 
MediaFormatView::_RebuildCodecsMenu(const char* currentCodec)
{
	BSCApp* app = dynamic_cast<BSCApp*>(be_app);

	BMenu* codecsMenu = fCodecMenu->Menu();
	
	BString currentCodecString;
	if (currentCodec != NULL)
		currentCodecString = currentCodec;
	else {
		BMenuItem* item = codecsMenu->FindMarked();
		if (item != NULL)
			currentCodecString = item->Label();
	}		
	Window()->BeginViewTransaction();
		
	codecsMenu->RemoveItems(0, codecsMenu->CountItems(), true);
	
	BObjectList<media_codec_info> codecList(1, true);	
	if (app->GetCodecsList(codecList) == B_OK) {
		for (int32 i = 0; i < codecList.CountItems(); i++) {
			media_codec_info* codec = codecList.ItemAt(i);
			BMenuItem* item = new BMenuItem(codec->pretty_name, new BMessage(kLocalCodecChanged));
			codecsMenu->AddItem(item);
			if (codec->pretty_name == currentCodecString)
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
		if (currentCodecString != codecsMenu->FindMarked()->Label())
			app->SetMediaCodec(codecsMenu->FindMarked()->Label());
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
