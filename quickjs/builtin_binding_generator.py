#!/usr/bin/env python
import json, os

DIR = os.path.abspath( os.path.dirname(__file__) )
OUTPUT_FILE = os.path.join(DIR, "quickjs_builtin_binder.gen.cpp")
API = json.load(open(os.path.join(DIR, '..', 'buitin_api.gen.json'), 'r'))

VariantTypes = {
	"boolean": "Variant::BOOL",
	"number": "Variant::REAL",
	"string": "Variant::STRING",
	"Vector2": "Variant::VECTOR2",
	"Vector3": "Variant::VECTOR3",
	"Basis": "Variant::BASIS",
	"Quat": "Variant::QUAT",
	"Color": "Variant::COLOR",
	"Rect2": "Variant::RECT2",
	"RID": "Variant::_RID",
	"Transform2D": "Variant::TRANSFORM2D",
	"Plane": "Variant::PLANE",
	"AABB": "Variant::AABB",
	"Transform": "Variant::TRANSFORM",
	"PackedByteArray": "Variant::PACKED_BYTE_ARRAY",
	"PackedInt32Array": "Variant::PACKED_INT32_ARRAY",
	"PackedFloat32Array": "Variant::PACKED_FLOAT32_ARRAY",
	"PackedStringArray": "Variant::PACKED_STRING_ARRAY",
	"PackedVector2Array": "Variant::PACKED_VECTOR2_ARRAY",
	"PackedVector3Array": "Variant::PACKED_VECTOR3_ARRAY",
	"PackedColorArray": "Variant::PACKED_COLOR_ARRAY",
	# "PoolByteArray": "Variant::POOL_BYTE_ARRAY",
	# "PoolIntArray": "Variant::POOL_INT_ARRAY",
	# "PoolRealArray": "Variant::POOL_REAL_ARRAY",
	# "PoolStringArray": "Variant::POOL_STRING_ARRAY",
	# "PoolVector2Array": "Variant::POOL_VECTOR2_ARRAY",
	# "PoolVector3Array": "Variant::POOL_VECTOR3_ARRAY",
	# "PoolColorArray": "Variant::POOL_COLOR_ARRAY",
}

JSToGodotTemplates = {
	"number": 'QuickJSBinder::js_to_number(ctx, ${arg})',
	"string": 'QuickJSBinder::js_to_string(ctx, ${arg})',
	"boolean": 'QuickJSBinder::js_to_bool(ctx, ${arg})',
	"Vector2": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getVector2()',
	"Rect2": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getRect2()',
	"Color": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getColor()',
	"RID": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getRID()',
	"AABB": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getAABB()',
	"Plane": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getPlane()',
	"Quat": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getQuat()',
	"Transform2D": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getTransform2D()',
	"Vector3": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getVector3()',
	"Basis": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getBasis()',
	"Transform": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getTransform()',
	"PackedByteArray": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getPackedByteArray()',
	"PackedInt32Array": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getPackedInt32Array()',
	"PackedFloat32Array": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getPackedFloat32Array()',
	"PackedStringArray": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getPackedStringArray()',
	"PackedVector2Array": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getPackedVector2Array()',
	"PackedVector3Array": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getPackedVector3Array()',
	"PackedColorArray": '*(BINDING_DATA_FROM_JS(ctx, ${arg}))->getPackedColorArray()',
	"Variant": '(BINDING_DATA_FROM_JS(ctx, ${arg}))->get_value()',
}

GodotTypeNames = {
	"number": "real_t",
	"string": "String",
	"boolean": "bool",
	"Vector2": "Vector2",
	"Rect2": "Rect2",
	"Color": "Color",
	"RID": "RID",
	"AABB": "AABB",
	"Plane": "Plane",
	"Quat": "Quat",
	"Transform2D": "Transform2D",
	"Vector3": "Vector3",
	"Basis": "Basis",
	"Transform": "Transform",
	"PackedByteArray": "PackedByteArray",
	"PackedInt32Array": "PackedInt32Array",
	"PackedFloat32Array": "PackedFloat32Array",
	"PackedStringArray": "PackedStringArray",
	"PackedVector2Array": "PackedVector2Array",
	"PackedVector3Array": "PackedVector3Array",
	"PackedColorArray": "PackedColorArray",
	# "PoolByteArray": "PoolByteArray",
	# "PoolIntArray": "PoolIntArray",
	# "PoolRealArray": "PoolRealArray",
	# "PoolStringArray": "PoolStringArray",
	# "PoolVector2Array": "PoolVector2Array",
	# "PoolVector3Array": "PoolVector3Array",
	# "PoolColorArray": "PoolColorArray",
	"Variant": "Variant",
}

GodotToJSTemplates = {
	"number": 'QuickJSBinder::to_js_number(ctx, ${arg})',
	"string": 'QuickJSBinder::to_js_string(ctx, ${arg})',
	"boolean": 'QuickJSBinder::to_js_bool(ctx, ${arg})',
	"Vector2": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"Rect2": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"Color": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"RID": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"AABB": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"Plane": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"Quat": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"Transform2D": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"Vector3": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"Basis": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"Transform": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"PackedByteArray": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"PackedInt32Array": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"PackedFloat32Array": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"PackedStringArray": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"PackedVector2Array": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"PackedVector3Array": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"PackedColorArray": 'QuickJSBuiltinBinder::new_object_from(ctx, ${arg})',
	"Variant": 'QuickJSBinder::variant_to_var(ctx, ${arg})',
}

def apply_parttern(template, values):
	for key in values:
		template = template.replace( '${' + key + '}', values[key])
	return template


def generate_constructor(cls):
	TemplateConstructorName = '${class}_constructor'
	TemplateConstructorDeclare = 'static JSValue ${class}_constructor(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv);\n'
	TemplateConstructor = '''
static JSValue ${func}(JSContext *ctx, JSValueConst new_target, int argc, JSValueConst *argv) {
	${class} *ptr = memnew(${class});
	${initializer}
	return QuickJSBuiltinBinder::bind_builtin_object_static(ctx, ${type}, ptr);
}
'''
	TemplateSimplePoolArrays = '''
	if (argc == 1) {
#ifdef DEBUG_METHODS_ENABLED
		ERR_FAIL_COND_V(!JS_IsArray(ctx, argv[0]), (JS_ThrowTypeError(ctx, "Array expected for argument #0 of ${class}(from)")));
#endif
		Variant arr = QuickJSBinder::var_to_variant(ctx, argv[0]);
		ptr->operator=(arr);
	}
	'''
	TemplatePoolArrays = '''
	if (argc == 1) {
		if (JS_IsArray(ctx, argv[0])) {
			Variant arr = QuickJSBinder::var_to_variant(ctx, argv[0]);
			ptr->operator=(arr);
		} else if (JS_IsArrayBuffer(argv[0])) {
			size_t size;
			uint8_t *buffer = JS_GetArrayBuffer(ctx, &size, argv[0]);
			if (size) {
				if (size % sizeof(${element}) != 0) {
					ERR_PRINTS("Length of the ArrayBuffer does not match for ${class}");
				}
				ptr->resize(size / sizeof(${element}));
				copymem(ptr->write().ptr(), buffer, size / sizeof(${element}) * sizeof(${element}));
			}
		} else {
			memdelete(ptr);
#ifdef DEBUG_METHODS_ENABLED
			ERR_FAIL_COND_V(false, (JS_ThrowTypeError(ctx, "Array or ArrayBuffer expected for argument #0 of ${class}(from)")));
#endif
		}
	}
	'''
	ConstructorInitializers = {
		"Vector2": '''
	if (argc == 2) {
		ptr->x = QuickJSBinder::js_to_number(ctx, argv[0]);
		ptr->y = QuickJSBinder::js_to_number(ctx, argv[1]);
	} else if (argc == 1) {
		if (ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0])) {
			if (bind->type == Variant::VECTOR2) {
				ptr->operator=(*bind->getVector2());
			}
		} else {
			ptr->x = QuickJSBinder::js_to_number(ctx, argv[0]);
			ptr->y = ptr->x;
		}
	}
''',
		"Vector3": '''
	if (argc == 1) {
		if (ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0])) {
			if (bind->type == Variant::VECTOR3)
				ptr->operator=(*bind->getVector3());
		} else {
			ptr->x = QuickJSBinder::js_to_number(ctx, argv[0]);
			ptr->z = ptr->y = ptr->x;
		}
	} else if (argc == 3) {
		ptr->x = QuickJSBinder::js_to_number(ctx, argv[0]);
		ptr->y = QuickJSBinder::js_to_number(ctx, argv[1]);
		ptr->z = QuickJSBinder::js_to_number(ctx, argv[2]);
	}
''',
		"Color": '''
	if (argc >= 3) {
		ptr->r = QuickJSBinder::js_to_number(ctx, argv[0]);
		ptr->g = QuickJSBinder::js_to_number(ctx, argv[1]);
		ptr->b = QuickJSBinder::js_to_number(ctx, argv[2]);
		ptr->a = (argc >= 4) ? QuickJSBinder::js_to_number(ctx, argv[3]) : 1.0f;
	} else if (argc == 1) {
		if (JS_IsNumber(argv[0])) {
			ptr->operator=(Color::hex(QuickJSBinder::js_to_uint(ctx, argv[0])));
		} else if (JS_IsString(argv[0])) {
			ptr->operator=(Color::html(QuickJSBinder::js_to_string(ctx, argv[0])));
		} else if (ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0])) {
			if (bind->type == Variant::COLOR) {
				ptr->operator=(*bind->getColor());
			}
		}
	}
''',
		"Rect2": '''
	if (argc == 4) {
		ptr->position.x = QuickJSBinder::js_to_number(ctx, argv[0]);
		ptr->position.y = QuickJSBinder::js_to_number(ctx, argv[1]);
		ptr->size.x = QuickJSBinder::js_to_number(ctx, argv[2]);
		ptr->size.y = QuickJSBinder::js_to_number(ctx, argv[3]);
	} else if (argc == 2) {
#ifdef DEBUG_METHODS_ENABLED
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR2, argv[0]), (JS_ThrowTypeError(ctx, "Vector2 expected for argument 0 of Rect2(position, size)")));
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR2, argv[1]), (JS_ThrowTypeError(ctx, "Vector2 expected for argument 1 of Rect2(position, size)")));
#endif
		ECMAScriptGCHandler *param0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ECMAScriptGCHandler *param1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
		ptr->position = *param0->getVector2();
		ptr->size = *param1->getVector2();
	} else if (argc == 1) {
		if (ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0])) {
			if (bind->type == Variant::RECT2)
				ptr->operator=(*bind->getRect2());
		}
	}
''',
		"AABB": '''
	if (argc == 2) {
#ifdef DEBUG_METHODS_ENABLED
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 0 of AABB(position, size)")));
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[1]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 1 of AABB(position, size)")));
#endif
		ECMAScriptGCHandler *param0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ECMAScriptGCHandler *param1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
		ptr->position = *param0->getVector3();
		ptr->size = *param1->getVector3();
	} else if (argc == 1) {
		if (ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0])) {
			if (bind->type == Variant::AABB)
				ptr->operator=(*bind->getAABB());
		}
	}
''',
		"Plane": '''
	if (argc == 4) {
		ptr->normal.x = QuickJSBinder::js_to_number(ctx, argv[0]);
		ptr->normal.y = QuickJSBinder::js_to_number(ctx, argv[1]);
		ptr->normal.z = QuickJSBinder::js_to_number(ctx, argv[2]);
		ptr->d = QuickJSBinder::js_to_number(ctx, argv[3]);
	} else if (argc == 3) {
#ifdef DEBUG_METHODS_ENABLED
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 0 of Plane(v1, v2, v3)")));
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[1]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 1 of Plane(v1, v2, v3)")));
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[2]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 2 of Plane(v1, v2, v3)")));
#endif
		ECMAScriptGCHandler *param0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ECMAScriptGCHandler *param1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
		ECMAScriptGCHandler *param2 = BINDING_DATA_FROM_JS(ctx, argv[2]);
		ptr->operator=(Plane(*param0->getVector3(), *param1->getVector3(), *param2->getVector3()));
	} else if (argc == 2) {
#ifdef DEBUG_METHODS_ENABLED
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 0 of Plane(normal, d)")));
#endif
		ECMAScriptGCHandler *param0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ptr->operator=(Plane(*param0->getVector3(), QuickJSBinder::js_to_number(ctx, argv[1])));
	} else if (argc == 1) {
		if (ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0])) {
			if (bind->type == Variant::PLANE)
				ptr->operator=(*bind->getPlane());
		}
	}
''',
		"Quat": '''
	if (argc == 4) {
		ptr->x = QuickJSBinder::js_to_number(ctx, argv[0]);
		ptr->y = QuickJSBinder::js_to_number(ctx, argv[1]);
		ptr->z = QuickJSBinder::js_to_number(ctx, argv[2]);
		ptr->w = QuickJSBinder::js_to_number(ctx, argv[3]);
	} else if (argc == 2) {
#ifdef DEBUG_METHODS_ENABLED
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 0 of Quat(axis, angle)")));
#endif
		ECMAScriptGCHandler *param0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ptr->operator=(Quat(*param0->getVector3(), QuickJSBinder::js_to_number(ctx, argv[1])));
	} else if (argc == 1) {
		if (ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0])) {
			if (bind->type == Variant::QUAT) {
				ptr->operator=(*bind->getQuat());
			} else if (bind->type == Variant::BASIS) {
				ptr->operator=(*bind->getBasis());
			} else if (bind->type == Variant::VECTOR3) {
				ptr->set_euler(*bind->getVector3());
			}
		}
	}
''',
		"Transform2D": '''
	if (argc == 3) {
#ifdef DEBUG_METHODS_ENABLED
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR2, argv[0]), (JS_ThrowTypeError(ctx, "Vector2 expected for argument 0 of Transform2D(x_axis, y_axis, origin)")));
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR2, argv[1]), (JS_ThrowTypeError(ctx, "Vector2 expected for argument 1 of Transform2D(x_axis, y_axis, origin)")));
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR2, argv[2]), (JS_ThrowTypeError(ctx, "Vector2 expected for argument 2 of Transform2D(x_axis, y_axis, origin)")));
#endif
		ECMAScriptGCHandler *param0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ECMAScriptGCHandler *param1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
		ECMAScriptGCHandler *param2 = BINDING_DATA_FROM_JS(ctx, argv[2]);
		ptr->elements[0].operator=(*param0->getVector2());
		ptr->elements[1].operator=(*param1->getVector2());
		ptr->elements[2].operator=(*param2->getVector2());
	} else if (argc == 2) {
#ifdef DEBUG_METHODS_ENABLED
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR2, argv[1]), (JS_ThrowTypeError(ctx, "Vector2 expected for argument 1 of Transform2D(rotation, position)")));
#endif
		ECMAScriptGCHandler *param1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
		ptr->set_origin(*param1->getVector2());
		ptr->set_rotation(QuickJSBinder::js_to_number(ctx, argv[0]));
	} else if (argc == 1) {
		if (ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0])) {
			if (bind->type == Variant::TRANSFORM2D)
				ptr->operator=(*bind->getTransform2D());
			else if (Variant::can_convert(bind->type, Variant::TRANSFORM2D)) {
				ptr->operator=(bind->get_value());
			}
		}
	}
''',
		"Basis": '''
	if (argc == 3) {
#ifdef DEBUG_METHODS_ENABLED
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 0 of Basis(x_axis, y_axis, z_axis)")));
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[1]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 1 of Basis(x_axis, y_axis, z_axis)")));
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[2]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 2 of Basis(x_axis, y_axis, z_axis)")));
#endif
		ECMAScriptGCHandler *param0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ECMAScriptGCHandler *param1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
		ECMAScriptGCHandler *param2 = BINDING_DATA_FROM_JS(ctx, argv[2]);
		ptr->elements[0].operator=(*param0->getVector3());
		ptr->elements[1].operator=(*param1->getVector3());
		ptr->elements[2].operator=(*param2->getVector3());
	} else if (argc == 2) {
#ifdef DEBUG_METHODS_ENABLED
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 0 of Basis(axis, phi)")));
#endif
		ECMAScriptGCHandler *param0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ptr->set_axis_angle(*param0->getVector3(), QuickJSBinder::js_to_number(ctx, argv[1]));
	} else if (argc == 1) {
		if (ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0])) {
			if (bind->type == Variant::VECTOR3) {
				ptr->set_euler(*bind->getVector3());
			} else if (bind->type == Variant::QUAT) {
				ptr->set_quat(*bind->getQuat());
			} else if (bind->type == Variant::BASIS) {
				ptr->operator=(*bind->getBasis());
			}
		}
	}
''',
		"Transform": '''
	if (argc == 4) {
#ifdef DEBUG_METHODS_ENABLED
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[0]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 0 of Transform(x_axis, y_axis, z_axis, origin)")));
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[1]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 1 of Transform(x_axis, y_axis, z_axis, origin)")));
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[2]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 2 of Transform(x_axis, y_axis, z_axis, origin)")));
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[3]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 3 of Transform(x_axis, y_axis, z_axis, origin)")));
#endif
		ECMAScriptGCHandler *param0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ECMAScriptGCHandler *param1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
		ECMAScriptGCHandler *param2 = BINDING_DATA_FROM_JS(ctx, argv[2]);
		ECMAScriptGCHandler *param3 = BINDING_DATA_FROM_JS(ctx, argv[3]);

		ptr->basis.elements[0].operator=(*param0->getVector3());
		ptr->basis.elements[1].operator=(*param1->getVector3());
		ptr->basis.elements[2].operator=(*param2->getVector3());
		ptr->origin.operator=(*param3->getVector3());
	} else if (argc == 2) {
#ifdef DEBUG_METHODS_ENABLED
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::BASIS, argv[0]), (JS_ThrowTypeError(ctx, "Basis expected for argument 0 of Transform(basis, origin)")));
		ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, Variant::VECTOR3, argv[1]), (JS_ThrowTypeError(ctx, "Vector3 expected for argument 1 of Transform(basis, origin)")));
#endif
		ECMAScriptGCHandler *param0 = BINDING_DATA_FROM_JS(ctx, argv[0]);
		ECMAScriptGCHandler *param1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
		ptr->basis.operator=(*param0->getBasis());
		ptr->origin.operator=(*param1->getVector3());
	} else if (argc == 1) {
		if (ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0])) {
			if (bind->type == Variant::TRANSFORM) {
				ptr->operator=(*bind->getTransform());
			} else if (Variant::can_convert(bind->type, Variant::TRANSFORM)) {
				ptr->operator=(bind->get_value());
			}
		}
	}
''',
		"PackedByteArray": apply_parttern(TemplatePoolArrays, {"class": "PackedByteArray", "type": "Variant::PACKED_BYTE_ARRAY", "element": "uint8_t"}),
		"PackedInt32Array": apply_parttern(TemplatePoolArrays, {"class": "PackedInt32Array", "type": "Variant::PACKED_INT32_ARRAY", "element": "int"}),
		"PackedFloat32Array": apply_parttern(TemplatePoolArrays, {"class": "PackedFloat32Array", "type": "Variant::PACKED_FLOAT32_ARRAY", "element": "real_t"}),
		"PackedVector2Array": apply_parttern(TemplatePoolArrays, {"class": "PackedVector2Array", "type": "Variant::PACKED_VECTOR2_ARRAY", "element": "Vector2"}),
		"PackedVector3Array": apply_parttern(TemplatePoolArrays, {"class": "PackedVector3Array", "type": "Variant::PACKED_VECTOR3_ARRAY", "element": "Vector3"}),
		"PackedColorArray": apply_parttern(TemplatePoolArrays, {"class": "PackedColorArray", "type": "Variant::PACKED_COLOR_ARRAY", "element": "Color"}),
		"PackedStringArray": apply_parttern(TemplateSimplePoolArrays,{"class": "PackedStringArray", "type": "Variant::PACKED_STRING_ARRAY", "element": "String"}),
	}
	class_name = cls['name']
	constructor_name = apply_parttern(TemplateConstructorName, {"class": class_name})
	constructor_declare = apply_parttern(TemplateConstructorDeclare, {"class": class_name})
	
	initializer = ''
	if class_name in ConstructorInitializers:
		initializer = ConstructorInitializers[class_name]
	consturctor = apply_parttern(TemplateConstructor, {
		'class': class_name,
		'type': VariantTypes[class_name],
		'func': constructor_name,
		'initializer': initializer
	})
	return constructor_name, constructor_declare, consturctor

def generate_property_bindings(cls):
	class_name = cls['name']
	TemplateDeclar = 'static void bind_${class}_properties(JSContext *ctx);\n'
	TemplateBind = '\tbind_${class}_properties(ctx);\n'
	def generate_members(cls):
		Template = '''
	JSCFunctionMagic *getter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
		ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
		const ${class} *ptr = bind->get${class}();
		switch (magic) {\
${getters}
		}
		return JS_UNDEFINED;
	};
	
	JSCFunctionMagic *setter = [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv, int magic) -> JSValue {
		ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
		${class} *ptr = bind->get${class}();\
${validation}
		switch (magic) {\
${setters}
		}
		return JS_DupValue(ctx, argv[0]);
	};
${bindings}
		'''
		TemplateGetterItem = '''
			case ${index}:
				return ${value};'''
		TemplateSetterItem = '''
			case ${index}:
#ifdef DEBUG_METHODS_ENABLED
				ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, ${type}, argv[0]), (JS_ThrowTypeError(ctx, "${type_name} expected for ${class}.${name}")));
#endif
				ptr->${native} = ${value};
				break;'''
		TemplateItemBinding = '\tbinder->get_builtin_binder().register_property(${type}, "${name}", getter, setter, ${index});\n'
		getters = ''
		setters = ''
		bindings = ''
		for i in range(len(cls['properties'])):
			p = cls['properties'][i]
			type = p['type']
			name = p['name']
			native_name = p['native']
			getters += apply_parttern(TemplateGetterItem, {
				'index': str(i),
				'value': apply_parttern(GodotToJSTemplates[type], { 'arg': apply_parttern('ptr->${native}', {'native': native_name}) })
			})
			setters += apply_parttern(TemplateSetterItem, {
				'index': str(i),
				'name': name,
				'native': native_name,
				'type': VariantTypes[type],
				'type_name': type,
				'class': class_name,
				'value': apply_parttern(JSToGodotTemplates[type], {'arg': 'argv[0]'})
			})
			bindings += apply_parttern(TemplateItemBinding, {'index': str(i), 'name': name, 'type': VariantTypes[class_name]})
		return apply_parttern(Template, {
			'class': class_name,
			'getters': getters,
			'setters': setters,
			'bindings': bindings,
			'validation': ''
		})
	
	def generate_methods(cls):
		TemplateMethod = '''
	binder->get_builtin_binder().register_method(
		${type},
		"${name}",
		[](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
			ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, this_val);
			${class} *ptr = bind->get${class}();\
${arg_declars}
			${call}
			return ${return};
		},
		${argc});'''
		TemplateArgDeclear = '''
#ifdef DEBUG_METHODS_ENABLED
			ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, ${type}, argv[${index}]), (JS_ThrowTypeError(ctx, "${type_name} expected for argument ${index} of ${class}.${name}")));
#endif
			const ${godot_type} &arg${index} = ${arg};
'''
		TemplateReturnValue = '${godot_type} ret = '
		bindings = ''
		for m in cls['methods']:
			args = ''
			arg_declars = ''
			for i in range(len(m['arguments'])):
				arg = m['arguments'][i]
				arg_type = arg['type']
				arg_declars += apply_parttern(TemplateArgDeclear, {
					'index': str(i),
					'type': VariantTypes[arg_type],
					'type_name': arg_type,
					'class': class_name,
					'name': m['name'],
					'arg': apply_parttern(JSToGodotTemplates[arg_type], {'arg': 'argv[' + str(i) +']'}),
					'godot_type': GodotTypeNames[arg_type],
				})
				if i > 0: args += ', '
				args += 'arg' + str(i)
			CallTemplate = ('' if m['return'] == 'void' else (apply_parttern(TemplateReturnValue, {"godot_type": GodotTypeNames[m['return']]}))) + 'ptr->${native_method}(${args});'
			call = apply_parttern(CallTemplate, {'native_method': m['native_method'], 'args': args})
			bindings += apply_parttern(TemplateMethod, {
				"class": class_name,
				"type": VariantTypes[class_name],
				"name": m['name'],
				"call": call,
				"arg_declars": arg_declars,
				"argc": str(len(m['arguments'])),
				"return": 'JS_UNDEFINED' if m['return'] == 'void' else apply_parttern(GodotToJSTemplates[m['return']], {'arg': 'ret'}),
			})
		return bindings
		
	def generate_constants(cls):
		ConstTemplate = '\tbinder->get_builtin_binder().register_constant(${type}, "${name}", ${value});\n'
		bindings = ''
		for c in cls['constants']:
			bindings += apply_parttern(ConstTemplate, {
				"name": c['name'],
				"type": VariantTypes[class_name],
				"value": c['value']
			})
		return bindings
	
	def genertate_operators(cls):
		OperatorMap = {
			'operator+': '+',
			'operator-': '-',
			'operator*': '*',
			'operator/': '/',
			'operator==': '==',
			'operator<': '<'
		}
		TargetDeclearTemplate = '''
#ifdef DEBUG_METHODS_ENABLED
			ERR_FAIL_COND_V(!QuickJSBinder::validate_type(ctx, ${type}, argv[1]), (JS_ThrowTypeError(ctx, "${target_class} expected for ${class}.${operator}")));
#endif
			ECMAScriptGCHandler *bind1 = BINDING_DATA_FROM_JS(ctx, argv[1]);
			${target_class} *target = bind1->get${target_class}();\
'''
		OperatorTemplate = '''
	JS_SetPropertyStr(ctx, base_operators, "${js_op}",
		JS_NewCFunction(ctx, [](JSContext *ctx, JSValueConst this_val, int argc, JSValueConst *argv) {
			ECMAScriptGCHandler *bind = BINDING_DATA_FROM_JS(ctx, argv[0]);
			${class} *ptr = bind->get${class}();\
${target_declear}
			${call}
			return ${return};
		},
		"${name}",
		${argc})
	);
	'''
		TemplateReturnValue = '${godot_type} ret = '
		bindings = '''\
	Vector<JSValue> operators;
	JSValue base_operators = JS_NewObject(ctx);
'''
		for o in cls['operators']:
			op = o['native_method']
			if op in OperatorMap:
				argc = len(o['arguments']) + 1
				js_op = OperatorMap[op]
				if argc <= 1:
					if op in ['operator-']:
						js_op = 'neg'
					
				args = ''
				target_declear = ''
				if argc > 1:
					arg_class = o['arguments'][0]['type']
					target_declear = apply_parttern(TargetDeclearTemplate, {
						'target_class': arg_class,
						'type': VariantTypes[arg_class],
						'class': class_name,
						'operator': o['native_method'],
					})
					args = '*target'
				CallTemplate = ('' if o['return'] == 'void' else apply_parttern(TemplateReturnValue, {'godot_type': GodotTypeNames[o['return']] })) + 'ptr->${op}(${args});'
				call = apply_parttern(CallTemplate, {'op': op, 'args': args})
				bindings += apply_parttern(OperatorTemplate, {
					'type': VariantTypes[class_name],
					'class': class_name,
					'js_op': js_op,
					'call': call,
					'name': o['name'],
					'target_declear': target_declear,
					"return": 'JS_UNDEFINED' if o['return'] == 'void' else apply_parttern(GodotToJSTemplates[o['return']], {'arg': 'ret'}),
					'argc': str(argc)
				})
		bindings += apply_parttern('''
	operators.push_back(base_operators);
	binder->get_builtin_binder().get_cross_type_operators(${type}, operators);
	binder->get_builtin_binder().register_operators(${type}, operators);
''', {'type': VariantTypes[class_name]})
		return bindings
	
	TemplateBindDefine = '''
static void bind_${class}_properties(JSContext *ctx) {
	QuickJSBinder *binder = QuickJSBinder::get_context_binder(ctx);
${members}
${operators}
${constants}
${methods}
}
'''
	class_name = cls['name']
	property_declare = apply_parttern(TemplateDeclar, {"class": class_name})
	property_defines = apply_parttern(TemplateBindDefine, {
		"class": class_name,
		"members": generate_members(cls) if len(cls['properties']) else '',
		"methods": generate_methods(cls),
		"constants": generate_constants(cls),
		"operators": genertate_operators(cls),
	})
	property_bind = apply_parttern(TemplateBind, {"class": class_name})
	return property_declare, property_defines, property_bind


def generate_class_bind_action(cls, constructor):
	Template = '\tregister_builtin_class(${type}, "${class}", ${constructor}, ${argc});\n'
	return apply_parttern(Template, {
		'class': cls['name'],
		'constructor': constructor,
		'type': VariantTypes[cls['name']],
		'argc': str(cls['constructor_argc'])
	})



def generate_builtin_bindings():
	Template = '''\
#include "core/variant.h"
#include "quickjs_binder.h"
#include "quickjs_builtin_binder.h"

#ifndef inf
#define inf INFINITY
#endif

${declarations}

void QuickJSBuiltinBinder::bind_builtin_classes_gen() {

${bindings}
}

${definitions}
'''
	
	declarations = ''
	definitions = ''
	bindings = ''
	for cls in API:
		constructor_name, constructor_declare, consturctor = generate_constructor(cls)
		declarations += constructor_declare
		definitions += consturctor
		bindings += generate_class_bind_action(cls, constructor_name)
		
		property_declare, property_defines, property_bind= generate_property_bindings(cls)
		declarations += property_declare
		definitions += property_defines
		bindings += property_bind
	
	output = apply_parttern(Template, {
		'declarations': declarations,
		'bindings': bindings,
		'definitions': definitions,
	})
	file = open(OUTPUT_FILE, 'w')
	file.write(output)

if __name__ == "__main__":
	generate_builtin_bindings()
