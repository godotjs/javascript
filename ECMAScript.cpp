#include "ECMAScript.h"
#include "core/os/memory.h"
#include "core/typedefs.h"

/************* SCRIPT LANGUAGE **************/
ECMAScriptLanguage *ECMAScriptLanguage::singleton = NULL;

static void *alloc_function(void *udata, duk_size_t size) {
	return memalloc(size);
}

static void *realloc_function(void *udata, void *ptr, duk_size_t size) {
	return memrealloc(ptr, size);
}

static void free_function(void *udata, void *ptr) {
	if (ptr) {
		memfree(ptr);
	}
}

static void fatal_function(void *udata, const char *msg) {
	ERR_EXPLAIN(msg);
}

void ECMAScriptLanguage::init() {
	ERR_FAIL_COND(this->ctx);
	this->ctx = duk_create_heap(alloc_function, realloc_function, free_function, NULL, fatal_function);
	ERR_FAIL_NULL(this->ctx);
}

void ECMAScriptLanguage::finish() {
	duk_destroy_heap(this->ctx);
	this->ctx = NULL;
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
	this->ctx = NULL;
}

ECMAScriptLanguage::~ECMAScriptLanguage() {
}
