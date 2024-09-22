// @ts-nocheck
/**
 * All Tests
 * @type {Map<string, {title: string, block: function}[]>}
 */
const TEST_ENTRIES = new Map();

test("godot", () => typeof godot === "object", "core");
test(
  "new Object",
  () => {
    let obj = new godot.Object();
    let ok = obj && obj instanceof godot.Object;
    obj.free();
    return ok;
  },
  "core",
);

test(
  "new Resource",
  () => {
    let obj = new godot.Resource();
    let ok = obj && obj.get_class() === "Resource";
    return ok;
  },
  "core",
);

test(
  "Object.prototype.get_class",
  () => {
    let obj = new godot.Object();
    let ok = obj.get_class() === "Object";
    obj.free();
    return ok;
  },
  "core",
);

test(
  "Object.prototype.connect",
  () => {
    let ok = typeof godot.Object.prototype.connect === "function";
    if (!ok) return ok;

    let obj = new godot.Object();
    obj.connect("script_changed", (...args) => {
      const [firstNumber, secondString] = args;
      ok = firstNumber === 123 && secondString === "hello";
    });
    obj.emit_signal("script_changed", 123, "hello");
    obj.free();
    return ok;
  },
  "core",
);

// --------------------------- Unit Test Implementation ------------------------

/**
 * Add test to runner
 * @param title {string}
 * @param block {function}
 * @param group {string}
 */
function test(title, block, group = "default") {
  const entries = TEST_ENTRIES.get(group) || [];
  entries.push({ title, block });
  TEST_ENTRIES.set(group, entries);
}

function run() {
  let count = 0;
  let passed = 0;
  for (const [group, entries] of TEST_ENTRIES) {
    console.log(`Start test ${group}`);
    count += entries.length;
    for (const { description, block } of entries) {
      try {
        const ok = block();
        if (ok) {
          passed++;
        }
      } catch (e) {
        console.error(description, e);
      }
    }
  }

  const ok = passed === count;
  const logFunc = ok ? console.log : console.warn;
  logFunc(`Test complete: ${passed}/${count} passed`);
  if (!ok) {
    throw "JavaScript unit tests have failed";
  }
}

run();
