#include <Alert.h>
#include <InputServerFilter.h>
#include <List.h>
#include <Message.h>
#include <Roster.h>
#include <syslog.h>

#include "../BSCApp.h"

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
			if ((modifiers & B_CONTROL_KEY) && (modifiers & B_COMMAND_KEY)
				&& (modifiers & B_SHIFT_KEY)) {
				if (key == 'r') {
						// That's for ignoring repeat keypresses
					int32 rep;
					if (message->FindInt32("be:key_repeat", &rep) == B_OK) {
						return B_SKIP_MESSAGE;
					}
						// That's for only toggling record on keydown
					if(message->what == B_KEY_UP
						|| message->what == B_UNMAPPED_KEY_UP)
						return B_SKIP_MESSAGE;
						// And that's for starting the app or sending the message
					BMessage msg(kCmdToggleRecording);
					if (!be_roster->IsRunning(kAppSignature)) {
						be_roster->Launch(kAppSignature, &msg);
					} else {
						BMessenger messenger(kAppSignature);
						messenger.SendMessage(&msg);
					}
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
