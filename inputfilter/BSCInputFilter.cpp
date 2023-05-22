/*
 * Copyright 2013-2021, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include <InputServerFilter.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Roster.h>

#include "BSCApp.h"
#include "PublicMessages.h"
#include "Settings.h"

class BSCInputFilter : public BInputServerFilter {
public:
	BSCInputFilter();
	virtual ~BSCInputFilter();
	status_t		InitCheck();
	filter_result	Filter(BMessage* message, BList* outList);
	void SetEnabled(bool enabled);
private:
	BLooper* fLooper;
	BMessenger fMessenger;
	node_ref fNodeRef;
	bool fEnabled;
};

class InputFilterLooper : public BLooper {
public:
	InputFilterLooper(BSCInputFilter* filter) : BLooper("BSCInputFilter looper") {
		fFilter = filter;
	};
	virtual void MessageReceived(BMessage* message) {
		switch (message->what) {
			case B_NODE_MONITOR:
				Settings::Current().Load();
				fFilter->SetEnabled(Settings::Current().EnableShortcut());
				break;
			default:
				BLooper::MessageReceived(message);
				break;
		}
	}
private:
	BSCInputFilter* fFilter;
};


BSCInputFilter::BSCInputFilter()
	:
	BInputServerFilter(),
	fLooper(nullptr),
	fEnabled(true)
{
	fLooper = new InputFilterLooper(this);
	fMessenger = BMessenger(fLooper);
	
	Settings::Initialize();
	fEnabled = Settings::Current().EnableShortcut();
	
	fLooper->Run();
	
	// Watch settings
	BPath settingsFile = Settings::SettingsFilePath();
	BEntry entry(settingsFile.Path());
	entry.GetNodeRef(&fNodeRef);
	watch_node(&fNodeRef, B_WATCH_ALL, fLooper);
}


BSCInputFilter::~BSCInputFilter()
{
	if (fLooper->Lock())
		fLooper->Quit();
}


status_t
BSCInputFilter::InitCheck()
{
	return B_NO_ERROR;
}


filter_result
BSCInputFilter::Filter(BMessage* message, BList* outList)
{
	if (!fEnabled)
		return B_DISPATCH_MESSAGE;

	switch (message->what) {
		case B_KEY_DOWN:
		case B_KEY_UP:
		case B_UNMAPPED_KEY_DOWN:
		case B_UNMAPPED_KEY_UP:
		{
			int32 key = message->GetInt32("raw_char", -1);
			int32 modifiers = message->GetInt32("modifiers", -1);
			if ((modifiers & B_CONTROL_KEY)
					&& (modifiers & B_COMMAND_KEY)
					&& (modifiers & B_SHIFT_KEY)) {
				if (key == 'r') {
					int32 repeat;
					if (message->FindInt32("be:key_repeat", &repeat) == B_OK) {
						// Ignore repeat keypresses
						return B_SKIP_MESSAGE;
					}
					if (message->what == B_KEY_UP
						|| message->what == B_UNMAPPED_KEY_UP) {
						// Only toggle record on keydown
						return B_SKIP_MESSAGE;
					}

					BMessage msg(kMsgGUIToggleCapture);
					if (be_roster->IsRunning(kAppSignature)) {
						BMessenger messenger(kAppSignature);
						messenger.SendMessage(&msg);
					} else
						be_roster->Launch(kAppSignature, &msg);

					return B_SKIP_MESSAGE;
				}
			}
			break;
		}
	}

	return B_DISPATCH_MESSAGE;
}


void
BSCInputFilter::SetEnabled(bool enable)
{
	fEnabled = enable;
}


extern "C"
BInputServerFilter*
instantiate_input_filter()
{
	return new BSCInputFilter();
}
