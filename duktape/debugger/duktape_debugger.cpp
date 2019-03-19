#include "duktape_debugger.h"
#include "duk_trans_socket.h"
#include "core/print_string.h"
#include "core/os/os.h"
#include "core/project_settings.h"

DuktapeDebugger* DuktapeDebugger::singleton = NULL;


duk_idx_t DuktapeDebugger::debugger_request(duk_context *ctx, void *udata, duk_idx_t nvalues) {
	if (nvalues < 1) {
		duk_push_string(ctx, "missing AppRequest argument(s)");
		return -1;
	}
	const char *cmd = duk_get_string(ctx, -nvalues + 0);
	duk_push_sprintf(ctx, "command not supported: %s", cmd);
	return -1;
}

void DuktapeDebugger::debugger_detached(duk_context *ctx, void *udata) {
	/* Ensure socket is closed even when detach is initiated by Duktape
	 * rather than debug client.
	 */
	duk_trans_socket_finish();
	if (!DuktapeDebugger::singleton->exit) {
		DuktapeDebugger::singleton->reconnect = true;
	}
}


void DuktapeDebugger::_thread_func(void *ud) {
	DuktapeDebugger * self = static_cast<DuktapeDebugger*>(ud);
	self->_thread();
}

void DuktapeDebugger::start() {
	thread = Thread::create(_thread_func, this);
	exited = false;
	exit = false;
	reconnect = true;
}

void DuktapeDebugger::stop() {
	if (thread) {
		exit = true;
		while (!exited) {
			OS::get_singleton()->delay_usec(10000);
		}
		Thread::wait_to_finish(thread);
		memdelete(thread);
		thread = NULL;
	}
}

void DuktapeDebugger::_thread() {
	while (!exit) {
		if (reconnect) {
			reconnect = false;
			reset_connection();
		}
		OS::get_singleton()->delay_usec(100);
	}
	exited = true;
}

void DuktapeDebugger::reset_connection() {
	uint32_t port = GLOBAL_DEF("ecmascript/debugger_port", 9091);
	duk_trans_socket_init(port);
	duk_trans_socket_waitconn();
	duk_debugger_attach(ctx,
						duk_trans_socket_read_cb,
						duk_trans_socket_write_cb,
						duk_trans_socket_peek_cb,
						duk_trans_socket_read_flush_cb,
						duk_trans_socket_write_flush_cb,
						DuktapeDebugger::debugger_request,
						DuktapeDebugger::debugger_detached,
						NULL);
}

void DuktapeDebugger::initialize(duk_context *ctx) {
	this->ctx = ctx;
	GLOBAL_DEF("ecmascript/debugger_port", 9091);
	debug_enabled = GLOBAL_DEF("ecmascript/debugger_enabled", false);
	ProjectSettings::get_singleton()->set_custom_property_info("ecmascript/debugger_port", PropertyInfo(Variant::INT, "ecmascript/debugger_port", PROPERTY_HINT_RANGE, "100,65535,1"));
	ProjectSettings::get_singleton()->set_custom_property_info("ecmascript/debugger_enabled", PropertyInfo(Variant::BOOL, "ecmascript/debugger_enabled"));

#if 0
	// FIXME: GODOT BUG
	//'-d' or '--debug' is is not included in OS::get_singleton()->get_cmdline_args()
	List<String> args = OS::get_singleton()->get_cmdline_args();
	for (List<String>::Element *E = args.front(); E; E=E->next()) {
		if(E->get() == "--debugger") {
			debug_enabled = true;
			break;
		}
	}
#endif
	if (debug_enabled) {
		start();
	}

}

void DuktapeDebugger::uninitialize() {
	if (debug_enabled) {
		stop();
	}
}

DuktapeDebugger *DuktapeDebugger::get_singleton() {
	return singleton;
}

DuktapeDebugger::DuktapeDebugger() {
	singleton = this;
}

