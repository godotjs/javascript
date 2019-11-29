#include "editor_tools.h"
#include "../ecmascript_language.h"
#include "core/math/expression.h"
#include "editor/filesystem_dock.h"

struct ECMAScriptAlphCompare {
	_FORCE_INLINE_ bool operator()(const Ref<ECMAScript> &l, const Ref<ECMAScript> &r) const {
		return String(l->get_class_name()) < String(r->get_class_name());
	}
};

void ECMAScriptPlugin::_bind_methods() {
	ClassDB::bind_method(D_METHOD("_on_menu_item_pressed"), &ECMAScriptPlugin::_on_menu_item_pressed);
	ClassDB::bind_method(D_METHOD("_export_typescript_declare_file"), &ECMAScriptPlugin::_export_typescript_declare_file);
}

void ECMAScriptPlugin::_on_menu_item_pressed(int item) {
	switch (item) {
		case MenuItem::ITEM_GEN_DECLAR_FILE:
			declaration_file_dialog->popup_centered_ratio();
			break;
	}
}

ECMAScriptPlugin::ECMAScriptPlugin(EditorNode *p_node) {

	PopupMenu *menu = memnew(PopupMenu);
	add_tool_submenu_item(TTR("ECMAScript"), menu);
	menu->add_item(TTR("Generate TypeScript Declaration File"), ITEM_GEN_DECLAR_FILE);
	menu->connect("id_pressed", this, "_on_menu_item_pressed");

	declaration_file_dialog = memnew(EditorFileDialog);
	declaration_file_dialog->set_title(TTR("Generate TypeScript Declaration File"));
	declaration_file_dialog->set_mode(EditorFileDialog::MODE_SAVE_FILE);
	declaration_file_dialog->set_access(EditorFileDialog::ACCESS_FILESYSTEM);
	declaration_file_dialog->add_filter(TTR("*.d.ts;TypeScript Declaration file"));
	declaration_file_dialog->set_current_file("godot.d.ts");
	declaration_file_dialog->connect("file_selected", this, "_export_typescript_declare_file");
	EditorNode::get_singleton()->get_gui_base()->add_child(declaration_file_dialog);
}

static String applay_partern(const String &p_partern, const Dictionary &p_values) {
	String ret = p_partern;
	for (const Variant *key = p_values.next(); key; key = p_values.next(key)) {
		String p = String("${") + String(*key) + "}";
		String v = p_values.get(*key, p);
		ret = ret.replace(p, v);
	}
	return ret;
}

static String format_doc_text(const String &p_source, const String &p_indent = "\t") {
	Dictionary dict;
	dict["[code]"] = "`";
	dict["[/code]"] = "`";
	dict["[codeblock]"] = "```gdscript";
	dict["[/codeblock]"] = "```";

	Vector<String> lines = p_source.split("\n");
	Vector<String> text_lines;
	for (int i = 0; i < lines.size(); i++) {
		String line = lines[i].strip_edges();
		if (line.empty()) continue;
		text_lines.push_back(line);
	}

	String ret = "";
	for (int i = 0; i < text_lines.size(); i++) {
		if (i > 0) {
			ret += p_indent;
		}
		ret += text_lines[i];
		if ((i < text_lines.size() - 1)) {
			ret += "  \n";
		}
	}

	for (const Variant *key = dict.next(); key; key = dict.next(key)) {
		String p = *key;
		String v = dict.get(*key, p);
		ret = ret.replace(p, v);
	}
	return ret;
}

static String format_identifier(const String &p_ident) {
	List<String> reserved_words;
	ECMAScriptLanguage::get_singleton()->get_reserved_words(&reserved_words);
	if (reserved_words.find(p_ident)) {
		return String("p_") + p_ident;
	}
	return p_ident;
}

static String format_property_name(const String &p_ident) {
	List<String> reserved_words;
	ECMAScriptLanguage::get_singleton()->get_reserved_words(&reserved_words);
	if (reserved_words.find(p_ident) || p_ident.find("/") != -1) {
		return String("'") + p_ident + "'";
	}
	return p_ident;
}

static String get_type_name(const String &p_type) {
	if (p_type.empty())
		return "void";
	if (p_type == "int" || p_type == "float")
		return "number";
	if (p_type == "bool")
		return "boolean";
	if (p_type == "String" || p_type == "NodePath")
		return "string";
	if (p_type == "Array")
		return "any[]";
	if (p_type == "Dictionary")
		return "object";
	if (p_type == "Variant")
		return "any";
	if (Engine::get_singleton()->has_singleton(p_type))
		return String("_") + p_type;
	return p_type;
}

String _export_method(const DocData::MethodDoc &p_method, bool is_function = false) {

	String method_template = ""
							 "\n"
							 "		/** ${description} */"
							 "\n"
							 "		${name}(${params}) : ${return_type};"
							 "\n";
	if (is_function) {
		method_template = ""
						  "\n"
						  "	/** ${description} */"
						  "\n"
						  "	function ${name}(${params}) : ${return_type};"
						  "\n";
	}

	Dictionary dict;
	dict["description"] = format_doc_text(p_method.description, "\t\t ");
	dict["name"] = format_property_name(p_method.name);
	dict["return_type"] = get_type_name(p_method.return_type);
	String params = "";
	bool arg_default_value_started = false;
	for (int i = 0; i < p_method.arguments.size(); i++) {
		const DocData::ArgumentDoc &arg = p_method.arguments[i];
		if (!arg_default_value_started && !arg.default_value.empty()) {
			arg_default_value_started = true;
		}
		String arg_str = format_identifier(arg.name) + (arg_default_value_started ? "?: " : ": ") + get_type_name(arg.type);
		if (i < p_method.arguments.size() - 1) {
			arg_str += ", ";
		}
		params += arg_str;
	}
	if (p_method.qualifiers.find("vararg") != -1) {
		params += params.empty() ? "...args" : ", ...args";
	}
	dict["params"] = params;
	return applay_partern(method_template, dict);
	;
}

String _export_class(const DocData::ClassDoc &class_doc) {

	const String class_template = ""
								  "\n"
								  ""
								  "\n"
								  "	namespace ${name} {"
								  "\n"
								  "		interface Signal${extends}${base_signal} {"
								  "\n"
								  "${signals}"
								  "		}"
								  "\n"
								  "	}"
								  "\n"
								  ""
								  "\n"
								  "	/** ${brief_description}"
								  "\n"
								  ""
								  "\n"
								  "	 ${description} */"
								  "\n"
								  "	class ${name}${extends}${inherits} {"
								  "\n"
								  ""
								  "\n"
								  "		static readonly Signal: ${name}.Signal;"
								  "\n"
								  "		readonly Signal: ${name}.Signal;"
								  "\n"
								  "${constants}"
								  "\n"
								  "${properties}"
								  "\n"
								  "${methods}"
								  "\n"
								  "	}"
								  "\n";
	Dictionary dict;
	dict["name"] = class_doc.name;
	dict["inherits"] = class_doc.inherits.empty() ? "" : get_type_name(class_doc.inherits);
	dict["base_signal"] = class_doc.inherits.empty() ? "" : get_type_name(class_doc.inherits) + ".Signal";
	dict["extends"] = class_doc.inherits.empty() ? "" : " extends ";
	String brief_description = format_doc_text(class_doc.brief_description, "\t ");
	dict["brief_description"] = brief_description;
	String description = format_doc_text(class_doc.description, "\t ");
	if (description.length() == brief_description.length() && description == brief_description) {
		dict["description"] = "";
	} else {
		dict["description"] = description;
	}

	String constants = "";
	for (int i = 0; i < class_doc.constants.size(); i++) {
		const DocData::ConstantDoc &const_doc = class_doc.constants[i];
		Dictionary dict;
		dict["description"] = format_doc_text(const_doc.description, "\t\t ");
		dict["name"] = format_property_name(const_doc.name);
		dict["value"] = const_doc.value;
		String type = "number";
		if (const_doc.value.find("(") != -1) {
			type = const_doc.value.split("(")[0];
		}
		dict["type"] = type;

		if (class_doc.name.begins_with("_")) {
			String signleton_const_str = ""
										 "\n"
										 "		/** ${description}"
										 "\n"
										 "		 * @value `${value}`"
										 "\n"
										 "		 */"
										 "\n"
										 "		readonly ${name}: ${type};"
										 "\n";
			constants += applay_partern(signleton_const_str, dict);
		} else {
			String const_str = ""
							   "\n"
							   "		/** ${description}"
							   "\n"
							   "		 * @value `${value}`"
							   "\n"
							   "		 */"
							   "\n"
							   "		static readonly ${name}: ${type};"
							   "\n";
			constants += applay_partern(const_str, dict);
		}
	}
	dict["constants"] = constants;

	Vector<DocData::MethodDoc> method_list = class_doc.methods;
	String properties = "";
	for (int i = 0; i < class_doc.properties.size(); i++) {
		const DocData::PropertyDoc &prop_doc = class_doc.properties[i];

		String prop_str = ""
						  "\n"
						  "		/** ${description} */"
						  "\n"
						  "		${name}: ${type};"
						  "\n";
		Dictionary dict;
		dict["description"] = format_doc_text(prop_doc.description, "\t\t ");
		dict["name"] = format_property_name(prop_doc.name);
		dict["type"] = get_type_name(prop_doc.type);
		properties += applay_partern(prop_str, dict);

		if (!prop_doc.getter.empty()) {
			DocData::MethodDoc md;
			md.name = prop_doc.getter;
			md.return_type = get_type_name(prop_doc.type);
			md.description = String("Getter of `") + prop_doc.name + "` property";
			method_list.push_back(md);
		}

		if (!prop_doc.setter.empty()) {
			DocData::MethodDoc md;
			md.name = prop_doc.setter;
			DocData::ArgumentDoc arg;
			arg.name = "p_value";
			arg.type = get_type_name(prop_doc.type);
			md.arguments.push_back(arg);
			md.return_type = "void";
			md.description = String("Setter of `") + prop_doc.name + "` property";
			method_list.push_back(md);
		}
	}
	dict["properties"] = properties;

	String signals = "";
	for (int i = 0; i < class_doc.signals.size(); ++i) {
		const DocData::MethodDoc &signal = class_doc.signals[i];
		String signal_str = ""
							"\n"
							"			/** ${description} */"
							"\n"
							"			${name}: '${name}',"
							"\n";
		Dictionary dict;
		dict["description"] = format_doc_text(signal.description, "\t\t\t ");
		dict["name"] = signal.name;
		signals += applay_partern(signal_str, dict);
	}
	dict["signals"] = signals;

	// TODO: Theme properties

	String methods = "";
	for (int i = 0; i < method_list.size(); i++) {
		const DocData::MethodDoc &method_doc = method_list[i];
		if (method_doc.name == class_doc.name) {
			continue;
		}
		methods += _export_method(method_doc);
	}
	dict["methods"] = methods;

	return applay_partern(class_template, dict);
}

void ECMAScriptPlugin::_export_typescript_declare_file(const String &p_path) {
	DocData *doc = EditorHelp::get_doc_data();
	Dictionary dict;

	const String godot_module = "// This file is generated by godot editor"
								"\n"
								"${buitins}"
								"\n"
								""
								"\n"
								"declare module " GODOT_OBJECT_NAME " {"
								"\n"
								"${singletons}"
								"\n"
								"${constants}"
								"\n"
								"${functions}"
								"\n"
								"${classes}"
								"\n"
								"}"
								"\n"
								""
								"\n";

	String classes = "";
	Set<String> ignored_classes;
	ignored_classes.insert("int");
	ignored_classes.insert("float");
	ignored_classes.insert("bool");
	ignored_classes.insert("String");
	ignored_classes.insert("Nil");
	ignored_classes.insert("Variant");
	ignored_classes.insert("Array");
	ignored_classes.insert("Dictionary");
	ignored_classes.insert("Vector2");
	ignored_classes.insert("Vector3");
	ignored_classes.insert("Color");
	ignored_classes.insert("Rect2");
	ignored_classes.insert("RID");
	ignored_classes.insert("NodePath");
	ignored_classes.insert("Transform2D");
	ignored_classes.insert("Basis");
	ignored_classes.insert("Quat");
	ignored_classes.insert("Plane");
	ignored_classes.insert("AABB");
	ignored_classes.insert("Transform");
	ignored_classes.insert("PoolByteArray");
	ignored_classes.insert("PoolIntArray");
	ignored_classes.insert("PoolRealArray");
	ignored_classes.insert("PoolStringArray");
	ignored_classes.insert("PoolVector2Array");
	ignored_classes.insert("PoolVector3Array");
	ignored_classes.insert("PoolColorArray");

	String constants = "";

	for (Map<String, DocData::ClassDoc>::Element *E = doc->class_list.front(); E; E = E->next()) {
		DocData::ClassDoc class_doc = E->get();
		if (ignored_classes.has(class_doc.name)) {
			continue;
		}
		class_doc.name = get_type_name(class_doc.name);
		if (class_doc.name.begins_with("@")) {
			if (class_doc.name == "@GlobalScope" || class_doc.name == "@GDScript") {
				for (int i = 0; i < class_doc.constants.size(); i++) {
					const DocData::ConstantDoc &const_doc = class_doc.constants[i];
					String const_str = ""
									   "\n"
									   "	/** ${description}"
									   "\n"
									   "	 * @value `${value}`"
									   "\n"
									   "	 */"
									   "\n"
									   "	const ${name}: number;"
									   "\n";
					Dictionary dict;
					dict["description"] = format_doc_text(const_doc.description, "\t ");
					dict["name"] = format_property_name(const_doc.name);
					dict["value"] = const_doc.value;
					constants += applay_partern(const_str, dict);
					;
				}
			}

			if (class_doc.name == "@GlobalScope") {
				String singletons = "";
				for (int i = 0; i < class_doc.properties.size(); i++) {
					const DocData::PropertyDoc &prop_doc = class_doc.properties[i];

					String prop_str = ""
									  "\n"
									  "	/** ${description} */"
									  "\n"
									  "	const ${name}: ${type};"
									  "\n";
					Dictionary dict;
					dict["description"] = format_doc_text(prop_doc.description, "\t\t ");
					dict["name"] = format_property_name(prop_doc.name);
					dict["type"] = get_type_name(prop_doc.type);
					singletons += applay_partern(prop_str, dict);
				}
				dict["singletons"] = singletons;
				dict["constants"] = constants;
			} else if (class_doc.name == "@GDScript") {
				String methods = "";
				for (int i = 0; i < class_doc.methods.size(); i++) {
					const DocData::MethodDoc &method_doc = class_doc.methods[i];
					if (Expression::find_function(method_doc.name) == Expression::FUNC_MAX) {
						continue;
					}
					if (format_property_name(method_doc.name) != method_doc.name) {
						continue;
					}
					methods += _export_method(method_doc, true);
				}
				dict["functions"] = methods;
			}
			continue;
		}
		classes += _export_class(class_doc);
	}
	dict["constants"] = constants;
	dict["classes"] = classes;
	dict["buitins"] = BUILTIN_DECLEARATION_TEXT;

	String text = applay_partern(godot_module, dict);
	FileAccessRef f = FileAccess::open(p_path, FileAccess::WRITE);
	if (f.f && f->is_open()) {
		f->store_string(text);
		f->close();
	}
}
