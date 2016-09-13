#include "Executor.h"


Executor::Executor(FunctionObjectWithResult<status_t> *function)
	:
	fFunction(function),
	fThread(-1)
{
}


Executor::~Executor()
{
}

	
void
Executor::Run()
{
	(*fFunction)();
}


thread_id
Executor::RunThreaded()
{
	fThread = spawn_thread((thread_entry)_executing_starter,
		"Capture Thread", B_LOW_PRIORITY, this);
	resume_thread(fThread);
	
	return fThread;
}


int32
Executor::_ExecutingThread()
{
	(*fFunction)();
	
	fThread = -1;
	
	delete this;
	return 0;
}


/* static */
int32
Executor::_executing_starter(void *arg)
{
	return static_cast<Executor *>(arg)->_ExecutingThread();
}
