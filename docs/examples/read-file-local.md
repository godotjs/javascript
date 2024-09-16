# Read file local

This example shows how to load any file as string from a local folder.

For the `TypeScript` examples, all `.ts` files will be compiled as `.mjs` to the folder `scripts/generated/**`.

## 1. Create the file you want to read

In this example we try to read a ``.csv`` file, but it should work with any other file as well.

We create a folder ``resources`` and add a new file `test.csv`:

````csv title="test.csv"
keys,en,de
HELLO,hello,hallo
````

## 2. Read the file in class

We use [FileAccess](https://docs.godotengine.org/en/stable/classes/class_fileaccess.html) to read the file.

We create a new file ``read-local-file.(mjs|ts)``:

=== "JavaScript"

    ````ts title="read-local-file.mjs"
    export default class ReadLocalFile extends godot.Node {
        _ready() {
            const file = new godot.FileAccess();
            file.open("res://resources/test.csv", godot.FileAccess.ModeFlags.READ);
            let fileContent = "";
            while (!file.eof_reached()) {
                fileContent += `${file.get_line()}\n`;
            }
            console.log(fileContent);
        }
    }
    ````
=== "TypeScript"

    ````ts title="read-local-file.ts"
    export default class ReadLocalFile extends godot.Node {
        _ready(): void {
            const file = new godot.FileAccess();
            file.open("res://resources/test.csv", godot.FileAccess.ModeFlags.READ);
            let fileContent: string = "";
            while (!file.eof_reached()) {
                fileContent += `${file.get_line()}\n`;
            }
            console.log(fileContent);
        }
    }
    ````

