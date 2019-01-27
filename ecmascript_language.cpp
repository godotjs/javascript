#include "ecmascript_language.h"
#include "core/os/file_access.h"

#include "core/class_db.h"

ECMAScriptLanguage *ECMAScriptLanguage::singleton = NULL;

void ECMAScriptLanguage::init() {
	ERR_FAIL_NULL(this->binding);

	this->binding->initialize();

	this->execute_file("test.js");
}

void ECMAScriptLanguage::finish() {
	this->binding->uninitialize();
}

Error ECMAScriptLanguage::execute_file(const String &p_path) {
	ERR_FAIL_NULL_V(this->binding, ERR_BUG);

	FileAccessRef f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V(!f, ERR_INVALID_PARAMETER);

	Vector<uint8_t> buff;
	buff.resize(f->get_len() + 1);
	f->get_buffer(buff.ptrw(), f->get_len());
	buff.ptrw()[buff.size() - 1] = 0;

	duk_eval_string(this->binding->get_context(), (char *)buff.ptr());
	return OK;
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

ECMAScriptLanguage::ECMAScriptLanguage() {

	ERR_FAIL_COND(singleton);
	singleton = this;
	binding = memnew(DuktapeBindingHelper);
}

ECMAScriptLanguage::~ECMAScriptLanguage() {
	memdelete(this->binding);
}
