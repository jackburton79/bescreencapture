#ifndef __EXECUTOR_H
#define __EXECUTOR_H

#include <OS.h>

#include "FunctionObject.h"

class Executor {
public:
	Executor(FunctionObjectWithResult<status_t> *function);
	virtual	~Executor();

	virtual void Run();
	virtual thread_id RunThreaded();
private:
	FunctionObjectWithResult<status_t> *fFunction;
	thread_id fThread;
	
	int32 _ExecutingThread();
	static int32 _executing_starter(void *arg);
};


#endif
