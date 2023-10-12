/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
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
