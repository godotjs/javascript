## JavaScript language binding for godot game engine

This module implements the ECMAScript 5.1 script language support for godot game engine.

[Duktape 2.3](https://duktape.org/) is used as the ECMAScript engine. It is possible to replace it with other JIT-supported engine but not in the near plan. 

**This project is still in development so it may contains bugs.**

The documentation is lack because of my pool English skill. Contributions to documentation is welcomed!


## Documentation

### Compile
* Clone the source code of [godot](https://github.com/godotengine/godot)
* Clone this module and put it into `godot/modules/` make sure the folder name of this module is `ECMAScript`
* [Recompile godot engine](https://docs.godotengine.org/en/3.0/development/compiling/index.html)

### Usage

#### Generate TypeScript declearation file

In the editor menu `Project > Tools > ECMAScript > Generate TypeScript Declaration`.

#### Attach Classe defined in ECMAScript to Node/Object

Drag the class item in the `ECMAScript` panel at bottom of the editor to the target


## Demo

You can try demos in the [ECMAScriptDemos](https://github.com/Geequlim/ECMAScriptDemos)

## TODO:
* Implement debugger server.
* The way to do tasks in another thread
