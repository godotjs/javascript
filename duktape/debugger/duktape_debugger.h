#ifndef DUKTAPE_DEBUGGER_H
#define DUKTAPE_DEBUGGER_H

#include "../src/duktape.h"
#include "core/os/thread.h"

class DuktapeDebugger {
	duk_context *ctx;
	Thread *thread;

	bool debug_enabled;

	bool exited;
	bool exit;
	bool reconnect;

	void start();
	void stop();
	void _thread();
	void reset_connection();

	static DuktapeDebugger* singleton;

public:
	void initialize(duk_context *ctx);
	void uninitialize();

	static void _thread_func(void *ud);
	static duk_idx_t debugger_request(duk_context *ctx, void *udata, duk_idx_t nvalues);
	static void debugger_detached(duk_context *ctx, void *udata);

	static DuktapeDebugger *get_singleton();
	DuktapeDebugger();
};

#endif
