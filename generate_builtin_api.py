#!/usr/bin/env python
import json, os
import xml.etree.ElementTree as ET

DOCS_DIR = "../../godot/doc/classes"

BUILTIN_CLASSES = [
	'Vector2',
	# 'Vector3',
	# 'Color',
]

TYPE_MAP = {
	'int': 'number',
	'float': 'number',
	'bool': 'boolean',
	'String': 'string',
}

def parse_class(cls):
	ret = {'name': cls.get('name')}
	members = []
	methods = []
	constants = []
	ret['properties'] = members
	ret['methods'] = methods
	ret['constants'] = constants
	
	for m in (cls.find("members") if cls.find("members") is not None else []):
		m_dict = dict(m.attrib)
		type = m_dict['type']
		if type in TYPE_MAP:
			type = TYPE_MAP[type]
		members.append({'name': m_dict['name'], 'type': type })
	
	for m in (cls.find("methods") if cls.find("methods") is not None else []):
		m_dict = dict(m.attrib)
		if m_dict['name'] == cls.get('name'):
			continue
		return_type = m.find("return").attrib["type"]
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
			'name': m_dict['name'],
			'native_method': m_dict['name'],
			'return': return_type,
			'arguments': arguments,
		})
	# add equals method
	methods.append({
		"arguments": [
			{
				"default_value": None,
				"has_default_value": False,
				"type": cls.get('name')
			}
		],
		"name": "equals",
		"native_method": "operator==",
		"return": "boolean"
	})
	for c in (cls.find("constants") if cls.find("constants") is not None else []):
		constants.append(dict(c.attrib))
	return ret

def generate_api_json():
	classes = []
	for cls in BUILTIN_CLASSES:
		tree = ET.parse(open(os.path.join(DOCS_DIR, cls + '.xml'), 'r'))
		data = tree.getroot()
		classes.append(parse_class(data))
	json.dump(classes, open('buitin_api.gen.json', 'w', encoding='utf8'), ensure_ascii=False, indent=2, sort_keys=True)
	print('Done')
	
if __name__ == "__main__":
	generate_api_json()
