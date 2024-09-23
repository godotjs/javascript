# JavaScriptLanguage

This class is a singleton which acts as a bridge between JavaScript (QuickJSBinder) and Godot Editor.

It handles how the language is structured and how script files should be reloaded and created.

## Unimplemented features

> **Note:** Some features aren't fully implemented yet (marked with "\*\_todo\_\*".cpp)

> - debugger
> - doctools
> - multithread
> - profiling
> - texteditor

## Editor templates

Furthermore, the singleton handles creation of new files via templates.

The templates will be generated.
To update or include new ones look at `misc/generate/templates.py` and `misc/template-contents/**`.

The plain JS methods will be located inside `misc/template-contents/**`.
You need to use `export default class CLASS extends GODOT_OBJECT_NAME.BASE` as class definition, those will be replaced on generation.

Inside `misc/generate/templates.py` ou need to add a new template where you need to add some template name and description.

If you want to add a new template you need to add the variable as well inside `src/language/templates/javascript_template_manager.h`, like
`static ScriptLanguage::ScriptTemplate DEFAULT_TEMPLATE;`.
And you need to append it into the `get_templates` function inside `src/language/templates/javascript_template_manager.cpp`.
