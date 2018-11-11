#ifndef __PRIORITYCONTROL_H
#define __PRIORITYCONTROL_H

#include <OptionPopUp.h>

const static uint32 kPriorityChanged = 'PrCh';

class PriorityControl : public BOptionPopUp {
public:
	PriorityControl(const char *name, const char *label,
		uint32 = B_WILL_DRAW);
};


#endif // PRIORITYCONTROL_H
