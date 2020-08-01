#ifndef QUICKJSDEBUGGER_H
#define QUICKJSDEBUGGER_H

#include "core/io/stream_peer_tcp.h"
#include "core/io/tcp_server.h"
#include "core/reference.h"
#include "quickjs/quickjs-debugger.h"
#define QJS_DEBUGGER_MAX_BUFFER_SIZE 4194304

class QuickJSDebugger : public Reference {
	GDCLASS(QuickJSDebugger, Reference)

	Ref<StreamPeerTCP> peer;
	Ref<TCP_Server> server;
	JSContext *ctx;
	uint8_t request_buffer[QJS_DEBUGGER_MAX_BUFFER_SIZE];

	struct ConnectionConfig {
		IP_Address address;
		uint16_t port;
	};

	ConnectionConfig parse_address(const String &address);

	static size_t transport_read(void *udata, char *buffer, size_t length);
	static size_t transport_write(void *udata, const char *buffer, size_t length);
	static size_t transport_peek(void *udata);
	static void transport_close(JSContext *ctx, void *udata);

	Error attach_js_debugger(JSContext *p_ctx, Ref<StreamPeerTCP> p_peer);

public:
	Error connect(JSContext *ctx, const String &address);
	Error listen(JSContext *ctx, const String &address);
	void poll();

	QuickJSDebugger();
	~QuickJSDebugger();
};

#endif // QUICKJSDEBUGGER_H
