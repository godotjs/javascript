#ifndef QUICKJS_BUILTIN_BINDER_H
#define QUICKJS_BUILTIN_BINDER_H

#include "quickjs/quickjs.h"
#include <core/variant.h>
class ECMAScriptGCHandler;
class QuickJSBinder;

typedef JSValue (*JSConstructorFunc)(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv);
typedef void (*JSFinalizerFunc)(JSRuntime *rt, JSValue val);

class QuickJSBuiltinBinder {
public:
	struct BuiltinClass {
		JSClassID id;
		JSValue class_function;
		JSValue class_prototype;
		JSClassDef js_class;
	};

private:
	QuickJSBinder *binder;
	JSContext *ctx;
	BuiltinClass *builtin_class_map;
	JSValue to_string_function;
	JSAtom js_key_to_string;

public:
	static void builtin_finalizer(ECMAScriptGCHandler *p_bind);
	static JSValue bind_builtin_object(JSContext *ctx, Variant::Type p_type, void *p_object);

	void register_builtin_class(Variant::Type p_type, const char *p_name, JSConstructorFunc p_constructor, int argc);
	void register_property(Variant::Type p_type, const char *p_name, JSCFunctionMagic *p_getter, JSCFunctionMagic *p_setter, int magic);
	void register_operator(Variant::Type p_type, JSAtom p_operator, JSCFunctionMagic *p_func, int p_length);

public:
	QuickJSBuiltinBinder();
	~QuickJSBuiltinBinder();

	void initialize(JSContext *p_context, QuickJSBinder *p_binder);
	void uninitialize();

	_FORCE_INLINE_ void set_classid(Variant::Type p_type, JSClassID p_id) { builtin_class_map[p_type].id = p_id; }
	_FORCE_INLINE_ JSClassID get_classid(Variant::Type p_type) { return builtin_class_map[p_type].id; }
	_FORCE_INLINE_ BuiltinClass &get_class(Variant::Type p_type) { return *(builtin_class_map + p_type); }

	static JSValue new_object_from(JSContext *ctx, const Variant &p_val);
};

#endif // QUICKJS_BUILTIN_BINDER_H
