#include <Alert.h>
#include <InputServerFilter.h>
#include <List.h>
#include <Message.h>
#include <Roster.h>
#include <syslog.h>

#include "BSCApp.h"

class ThinInputFilter : public BInputServerFilter {
	status_t		InitCheck();
	filter_result	Filter(BMessage* message, BList* outList);
};


status_t
ThinInputFilter::InitCheck()
{
	return B_NO_ERROR;
}


filter_result
ThinInputFilter::Filter(BMessage* message, BList* outList)
{
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

					BMessage msg(kCmdToggleRecording);
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


extern "C"
BInputServerFilter*
instantiate_input_filter()
{
	return new ThinInputFilter();
}
