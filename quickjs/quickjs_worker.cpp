#include "quickjs_worker.h"
#include "core/os/os.h"

void QuickJSWorker::thread_main(void *p_this) {
	QuickJSWorker *self = static_cast<QuickJSWorker *>(p_this);

	self->initialize();
	self->running = true;

	Error err;
	String text = FileAccess::get_file_as_string(self->entry_script, &err);

	if (err == OK) {
		String err_text;
		ECMAScriptGCHandler eval_ret;
		err = self->safe_eval_text(text, ECMAScriptBinder::EVAL_TYPE_MODULE, self->entry_script, err_text, eval_ret);
		if (err == OK) {
			JSValue onmessage_callback = JS_GetPropertyStr(self->ctx, self->global_object, "onmessage");
			bool onmessage_valid = JS_IsFunction(self->ctx, onmessage_callback);
			while (self->running) {
				self->frame();

				if (onmessage_valid) {
					List<Variant> messages;
					{
						GLOBAL_LOCK_FUNCTION
						for (List<Variant>::Element *E = self->input_message_queue.front(); E; E = E->next()) {
							messages.push_back(E->get());
						}
						self->input_message_queue.clear();
					}
					for (List<Variant>::Element *E = messages.front(); E; E = E->next()) {
						JSValue argv[] = { variant_to_var(self->ctx, E->get()) };
						JSValue ret = JS_Call(self->ctx, onmessage_callback, self->global_object, 1, argv);
						if (JS_IsException(ret)) {
							JSValue e = JS_GetException(self->ctx);
							ECMAscriptScriptError err;
							dump_exception(self->ctx, e, &err);
							ERR_PRINT(String("Error in worker onmessage callback") + ENDL + self->error_to_string(err));
							JS_FreeValue(self->ctx, e);
						}
						JS_FreeValue(self->ctx, argv[0]);
					}
				}
				OS::get_singleton()->delay_usec(1000);
			}
			JS_FreeValue(self->ctx, onmessage_callback);
		} else {
			ERR_PRINT("Failed to eval entry script:" + self->entry_script + "\nError:" + err_text);
		}
	} else {
		ERR_PRINT("Failed to load entry script:" + self->entry_script);
	}

	self->uninitialize();
}

JSValue QuickJSWorker::global_worker_close(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	QuickJSWorker *worker = static_cast<QuickJSWorker *>(get_context_binder(ctx));
	if (worker) {
		worker->running = false;
	}
	return JS_UNDEFINED;
}

JSValue QuickJSWorker::global_worker_post_message(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	ERR_FAIL_COND_V(argc < 1, JS_ThrowTypeError(ctx, "message value expected of argument #0"));
	QuickJSWorker *worker = static_cast<QuickJSWorker *>(get_context_binder(ctx));
	if (worker) {
		GLOBAL_LOCK_FUNCTION
		worker->output_message_queue.push_back(var_to_variant(ctx, argv[0]));
	}
	return JS_UNDEFINED;
}

JSValue QuickJSWorker::global_import_scripts(JSContext *ctx, JSValue this_val, int argc, JSValue *argv) {
	for (int i = 0; i < argc; i++) {
		if (JS_IsString(argv[i])) {
			Error err;
			String path = resolve_module_file(js_to_string(ctx, argv[0]));
			String source = FileAccess::get_file_as_string(path, &err);
			QuickJSBinder *bind = get_context_binder(ctx);
			ECMAScriptGCHandler eval_ret;
			bind->eval_string(source, ECMAScriptBinder::EVAL_TYPE_GLOBAL, path, eval_ret);
		}
	}
	return JS_UNDEFINED;
}

QuickJSWorker::QuickJSWorker(const QuickJSBinder *p_host_context) :
		QuickJSBinder() {
	running = false;
	host_context = p_host_context;
}

QuickJSWorker::~QuickJSWorker() {
	stop();
}

void QuickJSWorker::initialize() {
	QuickJSBinder::initialize();
	// onmessage
	JS_SetPropertyStr(ctx, global_object, "onmessage", JS_NULL);
	// close
	JSValue close_func = JS_NewCFunction(ctx, global_worker_close, "close", 0);
	JS_DefinePropertyValueStr(ctx, global_object, "close", close_func, PROP_DEF_DEFAULT);
	JSValue post_message_func = JS_NewCFunction(ctx, global_worker_post_message, "postMessage", 1);
	// INSIDE_WORKER
	JS_DefinePropertyValueStr(ctx, global_object, "postMessage", post_message_func, PROP_DEF_DEFAULT);
	JS_DefinePropertyValueStr(ctx, global_object, "INSIDE_WORKER", JS_TRUE, JS_PROP_ENUMERABLE);
	// importScripts
	JSValue import_scripts_func = JS_NewCFunction(ctx, global_import_scripts, "importScripts", 10);
	JS_DefinePropertyValueStr(ctx, global_object, "importScripts", import_scripts_func, JS_PROP_ENUMERABLE);
}

void QuickJSWorker::uninitialize() {
	QuickJSBinder::uninitialize();
}

bool QuickJSWorker::frame_of_host(QuickJSBinder *host, const JSValueConst &value) {

	JSValue onmessage_callback = JS_GetPropertyStr(host->ctx, value, "onmessage");
	if (JS_IsFunction(host->ctx, onmessage_callback)) {

		List<Variant> messages;
		{
			GLOBAL_LOCK_FUNCTION
			for (List<Variant>::Element *E = output_message_queue.front(); E; E = E->next()) {
				messages.push_back(E->get());
			}
			output_message_queue.clear();
		}

		for (List<Variant>::Element *E = messages.front(); E; E = E->next()) {
			JSValue argv[] = { variant_to_var(host->ctx, E->get()) };
			JSValue ret = JS_Call(host->ctx, onmessage_callback, JS_NULL, 1, argv);
			if (JS_IsException(ret)) {
				JSValue e = JS_GetException(host->ctx);
				ECMAscriptScriptError err;
				dump_exception(host->ctx, e, &err);
				ERR_PRINT(String("Error in worker onmessage callback") + ENDL + error_to_string(err));
				JS_FreeValue(host->ctx, e);
			}
			JS_FreeValue(host->ctx, argv[0]);
		}
	}

	JS_FreeValue(host->ctx, onmessage_callback);
	return running;
}

void QuickJSWorker::post_message_from_host(const Variant &p_message) {
	GLOBAL_LOCK_FUNCTION
	input_message_queue.push_back(p_message);
}

void QuickJSWorker::start(const String &p_path) {
	ERR_FAIL_COND(running || thread.is_started());
	entry_script = p_path;
	thread.start(thread_main, this);
}

void QuickJSWorker::stop() {
	if (thread.is_started()) {
		running = false;
		thread.wait_to_finish();
	}
}
