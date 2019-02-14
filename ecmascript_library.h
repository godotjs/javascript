#ifndef ECMASCRIPT_LIBRARY_H
#define ECMASCRIPT_LIBRARY_H

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "scene/resources/text_file.h"

class ECMAScriptLibrary : public TextFile {
	GDCLASS(ECMAScriptLibrary, TextFile)
protected:
	static void _bind_methods() {}
};

class ECMAScriptLibraryResourceLoader : public ResourceFormatLoader {
	GDCLASS(ECMAScriptLibraryResourceLoader, ResourceFormatLoader)
public:
	virtual RES load(const String &p_path, const String &p_original_path, Error *r_error);
	virtual void get_recognized_extensions(List<String> *p_extensions) const;
	virtual void get_recognized_extensions_for_type(const String &p_type, List<String> *p_extensions) const;
	virtual bool handles_type(const String &p_type) const;
	virtual String get_resource_type(const String &p_path) const;
};

class ECMAScriptLibraryResourceSaver : public ResourceFormatSaver {
	GDCLASS(ECMAScriptLibraryResourceSaver, ResourceFormatSaver)
public:
	virtual Error save(const String &p_path, const RES &p_resource, uint32_t p_flags = 0);
	virtual bool recognize(const RES &p_resource) const;
	virtual void get_recognized_extensions(const RES &p_resource, List<String> *p_extensions) const;
};

#endif // ECMASCRIPT_LIBRARY_H
