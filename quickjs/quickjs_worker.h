#ifndef QUICKJS_WORKER_H
#define QUICKJS_WORKER_H

#include "quickjs_binder.h"
#include "core/os/thread.h"

class QuickJSWorker : public QuickJSBinder {
	Thread *thread;
	bool running = false;
	static void thread_main(void* p_self);

	String entry_script;
public:
	QuickJSWorker();
	virtual ~QuickJSWorker();

	virtual void initialize();
	virtual void uninitialize();

	void start(const String &p_path);
	void stop();

};

#endif // QUICKJS_WORKER_H
