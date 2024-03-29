/*
 * Copyright 2013-2023, Stefano Ceccherini <stefano.ceccherini@gmail.com>
 * All rights reserved. Distributed under the terms of the MIT license.
 */
#ifndef __ADVANCEDOPTIONSVIEW_H
#define __ADVANCEDOPTIONSVIEW_H

#include <View.h>

class BCheckBox;
class SizeControl;
class AdvancedOptionsView : public BView {
public:
	AdvancedOptionsView();

	virtual void AttachedToWindow();
	virtual void MessageReceived(BMessage *message);

private:
	BCheckBox* fUseDirectWindow;
	BCheckBox *fMinimizeOnStart;
	BCheckBox* fHideDeskbarIcon;
	BCheckBox* fUseShortcut;
	BCheckBox* fSelectOnStart;
	BCheckBox* fQuitWhenFinished;
	bool fCurrentMinimizeValue;

	void _EnableDirectWindowIfSupported();
};

#endif // __ADVANCEDOPTIONSVIEW_H
