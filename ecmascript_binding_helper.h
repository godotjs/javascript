#ifndef ECMASCRIPT_BINDING_HELPER_H
#define ECMASCRIPT_BINDING_HELPER_H

#include "core/object.h"
#include "core/reference.h"

struct ECMAScriptGCHandler {
	void *ecma_object;
};

struct ECMAScriptBindingData : public ECMAScriptGCHandler {
	Object *godot_object;
};

struct ECMAClassInfo {
	ECMAScriptGCHandler ecma_constructor;
	StringName class_name;
	String icon_path;
};

class ECMAScriptBindingHelper {

protected:
	HashMap<StringName, ECMAClassInfo> ecma_classes;

public:
	virtual void initialize() = 0;
	virtual void uninitialize() = 0;

	virtual void *alloc_object_binding_data(Object *p_object) = 0;
	virtual void free_object_binding_data(void *p_gc_handle) = 0;

	virtual void godot_refcount_incremented(Reference *p_object) = 0;
	virtual bool godot_refcount_decremented(Reference *p_object) = 0;

	virtual Error eval_string(const String &p_source) = 0;

	virtual Error create_ecma_object_for_godot_object(const ECMAScriptGCHandler &p_prototype, Object *p_object, ECMAScriptGCHandler &r_handler) = 0;
};

#endif
