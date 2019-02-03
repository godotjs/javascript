/*************************************************************************/
/*  register_types.cpp                                                   */
/*************************************************************************/
/*                       This file is part of:                           */
/*                           GODOT ENGINE                                */
/*                      https://godotengine.org                          */
/*************************************************************************/
/* Copyright (c) 2007-2019 Juan Linietsky, Ariel Manzur.                 */
/* Copyright (c) 2014-2019 Godot Engine contributors (cf. AUTHORS.md)    */
/*                                                                       */
/* Permission is hereby granted, free of charge, to any person obtaining */
/* a copy of this software and associated documentation files (the       */
/* "Software"), to deal in the Software without restriction, including   */
/* without limitation the rights to use, copy, modify, merge, publish,   */
/* distribute, sublicense, and/or sell copies of the Software, and to    */
/* permit persons to whom the Software is furnished to do so, subject to */
/* the following conditions:                                             */
/*                                                                       */
/* The above copyright notice and this permission notice shall be        */
/* included in all copies or substantial portions of the Software.       */
/*                                                                       */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,       */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF    */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.*/
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY  */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE     */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                */
/*************************************************************************/

#include "register_types.h"
#include "ecmascript.h"
#include "ecmascript_language.h"
#include "ecmascript_library.h"

Ref<ECMAScriptLibraryResourceLoader> resource_loader_ecmalib;
Ref<ECMAScriptLibraryResourceSaver> resource_saver_ecmalib;
Ref<ResourceFormatLoaderECMAScript> resource_loader_ecmascript;
Ref<ResourceFormatSaverECMAScript> resource_saver_ecmascript;

static ECMAScriptLanguage *script_language_es = NULL;

void register_ECMAScript_types() {

	ClassDB::register_class<ECMAScriptLibrary>();
	ClassDB::register_class<ECMAScript>();

	resource_loader_ecmalib.instance();
	resource_saver_ecmalib.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_ecmalib, true);
	ResourceSaver::add_resource_format_saver(resource_saver_ecmalib, true);

	resource_loader_ecmascript.instance();
	resource_saver_ecmascript.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_ecmascript, true);
	ResourceSaver::add_resource_format_saver(resource_saver_ecmascript, true);

	script_language_es = memnew(ECMAScriptLanguage);
	script_language_es->set_language_index(ScriptServer::get_language_count());
	ScriptServer::register_language(script_language_es);
}

void unregister_ECMAScript_types() {
	ScriptServer::unregister_language(script_language_es);
	memdelete(script_language_es);

	ResourceLoader::remove_resource_format_loader(resource_loader_ecmalib);
	ResourceSaver::remove_resource_format_saver(resource_saver_ecmalib);
	resource_loader_ecmalib.unref();
	resource_saver_ecmalib.unref();

	ResourceLoader::remove_resource_format_loader(resource_loader_ecmascript);
	ResourceSaver::remove_resource_format_saver(resource_saver_ecmascript);
	resource_loader_ecmascript.unref();
	resource_saver_ecmascript.unref();
}
