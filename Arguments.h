/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */

#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <Rect.h>

class Arguments {
public:
	Arguments(int defaultArgcNum, const char* const* defaultArgv);
	~Arguments();

	void Parse(int argc, const char* const* argv);

	bool RecordNow() const { return fRecordNow; }
	bool FullScreen() const { return fFullScreen; }
	bool UsageRequested() const	{ return fUsageRequested; }
	void GetShellArguments(int& argc, const char* const*& argv) const;

private:
	void _SetShellArguments(int argc, const char* const* argv);

	bool			fUsageRequested;
	int				fShellArgumentCount;
	const char**	fShellArguments;
	bool			fRecordNow;
	bool			fFullScreen;
};


#endif	// ARGUMENTS_H
