#include "duktape_binding_helper.h"
#include "ecmascript_language.h"

void DuktapeBindingHelper::fatal_function(void *udata, const char *msg) {
	fprintf(stderr, "*** FATAL ERROR: %s\n", (msg ? msg : "no message"));
	fflush(stderr);
#ifdef DEBUG_ENABLED
	CRASH_NOW();
#elif
	abort();
#endif
}

void DuktapeBindingHelper::set_weak_ref(Object *obj, DuktapeHeapObject *ptr) {
	ERR_FAIL_NULL(obj);
	weakref_pool.set(obj->get_instance_id(), ptr);
}

DuktapeHeapObject *DuktapeBindingHelper::get_weak_ref(Object *obj) {
	ERR_FAIL_NULL_V(obj, NULL);
	void **ele_ptr = get_singleton()->weakref_pool.getptr(obj->get_instance_id());
	if (ele_ptr) {
		return *ele_ptr;
	}
	return NULL;
}

void DuktapeBindingHelper::set_strong_ref(Object *obj, DuktapeHeapObject *ptr) {

	ERR_FAIL_NULL(obj);

	strongref_pool.set(obj->get_instance_id(), ptr);

	// fill the script instance binding data to make sure godot_refcount_decremented can get when unreference
	obj->get_script_instance_binding(get_language()->get_language_index());

	duk_push_strong_ref_container(ctx);
	duk_push_heapptr(ctx, ptr);
	duk_put_prop_index(ctx, -2, obj->get_instance_id());
	duk_pop(ctx);
}

DuktapeHeapObject *DuktapeBindingHelper::get_strong_ref(Object *obj) {
	DuktapeHeapObject **val_ptr = strongref_pool.getptr(obj->get_instance_id());
	if (val_ptr) {
		return *val_ptr;
	}
	return NULL;
}

void DuktapeBindingHelper::duk_push_strong_ref_container(duk_context *ctx) {
	duk_push_heapptr(ctx, this->strongref_pool_ptr);
}

duk_ret_t DuktapeBindingHelper::duk_godot_object_constructor(duk_context *ctx) {
	if (!duk_is_constructor_call(ctx)) {
		return DUK_ERR_TYPE_ERROR;
	}

	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("cls"));
	ClassDB::ClassInfo *cls = static_cast<ClassDB::ClassInfo *>(duk_get_pointer_default(ctx, -1, NULL));
	ERR_FAIL_NULL_V(cls, DUK_ERR_TYPE_ERROR);
	ERR_FAIL_NULL_V(cls->creation_func, DUK_ERR_TYPE_ERROR);

	duk_push_godot_object(ctx, cls->creation_func());
	return NO_RET_VAL;
}

duk_ret_t DuktapeBindingHelper::duk_godot_object_finalizer(duk_context *ctx) {
	Object *ptr = duk_get_godot_object(ctx, -1);
	if (NULL == ptr) return NO_RET_VAL;

	if (Reference *ref = Object::cast_to<Reference>(ptr)) {
		if (ref->unreference()) {
			// A reference without C++ usage
			get_singleton()->weakref_pool.erase(ptr->get_instance_id());
			memdelete(ref);
		} else if (ref->get_script_instance_binding(get_language()->get_language_index())) {
			// A reference not used in C++
			memdelete(ref);
		} else {
			// A reference with other C++ reference
			// Rescue the ecmascript object as the reference is still alive
			get_singleton()->set_strong_ref(ref, duk_get_heapptr(ctx, 0));
		}
	}
	return NO_RET_VAL;
}

void DuktapeBindingHelper::godot_refcount_incremented(Reference *p_object) {
}

bool DuktapeBindingHelper::godot_refcount_decremented(Reference *p_object) {
	int refcount = p_object->reference_get_count();
	DuktapeGCHandler *gc_handler = static_cast<DuktapeGCHandler *>(p_object->get_script_instance_binding(get_language()->get_language_index()));
	if (gc_handler) {
		if (refcount == 0) {
			// clear taged strong reference in script
			// the life of this refernce is given to script gc
			set_strong_ref(p_object, NULL);
		}
		return false;
	}
	return refcount == 0;
}

DuktapeBindingHelper::DuktapeGCHandler *DuktapeBindingHelper::alloc_object_binding_data(Object *p_object) {
	DuktapeGCHandler *handler = NULL;
	if (DuktapeHeapObject *heap_ptr = get_strong_ref(p_object)) {
		handler = memnew(DuktapeGCHandler);
		handler->godot_object = p_object;
		handler->duktape_heap_ptr = heap_ptr;
	}
	return handler;
}

void DuktapeBindingHelper::free_object_binding_data(DuktapeGCHandler *p_gc_handler) {
	if (p_gc_handler) {
		if (Object::cast_to<Reference>(p_gc_handler->godot_object)) {
			// References don't need do this as they are weak referenced or they
			return;
		} else if (p_gc_handler->duktape_heap_ptr) {
			// clear taged script reference
			set_strong_ref(p_gc_handler->godot_object, NULL);
		}
		memdelete(p_gc_handler);
	}
}

duk_ret_t DuktapeBindingHelper::godot_object_free(duk_context *ctx) {
	duk_push_this(ctx);
	Object *obj = duk_get_godot_object(ctx, -1);
	ERR_FAIL_NULL_V(obj, NO_RET_VAL);
	if (Object::cast_to<Reference>(obj)) {
		ERR_FAIL_V(DUK_ERR_TYPE_ERROR);
	} else {
		memdelete(obj);
		duk_del_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));
	}
	return NO_RET_VAL;
}

duk_ret_t DuktapeBindingHelper::godot_print_function(duk_context *ctx) {
	int size = duk_get_top(ctx);
	String msg;
	for (int i = 0; i < size; ++i) {
		msg += DuktapeBindingHelper::duk_get_godot_string(ctx, i, true);
		if (i < size - 1) {
			msg += " ";
		}
	}
	print_line(msg);
	return NO_RET_VAL;
}

duk_ret_t DuktapeBindingHelper::duk_godot_object_method(duk_context *ctx) {

	duk_idx_t argc = duk_get_top(ctx);

	duk_push_current_function(ctx);
	duk_get_prop_string(ctx, -1, DUK_HIDDEN_SYMBOL("mb"));
	MethodBind *mb = static_cast<MethodBind *>(duk_get_pointer_default(ctx, -1, NULL));
	ERR_FAIL_NULL_V(mb, NO_RET_VAL);

	duk_push_this(ctx);
	Object *ptr = duk_get_godot_object(ctx, -1);
	ERR_FAIL_NULL_V(ptr, NO_RET_VAL);

	Variant::CallError err;

	Vector<Variant> args;
	args.resize(argc);
	for (duk_idx_t i = 0; i < argc; ++i) {
		args.ptrw()->operator=(duk_get_godot_variant(ctx, i));
	}

	const Variant *args_ptr = args.ptr();
	const Variant &ret_val = mb->call(ptr, &args_ptr, argc, err);
	ERR_FAIL_COND_V(err.error != Variant::CallError::CALL_OK, DUK_ERR_TYPE_ERROR);

	if (mb->has_return()) {
		duk_push_godot_variant(ctx, ret_val);
		return HAS_RET_VAL;
	}
	return NO_RET_VAL;
}

void DuktapeBindingHelper::duk_push_godot_variant(duk_context *ctx, const Variant &var) {
	switch (var.get_type()) {
		case Variant::NIL:
			duk_push_null(ctx);
			break;
		case Variant::BOOL:
			duk_push_boolean(ctx, duk_bool_t((bool)var));
			break;
		case Variant::REAL:
		case Variant::INT:
			duk_push_number(ctx, var);
			break;
		case Variant::STRING: {
			duk_push_godot_string(ctx, var);
		} break;
		case Variant::OBJECT:
			duk_push_godot_object(ctx, var);
			break;
		default:
			// not supported
			duk_push_undefined(ctx);
			break;
	}
}

Variant DuktapeBindingHelper::duk_get_godot_variant(duk_context *ctx, duk_idx_t idx) {
	duk_int_t type = duk_get_type(ctx, idx);
	switch (type) {
		case DUK_TYPE_NUMBER:
			return Variant(duk_get_number(ctx, idx));
		case DUK_TYPE_STRING: {
			String str;
			str.parse_utf8(duk_get_string(ctx, idx));
			return str;
		} break;
		case DUK_TYPE_OBJECT: {
			Object *obj = duk_get_godot_object(ctx, idx);
			if (Reference *r = Object::cast_to<Reference>(obj)) {
				// Add reference count as the REF construct from Reference* don't increase the reference count
				// This reference count will minused in the desctuctor of REF
				REF ref(r);
				ref->reference();
				return ref;
			}
			return obj;
		}
		case DUK_TYPE_NULL:
		case DUK_TYPE_UNDEFINED:
		default:
			return Variant();
	}
}

String DuktapeBindingHelper::duk_get_godot_string(duk_context *ctx, duk_idx_t idx, bool convert_type) {
	const char *raw_str = convert_type ? duk_to_string(ctx, idx) : duk_get_string(ctx, idx);
	String str;
	str.parse_utf8(raw_str);
	return str;
}

Object *DuktapeBindingHelper::duk_get_godot_object(duk_context *ctx, duk_idx_t idx) {
	duk_get_prop_string(ctx, idx, DUK_HIDDEN_SYMBOL("ptr"));
	return static_cast<Object *>(duk_get_pointer_default(ctx, -1, NULL));
}

void DuktapeBindingHelper::duk_push_godot_object(duk_context *ctx, Object *obj) {
	if (obj) {
		DuktapeHeapObject *heap_obj = get_singleton()->get_weak_ref(obj);
		if (heap_obj) {
			duk_push_heapptr(ctx, heap_obj);
		} else {
			if (duk_is_constructor_call(ctx)) {
				duk_push_this(ctx);
			} else {
				duk_push_object(ctx);
			}

			duk_push_pointer(ctx, obj);
			duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("ptr"));

			duk_push_heapptr(ctx, get_singleton()->class_prototypes.get(obj->get_class_name()));
			duk_set_prototype(ctx, -2);

			heap_obj = duk_get_heapptr(ctx, -1);
			if (Reference *ref = Object::cast_to<Reference>(obj)) {
				get_singleton()->set_weak_ref(ref, heap_obj);
			} else {
				// The strong reference is released when object is going to die
				// See godot_free_instance_callback and godot_refcount_decremented
				get_singleton()->set_strong_ref(obj, heap_obj);
			}
		}
	} else {
		duk_push_null(ctx);
	}
}

void DuktapeBindingHelper::duk_push_godot_string(duk_context *ctx, const String &str) {
	duk_push_string(ctx, str.utf8().ptr());
}

void DuktapeBindingHelper::duk_push_godot_string_name(duk_context *ctx, const StringName &str) {
	duk_push_godot_string(ctx, str);
}

void DuktapeBindingHelper::duk_put_prop_godot_string(duk_context *ctx, duk_idx_t idx, const String &str) {
	duk_put_prop_string(ctx, idx, str.utf8().ptr());
}

void DuktapeBindingHelper::duk_put_prop_godot_string_name(duk_context *ctx, duk_idx_t idx, const StringName &str) {
	duk_put_prop_godot_string(ctx, idx, str);
}

void DuktapeBindingHelper::rigister_class(duk_context *ctx, const ClassDB::ClassInfo *cls) {
	duk_require_stack(ctx, 2);

	// Class constuctor function
	duk_push_c_function(ctx, duk_godot_object_constructor, 0);
	duk_push_pointer(ctx, (void *)cls);
	duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("cls"));
	{
		// Class.prototype
		duk_push_object(ctx);
		{
			// Class.prototype.fin
			duk_push_c_function(ctx, duk_godot_object_finalizer, 1);
			duk_set_finalizer(ctx, -2);
			// Object.free
			duk_push_c_function(ctx, godot_object_free, 0);
			duk_put_prop_literal(ctx, -2, "free");
		}
		{
			// members
			register_class_members(ctx, cls);
		}

		DuktapeHeapObject *proto_ptr = duk_get_heapptr(ctx, -1);
		get_singleton()->class_prototypes[cls->name] = proto_ptr;

		duk_set_prototype(ctx, -2);
	}
	duk_put_prop_godot_string_name(ctx, -2, cls->name);
}

DuktapeBindingHelper *DuktapeBindingHelper::get_singleton() {
	return ECMAScriptLanguage::get_singleton()->binding;
}

ECMAScriptLanguage *DuktapeBindingHelper::get_language() {
	return ECMAScriptLanguage::get_singleton();
}

void DuktapeBindingHelper::initialize() {

	this->ctx = duk_create_heap(alloc_function, realloc_function, free_function, this, fatal_function);
	ERR_FAIL_NULL(this->ctx);

	// strong reference object pool
	duk_push_heap_stash(this->ctx);
	duk_push_object(this->ctx);
	this->strongref_pool_ptr = duk_get_heapptr(ctx, -1);
	duk_put_prop_literal(ctx, -2, "object_pool");
	duk_pop(ctx);

	// global scope
	duk_push_c_function(this->ctx, godot_print_function, DUK_VARARGS);
	duk_put_global_string(this->ctx, "print");

	// godot namespace
	duk_push_object(this->ctx);
	{
		// TODO: register builtin classes

		// register classes
		const StringName *key = ClassDB::classes.next(NULL);
		while (key) {
			const ClassDB::ClassInfo *cls = ClassDB::classes.getptr(*key);
			rigister_class(this->ctx, cls);
			key = ClassDB::classes.next(key);
		}
	}
	duk_put_global_string(this->ctx, "godot");
}

void DuktapeBindingHelper::uninitialize() {
	duk_destroy_heap(this->ctx);
	this->ctx = NULL;
}

void DuktapeBindingHelper::register_class_members(duk_context *ctx, const ClassDB::ClassInfo *cls) {
	if (cls->inherits_ptr) {
		register_class_members(ctx, cls->inherits_ptr);
	}

	const duk_idx_t prototype_idx = duk_get_top(ctx) - 1;

	{
		// methods
		const StringName *key = cls->method_map.next(NULL);
		while (key) {

			duk_require_stack(ctx, 2);

			MethodBind *mb = cls->method_map.get(*key);
			duk_push_godot_method(ctx, mb);
			duk_put_prop_godot_string_name(ctx, -2, mb->get_name());

			key = cls->method_map.next(key);
		}
	}

	{
		// properties
		const StringName *key = cls->property_setget.next(NULL);
		while (key) {

			duk_require_stack(ctx, 3);

			const ClassDB::PropertySetGet &prop = cls->property_setget[*key];
			duk_uidx_t masks = DUK_DEFPROP_FORCE;
			duk_push_godot_string_name(ctx, *key);
			if (prop._getptr) {
				duk_push_godot_method(ctx, prop._getptr);
				masks |= DUK_DEFPROP_HAVE_GETTER;
			}
			if (prop._setptr) {
				duk_push_godot_method(ctx, prop._setptr);
				masks |= DUK_DEFPROP_HAVE_SETTER;
			}

			if (masks) {
				duk_def_prop(ctx, prototype_idx, masks);
			} else {
				duk_pop(ctx);
			}
			key = cls->property_setget.next(key);
		}
	}
}

void DuktapeBindingHelper::duk_push_godot_method(duk_context *ctx, const MethodBind *mb) {

	DuktapeHeapObject **val_ele = method_bindings.getptr(mb);
	DuktapeHeapObject *heap_ptr = val_ele ? (*val_ele) : NULL;

	if (heap_ptr) {
		duk_push_heapptr(ctx, heap_ptr);
	} else {
		duk_push_c_function(ctx, duk_godot_object_method, mb->is_vararg() ? DUK_VARARGS : mb->get_argument_count());
		duk_push_pointer(ctx, (void *)mb);
		duk_put_prop_literal(ctx, -2, DUK_HIDDEN_SYMBOL("mb"));

		heap_ptr = duk_get_heapptr(ctx, -1);
		if (heap_ptr) {
			method_bindings[mb] = heap_ptr;
		}
	}
}
