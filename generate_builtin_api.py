#!/usr/bin/env python
import json, os, sys
import xml.etree.ElementTree as ET

BUILTIN_CLASSES = [
	'Vector2',
	'Rect2',
	'Color',
	'Vector3',
	'Basis',
	'Quat',
	'RID',
	'Transform2D',
	'Plane',
	'AABB',
	"Transform",
	"PoolByteArray",
	"PoolIntArray",
	"PoolRealArray",
	"PoolStringArray",
	"PoolVector2Array",
	"PoolVector3Array",
	"PoolColorArray",
]

MAX_CONSTRUCTOR_ARGC = {
	'Vector2': 2,
	'Rect2': 4,
	'Color': 4,
	'Vector3': 3,
	'Basis': 0,
	'Quat': 0,
	'RID': 0,
	'Transform2D': 0,
	'Plane': 0,
	'AABB': 0,
	"Transform": 0,
	"PoolByteArray": 0,
	"PoolIntArray": 0,
	"PoolRealArray": 0,
	"PoolStringArray": 0,
	"PoolVector2Array": 0,
	"PoolVector3Array": 0,
	"PoolColorArray": 0,
}

TYPE_MAP = {
	'int': 'number',
	'float': 'number',
	'bool': 'boolean',
	'String': 'string',
	'NodePath': 'string',
}

METHOD_OP_EQUALS = {
	"arguments": [
		{
			"default_value": None,
			"has_default_value": False,
			"type": "${class_name}"
		}
	],
	"name": "equals",
	"native_method": "operator==",
	"return": "boolean"
}

METHOD_OP_ADD = {
	"arguments": [
		{
			"default_value": None,
			"has_default_value": False,
			"type": "${class_name}"
		}
	],
	"name": "add",
	"native_method": "operator+",
	"return": "${class_name}"
}

METHOD_OP_ADD_ASSIGN = {
	"arguments": [
		{
			"default_value": None,
			"has_default_value": False,
			"type": "${class_name}"
		}
	],
	"name": "add_assign",
	"native_method": "operator+=",
	"return": "this"
}

METHOD_OP_SUB = {
	"arguments": [
		{
			"default_value": None,
			"has_default_value": False,
			"type": "${class_name}"
		}
	],
	"name": "subtract",
	"native_method": "operator-",
	"return": "${class_name}"
}

METHOD_OP_SUB_ASSIGN = {
	"arguments": [
		{
			"default_value": None,
			"has_default_value": False,
			"type": "${class_name}"
		}
	],
	"name": "subtract_assign",
	"native_method": "operator-=",
	"return": "this"
}

METHOD_OP_MUL = {
	"arguments": [
		{
			"default_value": None,
			"has_default_value": False,
			"type": "${class_name}"
		}
	],
	"name": "multiply",
	"native_method": "operator*",
	"return": "${class_name}"
}

METHOD_OP_MUL_ASSIGN = {
	"arguments": [
		{
			"default_value": None,
			"has_default_value": False,
			"type": "${class_name}"
		}
	],
	"name": "multiply_assign",
	"native_method": "operator*=",
	"return": "this"
}

METHOD_OP_DIV = {
	"arguments": [
		{
			"default_value": None,
			"has_default_value": False,
			"type": "${class_name}"
		}
	],
	"name": "multiply",
	"native_method": "operator/",
	"return": "${class_name}"
}

METHOD_OP_DIV_ASSIGN = {
	"arguments": [
		{
			"default_value": None,
			"has_default_value": False,
			"type": "${class_name}"
		}
	],
	"name": "multiply_assign",
	"native_method": "operator/=",
	"return": "this"
}

METHOD_OP_NEG = {
	"arguments": [],
	"name": "negate",
	"native_method": "operator-",
	"return": "${class_name}"
}

METHOD_OP_LESS = {
	"arguments": [
		{
			"default_value": None,
			"has_default_value": False,
			"type": "${class_name}"
		}
	],
	"name": "less",
	"native_method": "operator<",
	"return": "boolean"
}


METHOD_OP_LESS_EQAUL = {
	"arguments": [
		{
			"default_value": None,
			"has_default_value": False,
			"type": "${class_name}"
		}
	],
	"name": "less_equal",
	"native_method": "operator<=",
	"return": "boolean"
}

METHOD_POOL_ARRAY_GET = {
	"arguments": [
		{
			"default_value": None,
			"has_default_value": False,
			"type": "number"
		}
	],
	"name": "get",
	"native_method": "operator[]",
	"return": "Variant"
}

IGNORED_PROPS = {
	"Rect2": ['end', 'grow_margin'],
	"Color": ['h', 's', 'v', 'r8', 'g8', 'b8', 'a8'],
	"Transform2D": ['xform', 'xform_inv'],
	"Basis": ['is_equal_approx'],
	"Plane": ['intersects_segment', 'intersects_ray', 'intersect_3'],
	"AABB": ['end'],
	"Transform": ['xform', 'xform_inv'],
	"PoolByteArray": ['compress', 'decompress', 'decompress_dynamic', 'get_string_from_ascii', 'get_string_from_utf8', 'hex_encode'],
}

PROPERTY_REMAP = {
	"Transform2D": {
		"x": "elements[0]",
		"y": "elements[1]",
		"origin": "elements[2]",
	},
	"Basis": {
		"x": "elements[0]",
		"y": "elements[1]",
		"z": "elements[2]",
	},
	"Plane": {
		"x": "normal.x",
		"y": "normal.y",
		"z": "normal.z",
	}
}

OPERATOR_METHODS = {
	"Vector2": [
		METHOD_OP_NEG,
		METHOD_OP_EQUALS,
		METHOD_OP_LESS,
		METHOD_OP_LESS_EQAUL,
		METHOD_OP_ADD,
		METHOD_OP_ADD_ASSIGN,
		METHOD_OP_SUB,
		METHOD_OP_SUB_ASSIGN,
		METHOD_OP_MUL,
		METHOD_OP_MUL_ASSIGN,
		METHOD_OP_DIV,
		METHOD_OP_DIV_ASSIGN,
	],
	"Vector3": [
		METHOD_OP_NEG,
		METHOD_OP_EQUALS,
		METHOD_OP_LESS,
		METHOD_OP_LESS_EQAUL,
		METHOD_OP_ADD,
		METHOD_OP_ADD_ASSIGN,
		METHOD_OP_SUB,
		METHOD_OP_SUB_ASSIGN,
		METHOD_OP_MUL,
		METHOD_OP_MUL_ASSIGN,
		METHOD_OP_DIV,
		METHOD_OP_DIV_ASSIGN,
	],
	"Basis": [
		METHOD_OP_EQUALS,
		METHOD_OP_ADD,
		METHOD_OP_ADD_ASSIGN,
		METHOD_OP_SUB,
		METHOD_OP_SUB_ASSIGN,
		METHOD_OP_MUL,
		METHOD_OP_MUL_ASSIGN,
	],
	"Quat": [
		METHOD_OP_NEG,
		METHOD_OP_EQUALS,
		METHOD_OP_ADD,
		METHOD_OP_ADD_ASSIGN,
		METHOD_OP_SUB,
		METHOD_OP_SUB_ASSIGN,
	],
	"Rect2": [
		METHOD_OP_EQUALS
	],
	"Transform2D": [
		METHOD_OP_EQUALS,
		METHOD_OP_MUL,
		METHOD_OP_MUL_ASSIGN,
	],
	"Color": [
		METHOD_OP_NEG,
		METHOD_OP_EQUALS,
		METHOD_OP_LESS,
		METHOD_OP_ADD,
		METHOD_OP_ADD_ASSIGN,
		METHOD_OP_SUB,
		METHOD_OP_SUB_ASSIGN,
		METHOD_OP_MUL,
		METHOD_OP_MUL_ASSIGN,
		METHOD_OP_DIV,
		METHOD_OP_DIV_ASSIGN,
	],
	"RID": [
		METHOD_OP_EQUALS,
		METHOD_OP_LESS,
		METHOD_OP_LESS_EQAUL,
	],
	"Plane": [
		METHOD_OP_NEG,
		METHOD_OP_EQUALS,
	],
	"AABB": [
		METHOD_OP_EQUALS,
	],
	"Transform": [
		METHOD_OP_EQUALS,
		METHOD_OP_MUL,
		METHOD_OP_MUL_ASSIGN,
	]
}

def apply_pattern(template, values):
	for key in values:
		template = template.replace( '${' + key + '}', values[key])
	return template

def parse_class(cls):
	class_name = cls.get('name')
	ret = {'name': class_name}
	members = []
	methods = []
	operators = []
	constants = []
	ret['properties'] = members
	ret['methods'] = methods
	ret['operators'] = operators
	ret['constants'] = constants
	ret['constructor_argc'] = MAX_CONSTRUCTOR_ARGC[class_name]
	
	for m in (cls.find("members") if cls.find("members") is not None else []):
		m_dict = dict(m.attrib)
		type = m_dict['type']
		name = m_dict['name']
		if (class_name in IGNORED_PROPS) and (name in IGNORED_PROPS[class_name]):
			continue
		if type in TYPE_MAP:
			type = TYPE_MAP[type]
		native_prop = name
		if class_name in PROPERTY_REMAP:
			if name in PROPERTY_REMAP[class_name]:
				native_prop = PROPERTY_REMAP[class_name][name]
		members.append({'name': name, 'type': type, 'native': native_prop})
	
	for m in (cls.find("methods") if cls.find("methods") is not None else []):
		m_dict = dict(m.attrib)
		method_name = m_dict['name']
		if method_name == class_name:
			continue# ignore constructors
		if class_name in IGNORED_PROPS and method_name in IGNORED_PROPS[class_name]:
			continue# ignored methods
		return_type = m.find("return").attrib["type"] if m.find("return") != None else "void"
		if return_type in TYPE_MAP:
			return_type = TYPE_MAP[return_type]
		arguments = []
		for arg in m.iter('argument'):
			dictArg = dict(arg.attrib)
			if "dictArg" in dictArg: dictArg.pop("index")
			dictArg["default_value"] = dictArg["default"] if "default" in dictArg else None
			if "default" in dictArg: dictArg.pop("default")
			type = dictArg['type']
			if type in TYPE_MAP:
				type = TYPE_MAP[type]
			arguments.append({
				'type': type,
				'default_value': dictArg['default_value'],
				'has_default_value': "default" in dictArg
			})
		methods.append({
			'name': method_name,
			'native_method': method_name,
			'return': return_type,
			'arguments': arguments,
		})
	if class_name.startswith("Pool") and class_name.endswith("Array"):
		methods.append(METHOD_POOL_ARRAY_GET)
	# add operator methods
	if class_name in OPERATOR_METHODS:
		for em in OPERATOR_METHODS[class_name]:
			operators.append(em)
	
	for c in (cls.find("constants") if cls.find("constants") is not None else []):
		const_name = c.get("name")
		if class_name in IGNORED_PROPS and const_name in IGNORED_PROPS[class_name]:
			continue
		constants.append(dict(c.attrib))
	return json.loads(apply_pattern(json.dumps(ret), {
		'class_name': class_name,
	}))

def generate_api_json(MODULE_DIR):
	DOCS_DIR = os.path.abspath(os.path.join(MODULE_DIR, "../../doc/classes"))
	if not os.path.isdir(DOCS_DIR) and len(sys.argv) > 1:
		DOCS_DIR = sys.argv[-1]
	OUTPUT_FILE = os.path.join(MODULE_DIR, "builtin_api.gen.json")

	classes = []
	for cls in BUILTIN_CLASSES:
		tree = ET.parse(open(os.path.join(DOCS_DIR, cls + '.xml'), 'r'))
		data = tree.getroot()
		classes.append(parse_class(data))
	json.dump(classes, open(OUTPUT_FILE, 'w'), ensure_ascii=False, indent=2, sort_keys=True)
	
if __name__ == "__main__":
	generate_api_json()
