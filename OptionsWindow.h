/*
 * Copyright 2017 Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef _OPTIONSWINDOW_H
#define _OPTIONSWINDOW_H


#include <Window.h>

class Controller;
class OptionsWindow : public BWindow {
public:
	OptionsWindow(Controller* controller);
};


#endif // _OPTIONSWINDOW_H
