#ifndef QUICKJSDEBUGGER_H
#define QUICKJSDEBUGGER_H

#include "core/object/ref_counted.h"
#include "core/io/stream_peer_tcp.h"
#include "core/io/tcp_server.h"
#include "core/io/ip_address.h"
#include "core/object/ref_counted.h"
#include "quickjs/quickjs-debugger.h"
#define QJS_DEBUGGER_MAX_BUFFER_SIZE 4194304

class QuickJSDebugger : public RefCounted {
	GDCLASS(QuickJSDebugger, RefCounted)

	Ref<StreamPeerTCP> peer;
	Ref<TCPServer> server;
	JSRuntime *runtime;
	JSContext *ctx;
	uint8_t request_buffer[QJS_DEBUGGER_MAX_BUFFER_SIZE];

	struct ConnectionConfig {
		IPAddress address;
		uint16_t port;
	};

	ConnectionConfig parse_address(const String &address);

	static size_t transport_read(void *udata, char *buffer, size_t length);
	static size_t transport_write(void *udata, const char *buffer, size_t length);
	static size_t transport_peek(void *udata);
	static void transport_close(JSRuntime *rt, void *udata);

	Error attach_js_debugger(JSContext *p_ctx, Ref<StreamPeerTCP> p_peer);

public:
	Error connect_debugger(JSContext *ctx, const String &address);
	Error listen(JSContext *ctx, const String &address);
	void poll();

	QuickJSDebugger();
	~QuickJSDebugger();
};

#endif // QUICKJSDEBUGGER_H
