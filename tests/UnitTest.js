// @ts-nocheck
const TEST_ENTRIES = new Map();


test('godot', typeof godot === 'object', 'core');
test('new Object', () => {
	let obj = new godot.Object();
	let ok = obj && obj instanceof godot.Object;
	obj.free();
	return ok;
}, 'core');

test('new Resource', () => {
	let obj = new godot.Resource();
	let ok = obj && obj.get_class() === 'Resource';
	return ok;
}, 'core');

test('Object.prototype.get_class', () => {
	let obj = new godot.Object();
	let ok = obj.get_class() === 'Object';
	obj.free();
	return ok;
}, 'core')

test('Object.prototype.connect', () => {
	let ok = typeof godot.Object.prototype.connect === 'function';
	if (!ok) return ok;

	let obj = new godot.Object();
	obj.connect('script_changed', (...args)=> {
		console.log(`signal 'script_changed' emited with:`, ...args);
		ok = true;
	});
	obj.emit_signal('script_changed', 123, 'hello');
	obj.free();
	return ok;
}, 'core')


// --------------------------- Unit Test Implementation ------------------------
function test(description, blcok, group = 'default') {
	const entries = TEST_ENTRIES.get(group) || [];
	entries.push({ description, blcok });
	TEST_ENTRIES.set(group, entries);
}
function runEntry(entry) {
	return new Promise((resolve, reject) => {
		switch (typeof (entry.blcok)) {
			case 'boolean':
				return resolve(entry.blcok);
			case 'function': {
				try {
					resolve(entry.blcok());
				} catch (error) {
					console.error(error);
					resolve(false);
				}
			}
			break;
			case 'object':
				if (entry.blcok instanceof Promise) {
					entry.blcok.then(() => {
						resolve(true);
					}).catch(err => {
						resolve(false);
					});
				}
				break;
			default:
				return resolve(false);
		}
	});
}
async function run() {
	let count = 0;
	let passed = 0;
	for (const [group, entries] of TEST_ENTRIES) {
		console.log(`Start test ${group}`);
		count += entries.length;
		let groupCount = 0;
		for (let i = 0; i < entries.length; i++) {
			const entry = entries[i];
			const ret = await runEntry(entry);
			if (ret) {
				passed++;
				groupCount++;
				console.log(`\t[${i + 1}/${entries.length}][ √ ] ${entry.description}`);
			}
			else {
				console.error(`\t[${i + 1}/${entries.length}][ X ] ${entry.description}`);
			}
		}
		const logFunc2 = groupCount == entries.length ? console.log : console.warn;
		logFunc2(`\tTest ${group} finished: ${groupCount}/${entries.length} passed`);
	}
	const logFunc = passed == count ? console.log : console.warn;
	logFunc(`Test complete: ${passed}/${count} passed`);
}
const _ = run(); // QuickJS BUG ? 返回的是一个 Promise 对象，不赋给临时变量，会导致 Promise 对象无法被 GC 
