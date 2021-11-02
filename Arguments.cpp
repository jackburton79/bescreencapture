/*
 * Copyright 2005, Ingo Weinhold, bonefish@users.sf.net.
 * Distributed under the terms of the MIT License.
 */


#include "Arguments.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*#include <Catalog.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "Terminal arguments parsing"*/

#define B_TRANSLATE(x) x

Arguments::Arguments(int defaultArgcNum, const char* const* defaultArgv)
	: fUsageRequested(false),
	  fShellArgumentCount(0),
	  fShellArguments(NULL),
	  fRecordNow(false),
	  fFullScreen(false)
{
	_SetShellArguments(defaultArgcNum, defaultArgv);
}


Arguments::~Arguments()
{
	_SetShellArguments(0, NULL);
}


void
Arguments::Parse(int argc, const char* const* argv)
{
	int argi;
	for (argi = 1; argi < argc; argi ++) {
		const char* arg = argv[argi];

		if (*arg == '-') {
			if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
				fUsageRequested = true;
			else if (strcmp(arg, "-r") == 0 || strcmp(arg, "--recordnow") == 0) {
				fRecordNow = true;

			} else if (strcmp(arg, "-f") == 0 || strcmp(arg, "--fullscreen")
					== 0)
				fFullScreen = true;
			else {
				// illegal option
				fprintf(stderr, B_TRANSLATE("Unrecognized option \"%s\"\n"),
					arg);
				fUsageRequested = true;
			}
		} else {
			// no option, so the remainder is the shell program with arguments
			_SetShellArguments(argc - argi, argv + argi);
			argi = argc;
		}
	}
}


void
Arguments::GetShellArguments(int& argc, const char* const*& argv) const
{
	argc = fShellArgumentCount;
	argv = fShellArguments;
}


void
Arguments::_SetShellArguments(int argc, const char* const* argv)
{
	// delete old arguments
	for (int32 i = 0; i < fShellArgumentCount; i++)
		free((void *)fShellArguments[i]);
	delete[] fShellArguments;

	fShellArguments = NULL;
	fShellArgumentCount = 0;

	// copy new ones
	if (argc > 0 && argv) {
		fShellArguments = new const char*[argc + 1];
		for (int i = 0; i < argc; i++)
			fShellArguments[i] = strdup(argv[i]);

		fShellArguments[argc] = NULL;
		fShellArgumentCount = argc;
	}
}
