//@ts-ignore
declare module globalThis {
	/**
	 * A long integer value, the request id, that uniquely identifies the entry in the callback list.
	 * This is a non-zero value, but you may not make any other assumptions about its value.
	 * You can pass this value to `godot.cancelAnimationFrame()` to cancel the refresh callback request.
	 */
	type FrameRequetID = number;

	/**
	 * Request a refresh callback request, the `callback` will be called every frame
	 * @param callback The function to call when it's time to update your animation for the next repaint. The callback function is passed one single argument, a number similar to the one returned by `godot.OS.get_system_time_msecs()`, indicating the point in time when requestAnimationFrame() starts to execute callback functions.
	 */
	function requestAnimationFrame(callback: (time_stamp: number) => void): FrameRequetID;

	/**
	 * Cancel an frame request previously scheduled through a call to `godot.requestAnimationFrame()`.
	 * @param request_id The ID value returned by the call to `godot.requestAnimationFrame()` that requested the callback.
	 */
	function cancelAnimationFrame(request_id: FrameRequetID): void;

	/**
	 * The Console API provides functionality to allow developers to perform debugging tasks, such as logging messages or the values of variables at set points in your code, or timing how long an operation takes to complete.
	 */
	//@ts-ignore
	const console: {
		/**
		 * Outputs a message to the console. The message may be a single string (with optional substitution values), or it may be any one or more JavaScript objects.
		 * @param message A list of JavaScript objects to output. The string representations of each of these objects are appended together in the order listed and output.
		 */
		log(...message): void;

		/**
		 * Outputs a warning message to the console.
		 * @param message  list of JavaScript objects to output. The string representations of each of these objects are appended together in the order listed and output.
		 */
		warn(...message): void;

		/**
		 * Outputs an error message to the console.
		 * @param message A list of JavaScript objects to output. The string representations of each of these objects are appended together in the order listed and output.
		 */
		error(...message): void;

		/** Outputs a stack trace to the console.
		 * @param message A list of JavaScript objects to output. The string representations of each of these objects are appended together in the order listed and output.
		*/
		trace(...message): void;

		/** Log JavaScript Objects as JSON format */
		LOG_OBJECT_TO_JSON: boolean;
	}

	/**
	 * A worker is an object created using a constructor of `Worker` that runs a named JavaScript file — this file contains the code that will run in the worker thread;
	 *
	 * Workers run in another global context that is different from the current context.
	 *
	 * You can run whatever code you like inside the worker thread. All of the godot API are available inside workers.
	 *
	 * Data is sent between workers and the main thread via a system of messages — both sides send their messages using the `postMessage()` method, and respond to messages via the `onmessage` event handler (the message is contained within the Message event's data attribute.) The data is copied rather than shared.
	 *
	 * You can **transfer** value with `Worker.abandonValue` and `Worker.adoptValue`. After a value is abandoned you cannot using it anymore in the context.
	 *
	 * Workers may, in turn, spawn new workers, all sub-worker will be stopped when the host context stop.
	 */
	//@ts-ignore
	class Worker {

		/**
		 * Creates a dedicated worker thread that executes the script at the specified file
		 */
		constructor(script: string);

		/**
		 * The `onmessage` property of the Worker interface represents an event handler, that is a function to be called when the message event occurs.
		 * It will be called when the worker's parent receives a message from the worker context by `postMessage` method.
		 */
		onmessage(message: any): void;

		/**
		 * Sends a message to the worker's inner scope. This accepts a single parameter, which is the data to send to the worker.
		 * @param message The object to deliver to the worker; this will be in the data field in the event delivered to the `onmessage` handler.
		 * @note The data cannot be instance of `godot.Object` or any other JavaScript object contains functions.
		 */
		postMessage(message: any): void;

		/**
		 * Stop the worker thread
		 */
		terminate(): void;
	}

	/** **Worker context only**
	 *
	 *  Stop the worker thread of current context
	 */
	function close(): void;

	/** **Worker context only**
	 *
	 * The message handler to handle messages send from the host context
	 */
	//@ts-ignore
	function onmessage(message: any): void;

	/** **Worker context only**
	 *
	 * Sends a message to the host thread context that spawned it.
	 *
	 * @param {*} message The message to send
	 */
	function postMessage(message: any): void;

	/** **Worker context only**
	 *
	 * Synchronously load and run one or more scripts in the worker thread.
	 */
	function importScripts(...scripts: string[]): void;

	/** **Worker context only**
	 *
	 * The flag is `true` if current context is inside a worker thread.
	 */
	const INSIDE_WORKER: true | undefined;
}

declare module godot {

	type GodotClass = new() => godot.Object;

	interface PropertyInfo {
		type?: VariantType;
		name?: string;
		hint?: PropertyHint;
		/**@see PropertyHint */
		hint_string?: string;
		/** @see PropertyUsageFlags */
		usage?: number;
		/** Default value of the property */
		default?: any;
	}

	/**
	 * Export class to godot
	 *
	 * @param target The class extends from `godot.Object`
	 * @param name The class name
	 */
	function register_class(target: GodotClass, name: string);

	/**
	 * Register signal to class
	 * @param target The class of the signal
	 * @param name signal name
	 */
	function register_signal(target: GodotClass | godot.Object, name: string);

	/**
	 * Register property to class
	 * @param target The class of the property
	 * @param name The name of the property
	 * @param value The default value of the property
	 */
	function register_property(target: GodotClass | godot.Object, name: string, value: any);

	/**
	 * The meta data of an script
	 * @param target The script class
	 * @param tool is tooled of this class
	 */
	function set_script_tooled(target: GodotClass, tool: boolean);

	/**
	 * The meta data of an script
	 * @param target The script class
	 * @param icon The icon of the class
	 */
	function set_script_icon(target: GodotClass, icon: string);

	/**
	 * Returns the internal type of the given `Variant` object, using the `godot.TYPE_*`
	 */
	function get_type(val: any): number;

	/**
	 * Loads a resource from the filesystem located at `path`.
	 *
	 * **Note:** Resource paths can be obtained by right-clicking on a resource in the FileSystem dock and choosing **Copy Path**.
	 * ```
	 * // Load a scene called main located in the root of the project directory
	 * const main = godot.load("res://main.tscn")
	 * ```*/
	function load(path: string): Resource;

	/**
	 * Returns the Object that corresponds to `instance_id`. All Objects have a unique instance ID.
	 */
	function instance_from_id(instance_id: number): Object;

	/**
	 * Drop the `value` in the context. You should never touching an abandoned value anymore before it is adopted.
	 *
	 * You can adopt the value in another thread context to transfer the `value` in threads.
	 *
	 * A `non zero` ID is return if no error happen.
	 * @param value The value to abandon in the context
	 * @note You can only transfer values the godot `Variant` can represent.
	 * @returns The ID of the abandoned value
	*/
	function abandon_value(value: any): number;

	/**
	* Adopt an abandoned value.
	* @param value_id The ID of the abandoned value
	*/
	function adopt_value(value_id: number): any;

	/**
	 * Wait a signal of an object
	 * @param target The owner of the signal to wait
	 * @param signal The signal to wait
	 */
	function yield(target: godot.Object, signal: string): Promise<any[]>;

	const E: 2.7182818284590452353602874714;
	const LN2: 0.6931471805599453094172321215;
	const SQRT2: 1.4142135623730950488016887242;
	const SQRT12: 0.7071067811865475244008443621048490;

	/** The flag is `true` if current binary is compiled with `target=debug` or `target=release_debug` */
	const DEBUG_ENABLED: boolean;

	/** The flag is `true` if current binary is godot editor which is compiled with `tool=yes` */
	const TOOLS_ENABLED: boolean;

	/** The flag is `true` if current binary enable debug method information */
	const DEBUG_METHODS_ENABLED: boolean;
}
