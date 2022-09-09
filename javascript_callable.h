#ifndef JAVASCRIPT_CALLABLE_H
#define JAVASCRIPT_CALLABLE_H

#include "core/variant/callable.h"
#include "javascript_gc_handler.h"

class JavaScriptCallable : public CallableCustom {
protected:
	JavaScriptGCHandler js_function;

public:
	JavaScriptCallable() {}
	JavaScriptCallable(const JavaScriptGCHandler &p_function) : js_function(p_function) {}
	virtual ~JavaScriptCallable() {}
};

#endif // JAVASCRIPT_CALLABLE_H
