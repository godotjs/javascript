#include "quickjs_debugger.h"

#define QJS_DEBUGGER_DEBUG_LOG 0

QuickJSDebugger::ConnectionConfig QuickJSDebugger::parse_address(const String &address) {
	int idx = address.find_last(":");
	String address_str = address.substr(0, idx);
	if (address_str == "localhost") {
		address_str = "127.0.0.1";
	}
	ConnectionConfig connect;
	connect.port = address.substr(idx + 1).to_int();
	connect.address = IP_Address(address_str);
	return connect;
}

size_t QuickJSDebugger::transport_read(void *udata, char *buffer, size_t length) {
	QuickJSDebugger *debugger = static_cast<QuickJSDebugger *>(udata);
	ERR_FAIL_COND_V(debugger == NULL || debugger->peer.is_null() || !debugger->peer->is_connected_to_host(), -1);
	ERR_FAIL_COND_V(length == 0, -2);
	ERR_FAIL_NULL_V(buffer, -3);
	int ret = length;
	Error err = debugger->peer->get_data((uint8_t *)buffer, length);
#if QJS_DEBUGGER_DEBUG_LOG
	print_line(vformat("[read] %d %s", length, buffer));
#endif
	ERR_FAIL_COND_V(err != OK, -err);
	ERR_FAIL_COND_V(ret < 0, -4);
	ERR_FAIL_COND_V(ret > (ssize_t)length, -6);
	return ret;
}

size_t QuickJSDebugger::transport_write(void *udata, const char *buffer, size_t length) {
	QuickJSDebugger *debugger = static_cast<QuickJSDebugger *>(udata);
	ERR_FAIL_COND_V(debugger == NULL || debugger->peer.is_null() || !debugger->peer->is_connected_to_host(), -1);
	ERR_FAIL_COND_V(length == 0, -2);
	ERR_FAIL_NULL_V(buffer, -3);
	int ret = length;
	Error err = debugger->peer->put_data((uint8_t *)buffer, length);
#if QJS_DEBUGGER_DEBUG_LOG
	print_line(vformat("[write] %d %s", length, buffer));
#endif
	ERR_FAIL_COND_V(err != OK, -err);
	ERR_FAIL_COND_V(ret <= 0 || ret > (ssize_t)length, -4);
	return ret;
}

size_t QuickJSDebugger::transport_peek(void *udata) {
	QuickJSDebugger *debugger = static_cast<QuickJSDebugger *>(udata);
	ERR_FAIL_COND_V(debugger == NULL || debugger->peer.is_null() || !debugger->peer->is_connected_to_host(), -1);
	StreamPeerTCP::Status status = debugger->peer->get_status();
#if QJS_DEBUGGER_DEBUG_LOG
	print_line(vformat("[peek] %d", debugger->peer->get_available_bytes()));
#endif
	if (status != StreamPeerTCP::STATUS_CONNECTED && status != StreamPeerTCP::STATUS_CONNECTING) {
		return -2;
	}
	return debugger->peer->get_available_bytes();
}

void QuickJSDebugger::transport_close(JSRuntime *rt, void *udata) {
	QuickJSDebugger *debugger = static_cast<QuickJSDebugger *>(udata);
	ERR_FAIL_COND(debugger == NULL || debugger->peer.is_null());
	debugger->peer->disconnect_from_host();
	debugger->peer = Ref<StreamPeerTCP>();
#if QJS_DEBUGGER_DEBUG_LOG
	print_line(vformat("[transport_close]"));
#endif
}

Error QuickJSDebugger::attach_js_debugger(JSContext *ctx, Ref<StreamPeerTCP> p_peer) {
	ERR_FAIL_NULL_V(ctx, ERR_CANT_CONNECT);
	if (p_peer.is_valid() && p_peer->get_status() == StreamPeerTCP::STATUS_CONNECTED) {
		this->peer = p_peer;
		js_debugger_attach(ctx, transport_read, transport_write, transport_peek, transport_close, this);
		return OK;
	}
	return ERR_CANT_CONNECT;
}

Error QuickJSDebugger::connect(JSContext *ctx, const String &address) {
	this->ctx = ctx;
	this->runtime = JS_GetRuntime(ctx);

	ConnectionConfig c = parse_address(address);
	Ref<StreamPeerTCP> peer = memnew(StreamPeerTCP);
	Error err = peer->connect_to_host(c.address, c.port);
	if (OK == err) {
		while (peer->get_status() == StreamPeerTCP::STATUS_CONNECTING)
			; // wait until connected to the debugger
		return attach_js_debugger(ctx, peer);
	}
	return err;
}

Error QuickJSDebugger::listen(JSContext *ctx, const String &address) {
	this->ctx = ctx;
	this->runtime = JS_GetRuntime(ctx);
	if (server.is_valid() && server->is_listening()) {
		server->stop();
	}
	server.instance();
	ConnectionConfig c = parse_address(address);
	Error err = server->listen(c.port, c.address);
	return err;
}

void QuickJSDebugger::poll() {
	if (server.is_valid()) {
		if (server->is_connection_available()) {
			attach_js_debugger(ctx, server->take_connection());
		}
	}
	if (peer.is_valid() && server.is_valid() && server->is_listening()) {
		if (peer->get_status() == StreamPeerTCP::STATUS_NONE || peer->get_status() == StreamPeerTCP::STATUS_ERROR) {
			JSDebuggerInfo *info = js_debugger_info(runtime);
			js_debugger_free(runtime, info);
		}
	}
}

QuickJSDebugger::QuickJSDebugger() {
	ctx = NULL;
	runtime = NULL;
}

QuickJSDebugger::~QuickJSDebugger() {
	if (server.is_valid() && server->is_listening()) {
		server->stop();
	}
}
