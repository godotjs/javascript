# Use GDScript in TypeScript

This example shows how to use a class written in GDScript in another TS class.

We use ``TypeScript`` for this example all `.ts` files will be compiled as `.mjs` to the folder `scripts/generated/**`.

## 1. Create the GDScript file

First we create a simple class with GDScript inside a new file `scripts/gd/GDTest.gd`:

````gdscript title="GDTest.gd"
extends Node

func _ready():
	pass

func print_in_gd_test():
	print("bla")

````

## 2. Create a declaration (*.d.ts) for GDScript

For proper TypeScript support we need to add a ``gdtest.d.ts`` file:

````ts title="gdtest.d.ts"
declare module "res://scripts/gd/GDTest.gd" {
  class GDTest {
    call(func: "print_in_gd_test"): void;

    static new() {
      return this;
    }
  }
  export = GDTest;
}
````

## 3. Use the class inside your TS file

In the end we need to call the ``GDTest.gd`` from another `.ts` file, like `main.ts`:

````ts title="main.ts"
import GDTest from "res://scripts/gd/GDTest.gd";

export default class Main extends godot.Node {
	_ready(): void {
		const bla: Bla = Bla.new();
		bla.call("run_in_bla");
	}
}

````

> **Note:** The important thing here is that you use `new()` to instantiate and `call` to execute the function
