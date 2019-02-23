#include "ecmascript_language.h"
#include "core/class_db.h"
#include "core/os/file_access.h"

ECMAScriptLanguage *ECMAScriptLanguage::singleton = NULL;

void ECMAScriptLanguage::get_registered_classes(List<Ref<ECMAScript> > &r_list) const {
	for (const StringName *name = script_classes.next(NULL); name; name = script_classes.next(name)) {
		r_list.push_back(script_classes.get(*name));
	}
}

void ECMAScriptLanguage::init() {
	ERR_FAIL_NULL(binding);

	binding->initialize();
}

void ECMAScriptLanguage::finish() {
	binding->uninitialize();
}

Error ECMAScriptLanguage::execute_file(const String &p_path) {
	ERR_FAIL_NULL_V(binding, ERR_BUG);

	FileAccessRef f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V(!f, ERR_INVALID_PARAMETER);

	Vector<uint8_t> buff;
	buff.resize(f->get_len() + 1);
	f->get_buffer(buff.ptrw(), f->get_len());
	buff.ptrw()[buff.size() - 1] = 0;

	String source;
	source.parse_utf8((const char *)buff.ptr(), buff.size());

	return binding->eval_string(source);
}

Error ECMAScriptLanguage::eval_text(const String &p_source) {
	ERR_FAIL_NULL_V(binding, ERR_BUG);
	return binding->eval_string(p_source);
}

void ECMAScriptLanguage::get_reserved_words(List<String> *p_words) const {

	static const char *_reserved_words[] = {
		"abstract",
		"arguments",
		"await",
		"boolean",
		"break",
		"byte",
		"case",
		"catch",
		"char",
		"class",
		"const",
		"continue",
		"debugger",
		"default",
		"delete",
		"do",
		"double",
		"else",
		"enum",
		"eval",
		"export",
		"extends",
		"false",
		"final",
		"finally",
		"float",
		"for",
		"function",
		"goto",
		"if",
		"implements",
		"import",
		"in",
		"instanceof",
		"int",
		"interface",
		"let",
		"long",
		"native",
		"new",
		"null",
		"package",
		"private",
		"protected",
		"public",
		"return",
		"short",
		"static",
		"super",
		"switch",
		"synchronized",
		"this",
		"throw",
		"throws",
		"transient",
		"true",
		"try",
		"typeof",
		"var",
		"void",
		"volatile",
		"while",
		"with",
		"yield",
		0
	};

	const char **w = _reserved_words;

	while (*w) {

		p_words->push_back(*w);
		w++;
	}
}

void ECMAScriptLanguage::get_comment_delimiters(List<String> *p_delimiters) const {
	p_delimiters->push_back("//"); // single-line comment
	p_delimiters->push_back("/* */"); // delimited comment
}

void ECMAScriptLanguage::get_string_delimiters(List<String> *p_delimiters) const {
	p_delimiters->push_back("' '");
	p_delimiters->push_back("\" \"");
	p_delimiters->push_back("` `");
}

void ECMAScriptLanguage::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("js");
}

void *ECMAScriptLanguage::alloc_instance_binding_data(Object *p_object) {
	return binding->alloc_object_binding_data(p_object);
}

void ECMAScriptLanguage::free_instance_binding_data(void *p_data) {
	binding->free_object_binding_data(p_data);
}

void ECMAScriptLanguage::refcount_incremented_instance_binding(Object *p_object) {
	binding->godot_refcount_incremented(static_cast<Reference *>(p_object));
}

bool ECMAScriptLanguage::refcount_decremented_instance_binding(Object *p_object) {
	return binding->godot_refcount_decremented(static_cast<Reference *>(p_object));
}

ECMAScriptLanguage::ECMAScriptLanguage() {

	ERR_FAIL_COND(singleton);
	singleton = this;
	binding = memnew(DuktapeBindingHelper);
}

ECMAScriptLanguage::~ECMAScriptLanguage() {
	memdelete(binding);
}
