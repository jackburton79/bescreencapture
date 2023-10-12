/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#include "PriorityControl.h"

#define MAKE_STRING(x) #x

PriorityControl::PriorityControl(const char *name, const char *label,
		uint32 flags)
		:
		BOptionPopUp(name, label, new BMessage(kPriorityChanged), flags)
{
	AddOption(MAKE_STRING(LOW), B_LOW_PRIORITY);
	AddOption(MAKE_STRING(NORMAL), B_NORMAL_PRIORITY);
	AddOption(MAKE_STRING(HIGH), B_NORMAL_PRIORITY + 2);
}
