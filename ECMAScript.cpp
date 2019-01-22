#include "ECMAScript.h"
#include "core/os/file_access.h"
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

static int native_print(duk_context *ctx) {
	int size = duk_get_top(ctx);
	String msg;
	for (int i = 0; i < size; ++i) {
		msg += duk_to_string(ctx, i);
		if (i < size - 1) {
			msg += " ";
		}
	}
	print_line(msg);
	return 0;
}

void ECMAScriptLanguage::init() {
	ERR_FAIL_COND(this->ctx);
	this->ctx = duk_create_heap(alloc_function, realloc_function, free_function, NULL, fatal_function);
	ERR_FAIL_NULL(this->ctx);

	duk_push_c_function(this->ctx, native_print, DUK_VARARGS);
	duk_put_global_string(this->ctx, "print");

	duk_push_object(this->ctx);
	duk_put_global_string(this->ctx, "godot");

	this->execute_file("test.js");
}

void ECMAScriptLanguage::finish() {
	duk_destroy_heap(this->ctx);
	this->ctx = NULL;
}

Error ECMAScriptLanguage::execute_file(const String &p_path) {
	ERR_FAIL_COND_V(!this->ctx, ERR_BUG);

	FileAccessRef f = FileAccess::open(p_path, FileAccess::READ);
	ERR_FAIL_COND_V(!f, ERR_INVALID_PARAMETER);

	Vector<uint8_t> buff;
	buff.resize(f->get_len() + 1);
	f->get_buffer(buff.ptrw(), f->get_len());
	buff.ptrw()[buff.size() - 1] = 0;

	duk_eval_string(this->ctx, (char *)buff.ptr());

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
	this->ctx = NULL;
}

ECMAScriptLanguage::~ECMAScriptLanguage() {
}
