#ifndef QUICKJS_WORKER_H
#define QUICKJS_WORKER_H

#include "core/os/thread.h"
#include "quickjs_binder.h"

class QuickJSWorker : public QuickJSBinder {
	Thread thread;
	bool running = false;
	static void thread_main(void *p_self);
	String entry_script;

	const QuickJSBinder *host_context;
	List<Variant> input_message_queue;
	List<Variant> output_message_queue;

	static JSValue global_worker_close(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static JSValue global_worker_post_message(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);
	static JSValue global_import_scripts(JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv);

public:
	QuickJSWorker(const QuickJSBinder *p_host_context);
	virtual ~QuickJSWorker();

	virtual void initialize();
	virtual void uninitialize();

	bool frame_of_host(QuickJSBinder *host, const JSValueConst &value);
	void post_message_from_host(const Variant &p_message);
	void start(const String &p_path);
	void stop();
};

#endif // QUICKJS_WORKER_H
