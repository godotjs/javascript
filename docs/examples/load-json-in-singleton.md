# Load json in singleton

This example shows how to load a file like a `config` inside a singleton to access it everywhere.

For the `TypeScript` examples, all `.ts` files will be compiled as `.mjs` to the folder `scripts/generated/**`.

> **Note:** If you use `TypeScript` you need to set `"resolveJsonModule": true` inside your `tsconfig.json`.

## 1. Create file

We create a new folder named `config` and a new file `test.json`.

Next we write this to the `test.json`:

```json title="test.json"
{
  "test": true
}
```

## 2. Create the singleton

We create a new file inside our `src` folder like `read-config.(mjs|ts)` and add this code to it:

=== "JavaScript"

    ```javascript title="read-config.mjs"
    import TestJson from "res://config/test.json";

    export default class ReadConfig extends godot.Node {
      static _singleton;

      static get singleton() {
    	return ReadConfig._singleton;
      }

      constructor() {
    	super();
    	if (!ReadConfig._singleton) {
    	  ReadConfig._singleton = this;
    	}
      }

      // This property is available for other classes
      config = TestJson;
    }
    ```

=== "TypeScript"

    ```ts title="read-config.ts"
    // @ts-ignore
    import TestJson from "res://config/test.json";

    type TestType = {
      test: boolean;
    };

    export default class ReadConfig extends godot.Node {
      static _singleton: ReadConfig;

      static get singleton() {
    	return ReadConfig._singleton;
      }

      constructor() {
    	super();
    	if (!ReadConfig._singleton) {
    	  ReadConfig._singleton = this;
    	}
      }

      // This property is available for other classes
      config: TestType = TestJson as TestType;
    }
    ```

## 3. Autoload singleton in project

We need to update the `[autoload]` inside `project.godot`:

```text title="project.godot"
...
[autoload]

; Use the generated `.mjs` file instead of `.ts`
ReadConfig="*res://scripts/generated/read-config.mjs"
...
```

## 4. Use the singleton in other class

In another class e.g. `main.(mjs|ts)` you need to import the `ReadConfig` then you can access every public property and method from `ReadConfig` via `ReadConfig.singleton`:

=== "JavaScript"

    ```ts title="main.mjs"
    import ReadConfig from "./read-config";

    export default class Main extends godot.Node {
      _ready() {
    	console.log(ReadConfig.singleton.config.test); // prints "true"
      }
    }
    ```

=== "TypeScript"

    ```ts title="main.ts"
    import ReadConfig from "./read-config";

    export default class Main extends godot.Node {
      _ready(): void {
    	console.log(ReadConfig.singleton.config.test); // prints "true"
      }
    }
    ```
