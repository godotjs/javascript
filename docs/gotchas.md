# Gotchas and limitations

Some common mistakes and limitations.

## Import dependency from `node_modules`

If you use `TypeScript` you may encounter the problem where dependencies from `node_modules` are not bundled correctly.

As a workaround you can create a new file `npm-modules.bundle.ts`:

```ts title="npm-modules.bundle.ts"
import { default as dayjs } from "dayjs";
export default { dayjs };
```

In your class you can use the dependency like this:

```ts title="main.ts"
import npm from "./npm-modules.bundle";

export default class Main extends godot.Node {
  _ready(): void {
    console.log(npm.dayjs().toString());
  }
}
```

With a bundler like `esbuild` you should build the `npm-modules.bundle.ts` with the `--bundle` option, but all the other classes like `main.ts` without it.

## Position.x is immutable

You cannot change `this.position.x` try to change `this.position`:

```javascript title="player.mjs"
export default class Player extends godot.KinematicBody2D {
  constructor() {
    super();
    this.direction = new godot.Vector2(1, 0);
  }
  _ready() {}
  _process(delta) {
    this.position.x += this.direction.x; // <- breaks
    this.position += this.direction; // <- works
  }
}
godot.register_property(Player, "direction", new godot.Vector2(1, 0));
```

## ``register_property`` has to be a target

You cannot change `this.position.x` try to change `this.position`:

```javascript title="player.mjs"
export default class Player extends godot.KinematicBody2D {
}
// This works
godot.register_property(Player, "directionWorks", new godot.Vector2(1, 0));
// This breaks because `player` isn't a correct target
godot.register_property(player, "directionBreaks", new godot.Vector2(1, 0));
```
