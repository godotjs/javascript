declare module godot {
	/**
	 * Export class to godot
	 *
	 * @param target The class extends from `godot.Object`
	 * @param name The class name
	 * @param tool Does this script runs on godot editor
	 * @param icon The icon of the this class
	 */
	function register_class(target: Function, name: string, tool?: boolean, icon?: string);

	/**
	 * Register signal to class
	 * @param target The class of the signal
	 * @param name signal name
	 */
	function register_signal(target: Function|object, name: string);

	/**
	 * Register property to class
	 * @param target The class of the property
	 * @param name The name of the property
	 * @param type The godot variant type of the property must be one of `godot.TYPE_` constant value
	 * @param value The default value of the property
	 */
	function register_property(target: Function|object, name: string, type: number, value: any);


	/**
	 Vector used for 2D math.

	 2-element structure that can be used to represent positions in 2d space or any other pair of numeric values.
	*/
	class Vector2 {

		constructor(x?: number, y?: number);

		/**
		 Zero vector.
		* @value Vector2( 0, 0 )
		*/
		static readonly ZERO: number;

		/**
		 One vector.
		* @value Vector2( 1, 1 )
		*/
		static readonly ONE: number;

		/**
		 Infinite vector.
		* @value Vector2( inf, inf )
		*/
		static readonly INF: number;

		/**
		 Left unit vector.
		* @value Vector2( -1, 0 )
		*/
		static readonly LEFT: number;

		/**
		 Right unit vector.
		* @value Vector2( 1, 0 )
		*/
		static readonly RIGHT: number;

		/**
		 Up unit vector.
		* @value Vector2( 0, -1 )
		*/
		static readonly UP: number;

		/**
		 Down unit vector.
		* @value Vector2( 0, 1 )
		*/
		static readonly DOWN: number;


		/**
		 The vector's x component. Also accessible by using the index position `[0]`.
		*/
		x: number;

		/**
		 The vector's y component. Also accessible by using the index position `[1]`.
		*/
		y: number;

		add(p_value: Vector2): Vector2;
		add_assign(p_value: Vector2): Vector2;

		subtract(p_value: Vector2): Vector2;
		subtract_assign(p_value: Vector2): Vector2;

		multiply(p_value: Vector2 | number): Vector2;
		multiply_assign(p_value: Vector2 | number): Vector2;

		divide(p_value: Vector2 | number): Vector2;
		divide_assign(p_value: number): Vector2;

		negate(): Vector2;
		negate_assign(): Vector2;

		equals(p_value: Vector2): boolean;

		less(p_value: Vector2): boolean;
		less_equal(p_value: Vector2): boolean;

		/**
		 Returns a new vector with all components in absolute values (i.e. positive).
		*/
		abs() : Vector2;

		/**
		 Returns the vector's angle in radians with respect to the x-axis, or `(1, 0)` vector.
		 Equivalent to the result of atan2 when called with the vector's x and y as parameters: `atan2(x, y)`.
		*/
		angle() : number;

		/**
		 Returns the angle in radians between the two vectors.
		*/
		angle_to(to: Vector2) : number;

		/**
		 Returns the angle in radians between the line connecting the two points and the x coordinate.
		*/
		angle_to_point(to: Vector2) : number;

		/**
		 Returns the ratio of x to y.
		*/
		aspect() : number;

		/**
		 Returns the vector "bounced off" from a plane defined by the given normal.
		*/
		bounce(n: Vector2) : Vector2;

		/**
		 Returns the vector with all components rounded up.
		*/
		ceil() : Vector2;

		/**
		 Returns the vector with a maximum length.
		*/
		clamped(length: number) : Vector2;

		/**
		 Returns the 2 dimensional analog of the cross product with the given vector.
		*/
		cross(p_with: Vector2) : number;

		/**
		 Cubicly interpolates between this vector and `b` using `pre_a` and `post_b` as handles, and returns the result at position `t`. `t` is in the range of `0.0 - 1.0`, representing the amount of interpolation.
		*/
		cubic_interpolate(b: Vector2, pre_a: Vector2, post_b: Vector2, t: number) : Vector2;

		/**
		 Returns the squared distance to vector `b`. Prefer this function over [method distance_to] if you need to sort vectors or need the squared distance for some formula.
		*/
		distance_squared_to(to: Vector2) : number;

		/**
		 Returns the distance to vector `b`.
		*/
		distance_to(to: Vector2) : number;

		/**
		 Returns the dot product with vector `b`.
		*/
		dot(p_with: Vector2) : number;

		/**
		 Returns the vector with all components rounded down.
		*/
		floor() : Vector2;

		/**
		 Returns `true` if the vector is normalized.
		*/
		is_normalized() : boolean;

		/**
		 Returns the vector's length.
		*/
		length() : number;

		/**
		 Returns the vector's length squared. Prefer this method over [method length] if you need to sort vectors or need the squared length for some formula.
		*/
		length_squared() : number;

		/**
		 Returns the result of the linear interpolation between this vector and `b` by amount `t`. `t` is in the range of `0.0 - 1.0`, representing the amount of interpolation.
		*/
		linear_interpolate(b: Vector2, t: number) : Vector2;

		/**
		 Returns the vector scaled to unit length. Equivalent to `v / v.length()`.
		*/
		normalized() : Vector2;

		/**
		 Returns the vector projected onto the vector `b`.
		*/
		project(b: Vector2) : Vector2;

		/**
		 Returns the vector reflected from a plane defined by the given normal.
		*/
		reflect(n: Vector2) : Vector2;

		/**
		 Returns the vector rotated by `phi` radians.
		*/
		rotated(phi: number) : Vector2;

		/**
		 Returns the vector with all components rounded to the nearest integer, with halfway cases rounded away from zero.
		*/
		round() : Vector2;

		/**
		 Returns the result of SLERP between this vector and `b`, by amount `t`. `t` is in the range of `0.0 - 1.0`, representing the amount of interpolation.
		 Both vectors need to be normalized.
		*/
		slerp(b: Vector2, t: number) : Vector2;

		/**
		 Returns the component of the vector along a plane defined by the given normal.
		*/
		slide(n: Vector2) : Vector2;

		/**
		 Returns the vector snapped to a grid with the given size.
		*/
		snapped(by: Vector2) : Vector2;

		/**
		 Returns a perpendicular vector.
		*/
		tangent() : Vector2;

	}

	/**
	 Vector class, which performs basic 3D vector math operations.

	 Vector3 is one of the core classes of the engine, and includes several built-in helper functions to perform basic vector math operations.
	*/
	class Vector3 {

		constructor(x?: number, y?: number, z?: number)

		/**
		 Enumerated value for the X axis. Returned by [method max_axis] and [method min_axis].
		* @value 0
		 */
		static readonly AXIS_X: number;

		/**
		 Enumerated value for the Y axis.
		* @value 1
		 */
		static readonly AXIS_Y: number;

		/**
		 Enumerated value for the Z axis.
		* @value 2
		 */
		static readonly AXIS_Z: number;

		/**
		 Zero vector.
		* @value Vector3( 0, 0, 0 )
		*/
		static readonly ZERO: number;

		/**
		 One vector.
		* @value Vector3( 1, 1, 1 )
		*/
		static readonly ONE: number;

		/**
		 Infinite vector.
		* @value Vector3( inf, inf, inf )
		*/
		static readonly INF: number;

		/**
		 Left unit vector.
		* @value Vector3( -1, 0, 0 )
		*/
		static readonly LEFT: number;

		/**
		 Right unit vector.
		* @value Vector3( 1, 0, 0 )
		*/
		static readonly RIGHT: number;

		/**
		 Up unit vector.
		* @value Vector3( 0, 1, 0 )
		*/
		static readonly UP: number;

		/**
		 Down unit vector.
		* @value Vector3( 0, -1, 0 )
		*/
		static readonly DOWN: number;

		/**
		 Forward unit vector.
		* @value Vector3( 0, 0, -1 )
		*/
		static readonly FORWARD: number;

		/**
		 Back unit vector.
		* @value Vector3( 0, 0, 1 )
		*/
		static readonly BACK: number;


		/**
		 The vector's x component. Also accessible by using the index position `[0]`.
		*/
		x: number;

		/**
		 The vector's y component. Also accessible by using the index position `[1]`.
		*/
		y: number;

		/**
		 The vector's z component. Also accessible by using the index position `[2]`.
		*/
		z: number;
		
		add(p_value: Vector3): Vector3;
		add_assign(p_value: Vector3): Vector3;

		subtract(p_value: Vector3): Vector3;
		subtract_assign(p_value: Vector3): Vector3;

		multiply(p_value: Vector3 | number): Vector3;
		multiply_assign(p_value: Vector3 | number): Vector3;

		divide(p_value: Vector3 | number): Vector3;
		divide_assign(p_value: number|Vector3): Vector3;

		negate(): Vector3;
		negate_assign(): Vector3;

		equals(p_value: Vector3): boolean;

		less(p_value: Vector3): boolean;
		less_equal(p_value: Vector3): boolean;

		/**
		 Returns a new vector with all components in absolute values (i.e. positive).
		*/
		abs() : Vector3;

		/**
		 Returns the minimum angle to the given vector.
		*/
		angle_to(to: Vector3) : number;

		/**
		 Returns the vector "bounced off" from a plane defined by the given normal.
		*/
		bounce(n: Vector3) : Vector3;

		/**
		 Returns a new vector with all components rounded up.
		*/
		ceil() : Vector3;

		/**
		 Returns the cross product with `b`.
		*/
		cross(b: Vector3) : Vector3;

		/**
		 Performs a cubic interpolation between vectors `pre_a`, `a`, `b`, `post_b` (`a` is current), by the given amount `t`. `t` is in the range of `0.0 - 1.0`, representing the amount of interpolation.
		*/
		cubic_interpolate(b: Vector3, pre_a: Vector3, post_b: Vector3, t: number) : Vector3;

		/**
		 Returns the squared distance to `b`. Prefer this function over [method distance_to] if you need to sort vectors or need the squared distance for some formula.
		*/
		distance_squared_to(b: Vector3) : number;

		/**
		 Returns the distance to `b`.
		*/
		distance_to(b: Vector3) : number;

		/**
		 Returns the dot product with `b`.
		*/
		dot(b: Vector3) : number;

		/**
		 Returns a new vector with all components rounded down.
		*/
		floor() : Vector3;

		/**
		 Returns the inverse of the vector. This is the same as `Vector3( 1.0 / v.x, 1.0 / v.y, 1.0 / v.z )`.
		*/
		inverse() : Vector3;

		/**
		 Returns `true` if the vector is normalized.
		*/
		is_normalized() : boolean;

		/**
		 Returns the vector's length.
		*/
		length() : number;

		/**
		 Returns the vector's length squared. Prefer this function over [method length] if you need to sort vectors or need the squared length for some formula.
		*/
		length_squared() : number;

		/**
		 Returns the result of the linear interpolation between this vector and `b` by amount `t`. `t` is in the range of `0.0 - 1.0`, representing the amount of interpolation..
		*/
		linear_interpolate(b: Vector3, t: number) : Vector3;

		/**
		 Returns the axis of the vector's largest value. See `AXIS_*` constants.
		*/
		max_axis() : number;

		/**
		 Returns the axis of the vector's smallest value. See `AXIS_*` constants.
		*/
		min_axis() : number;

		/**
		 Returns the vector scaled to unit length. Equivalent to `v / v.length()`.
		*/
		normalized() : Vector3;

		/**
		 Returns the outer product with `b`.
		*/
		outer(b: Vector3) : Basis;

		/**
		 Returns the vector projected onto the vector `b`.
		*/
		project(b: Vector3) : Vector3;

		/**
		 Returns the vector reflected from a plane defined by the given normal.
		*/
		reflect(n: Vector3) : Vector3;

		/**
		 Rotates the vector around a given axis by `phi` radians. The axis must be a normalized vector.
		*/
		rotated(axis: Vector3, phi: number) : Vector3;

		/**
		 Returns the vector with all components rounded to the nearest integer, with halfway cases rounded away from zero.
		*/
		round() : Vector3;

		/**
		 Returns the result of SLERP between this vector and `b`, by amount `t`. `t` is in the range of `0.0 - 1.0`, representing the amount of interpolation.
		 Both vectors need to be normalized.
		*/
		slerp(b: Vector3, t: number) : Vector3;

		/**
		 Returns the component of the vector along a plane defined by the given normal.
		*/
		slide(n: Vector3) : Vector3;

		/**
		 Returns a copy of the vector, snapped to the lowest neared multiple.
		*/
		snapped(by: Vector3) : Vector3;

		/**
		 Returns a diagonal matrix with the vector as main diagonal.
		*/
		to_diagonal_matrix() : Basis;

	}


	/**
	 Color in RGBA format with some support for ARGB format.

	 A color is represented by red, green, and blue `(r, g, b)` components. Additionally, `a` represents the alpha component, often used for transparency. Values are in floating point and usually range from 0 to 1. Some properties (such as [member CanvasItem.modulate]) may accept values > 1.
	 You can also create a color from standardized color names by using [method @GDScript.ColorN].
	*/
	class Color {

		constructor(name_or_hex_or_r?:string|number, g?: number, b?: number, a?: number);

		add(p_value: Color): Color;
		add_assign(p_value: Color): Color;

		subtract(p_value: Color): Color;
		subtract_assign(p_value: Color): Color;

		multiply(p_value: Color | number): Color;
		multiply_assign(p_value: Color | number): Color;

		divide(p_value: Color | number): Color;
		divide_assign(p_value: Color | number): Color;

		negate(): Color;
		negate_assign(): Color;

		equals(p_value: Color): boolean;

		less(p_value: Color): boolean;

		/**
		* @value Color( 0.75, 0.75, 0.75, 1 )
		*/
		static readonly gray: number;

		/**
		* @value Color( 0.94, 0.97, 1, 1 )
		*/
		static readonly aliceblue: number;

		/**
		* @value Color( 0.98, 0.92, 0.84, 1 )
		*/
		static readonly antiquewhite: number;

		/**
		* @value Color( 0, 1, 1, 1 )
		*/
		static readonly aqua: number;

		/**
		* @value Color( 0.5, 1, 0.83, 1 )
		*/
		static readonly aquamarine: number;

		/**
		* @value Color( 0.94, 1, 1, 1 )
		*/
		static readonly azure: number;

		/**
		* @value Color( 0.96, 0.96, 0.86, 1 )
		*/
		static readonly beige: number;

		/**
		* @value Color( 1, 0.89, 0.77, 1 )
		*/
		static readonly bisque: number;

		/**
		* @value Color( 0, 0, 0, 1 )
		*/
		static readonly black: number;

		/**
		* @value Color( 1, 0.92, 0.8, 1 )
		*/
		static readonly blanchedalmond: number;

		/**
		* @value Color( 0, 0, 1, 1 )
		*/
		static readonly blue: number;

		/**
		* @value Color( 0.54, 0.17, 0.89, 1 )
		*/
		static readonly blueviolet: number;

		/**
		* @value Color( 0.65, 0.16, 0.16, 1 )
		*/
		static readonly brown: number;

		/**
		* @value Color( 0.87, 0.72, 0.53, 1 )
		*/
		static readonly burlywood: number;

		/**
		* @value Color( 0.37, 0.62, 0.63, 1 )
		*/
		static readonly cadetblue: number;

		/**
		* @value Color( 0.5, 1, 0, 1 )
		*/
		static readonly chartreuse: number;

		/**
		* @value Color( 0.82, 0.41, 0.12, 1 )
		*/
		static readonly chocolate: number;

		/**
		* @value Color( 1, 0.5, 0.31, 1 )
		*/
		static readonly coral: number;

		/**
		* @value Color( 0.39, 0.58, 0.93, 1 )
		*/
		static readonly cornflower: number;

		/**
		* @value Color( 1, 0.97, 0.86, 1 )
		*/
		static readonly cornsilk: number;

		/**
		* @value Color( 0.86, 0.08, 0.24, 1 )
		*/
		static readonly crimson: number;

		/**
		* @value Color( 0, 1, 1, 1 )
		*/
		static readonly cyan: number;

		/**
		* @value Color( 0, 0, 0.55, 1 )
		*/
		static readonly darkblue: number;

		/**
		* @value Color( 0, 0.55, 0.55, 1 )
		*/
		static readonly darkcyan: number;

		/**
		* @value Color( 0.72, 0.53, 0.04, 1 )
		*/
		static readonly darkgoldenrod: number;

		/**
		* @value Color( 0.66, 0.66, 0.66, 1 )
		*/
		static readonly darkgray: number;

		/**
		* @value Color( 0, 0.39, 0, 1 )
		*/
		static readonly darkgreen: number;

		/**
		* @value Color( 0.74, 0.72, 0.42, 1 )
		*/
		static readonly darkkhaki: number;

		/**
		* @value Color( 0.55, 0, 0.55, 1 )
		*/
		static readonly darkmagenta: number;

		/**
		* @value Color( 0.33, 0.42, 0.18, 1 )
		*/
		static readonly darkolivegreen: number;

		/**
		* @value Color( 1, 0.55, 0, 1 )
		*/
		static readonly darkorange: number;

		/**
		* @value Color( 0.6, 0.2, 0.8, 1 )
		*/
		static readonly darkorchid: number;

		/**
		* @value Color( 0.55, 0, 0, 1 )
		*/
		static readonly darkred: number;

		/**
		* @value Color( 0.91, 0.59, 0.48, 1 )
		*/
		static readonly darksalmon: number;

		/**
		* @value Color( 0.56, 0.74, 0.56, 1 )
		*/
		static readonly darkseagreen: number;

		/**
		* @value Color( 0.28, 0.24, 0.55, 1 )
		*/
		static readonly darkslateblue: number;

		/**
		* @value Color( 0.18, 0.31, 0.31, 1 )
		*/
		static readonly darkslategray: number;

		/**
		* @value Color( 0, 0.81, 0.82, 1 )
		*/
		static readonly darkturquoise: number;

		/**
		* @value Color( 0.58, 0, 0.83, 1 )
		*/
		static readonly darkviolet: number;

		/**
		* @value Color( 1, 0.08, 0.58, 1 )
		*/
		static readonly deeppink: number;

		/**
		* @value Color( 0, 0.75, 1, 1 )
		*/
		static readonly deepskyblue: number;

		/**
		* @value Color( 0.41, 0.41, 0.41, 1 )
		*/
		static readonly dimgray: number;

		/**
		* @value Color( 0.12, 0.56, 1, 1 )
		*/
		static readonly dodgerblue: number;

		/**
		* @value Color( 0.7, 0.13, 0.13, 1 )
		*/
		static readonly firebrick: number;

		/**
		* @value Color( 1, 0.98, 0.94, 1 )
		*/
		static readonly floralwhite: number;

		/**
		* @value Color( 0.13, 0.55, 0.13, 1 )
		*/
		static readonly forestgreen: number;

		/**
		* @value Color( 1, 0, 1, 1 )
		*/
		static readonly fuchsia: number;

		/**
		* @value Color( 0.86, 0.86, 0.86, 1 )
		*/
		static readonly gainsboro: number;

		/**
		* @value Color( 0.97, 0.97, 1, 1 )
		*/
		static readonly ghostwhite: number;

		/**
		* @value Color( 1, 0.84, 0, 1 )
		*/
		static readonly gold: number;

		/**
		* @value Color( 0.85, 0.65, 0.13, 1 )
		*/
		static readonly goldenrod: number;

		/**
		* @value Color( 0, 1, 0, 1 )
		*/
		static readonly green: number;

		/**
		* @value Color( 0.68, 1, 0.18, 1 )
		*/
		static readonly greenyellow: number;

		/**
		* @value Color( 0.94, 1, 0.94, 1 )
		*/
		static readonly honeydew: number;

		/**
		* @value Color( 1, 0.41, 0.71, 1 )
		*/
		static readonly hotpink: number;

		/**
		* @value Color( 0.8, 0.36, 0.36, 1 )
		*/
		static readonly indianred: number;

		/**
		* @value Color( 0.29, 0, 0.51, 1 )
		*/
		static readonly indigo: number;

		/**
		* @value Color( 1, 1, 0.94, 1 )
		*/
		static readonly ivory: number;

		/**
		* @value Color( 0.94, 0.9, 0.55, 1 )
		*/
		static readonly khaki: number;

		/**
		* @value Color( 0.9, 0.9, 0.98, 1 )
		*/
		static readonly lavender: number;

		/**
		* @value Color( 1, 0.94, 0.96, 1 )
		*/
		static readonly lavenderblush: number;

		/**
		* @value Color( 0.49, 0.99, 0, 1 )
		*/
		static readonly lawngreen: number;

		/**
		* @value Color( 1, 0.98, 0.8, 1 )
		*/
		static readonly lemonchiffon: number;

		/**
		* @value Color( 0.68, 0.85, 0.9, 1 )
		*/
		static readonly lightblue: number;

		/**
		* @value Color( 0.94, 0.5, 0.5, 1 )
		*/
		static readonly lightcoral: number;

		/**
		* @value Color( 0.88, 1, 1, 1 )
		*/
		static readonly lightcyan: number;

		/**
		* @value Color( 0.98, 0.98, 0.82, 1 )
		*/
		static readonly lightgoldenrod: number;

		/**
		* @value Color( 0.83, 0.83, 0.83, 1 )
		*/
		static readonly lightgray: number;

		/**
		* @value Color( 0.56, 0.93, 0.56, 1 )
		*/
		static readonly lightgreen: number;

		/**
		* @value Color( 1, 0.71, 0.76, 1 )
		*/
		static readonly lightpink: number;

		/**
		* @value Color( 1, 0.63, 0.48, 1 )
		*/
		static readonly lightsalmon: number;

		/**
		* @value Color( 0.13, 0.7, 0.67, 1 )
		*/
		static readonly lightseagreen: number;

		/**
		* @value Color( 0.53, 0.81, 0.98, 1 )
		*/
		static readonly lightskyblue: number;

		/**
		* @value Color( 0.47, 0.53, 0.6, 1 )
		*/
		static readonly lightslategray: number;

		/**
		* @value Color( 0.69, 0.77, 0.87, 1 )
		*/
		static readonly lightsteelblue: number;

		/**
		* @value Color( 1, 1, 0.88, 1 )
		*/
		static readonly lightyellow: number;

		/**
		* @value Color( 0, 1, 0, 1 )
		*/
		static readonly lime: number;

		/**
		* @value Color( 0.2, 0.8, 0.2, 1 )
		*/
		static readonly limegreen: number;

		/**
		* @value Color( 0.98, 0.94, 0.9, 1 )
		*/
		static readonly linen: number;

		/**
		* @value Color( 1, 0, 1, 1 )
		*/
		static readonly magenta: number;

		/**
		* @value Color( 0.69, 0.19, 0.38, 1 )
		*/
		static readonly maroon: number;

		/**
		* @value Color( 0.4, 0.8, 0.67, 1 )
		*/
		static readonly mediumaquamarine: number;

		/**
		* @value Color( 0, 0, 0.8, 1 )
		*/
		static readonly mediumblue: number;

		/**
		* @value Color( 0.73, 0.33, 0.83, 1 )
		*/
		static readonly mediumorchid: number;

		/**
		* @value Color( 0.58, 0.44, 0.86, 1 )
		*/
		static readonly mediumpurple: number;

		/**
		* @value Color( 0.24, 0.7, 0.44, 1 )
		*/
		static readonly mediumseagreen: number;

		/**
		* @value Color( 0.48, 0.41, 0.93, 1 )
		*/
		static readonly mediumslateblue: number;

		/**
		* @value Color( 0, 0.98, 0.6, 1 )
		*/
		static readonly mediumspringgreen: number;

		/**
		* @value Color( 0.28, 0.82, 0.8, 1 )
		*/
		static readonly mediumturquoise: number;

		/**
		* @value Color( 0.78, 0.08, 0.52, 1 )
		*/
		static readonly mediumvioletred: number;

		/**
		* @value Color( 0.1, 0.1, 0.44, 1 )
		*/
		static readonly midnightblue: number;

		/**
		* @value Color( 0.96, 1, 0.98, 1 )
		*/
		static readonly mintcream: number;

		/**
		* @value Color( 1, 0.89, 0.88, 1 )
		*/
		static readonly mistyrose: number;

		/**
		* @value Color( 1, 0.89, 0.71, 1 )
		*/
		static readonly moccasin: number;

		/**
		* @value Color( 1, 0.87, 0.68, 1 )
		*/
		static readonly navajowhite: number;

		/**
		* @value Color( 0, 0, 0.5, 1 )
		*/
		static readonly navyblue: number;

		/**
		* @value Color( 0.99, 0.96, 0.9, 1 )
		*/
		static readonly oldlace: number;

		/**
		* @value Color( 0.5, 0.5, 0, 1 )
		*/
		static readonly olive: number;

		/**
		* @value Color( 0.42, 0.56, 0.14, 1 )
		*/
		static readonly olivedrab: number;

		/**
		* @value Color( 1, 0.65, 0, 1 )
		*/
		static readonly orange: number;

		/**
		* @value Color( 1, 0.27, 0, 1 )
		*/
		static readonly orangered: number;

		/**
		* @value Color( 0.85, 0.44, 0.84, 1 )
		*/
		static readonly orchid: number;

		/**
		* @value Color( 0.93, 0.91, 0.67, 1 )
		*/
		static readonly palegoldenrod: number;

		/**
		* @value Color( 0.6, 0.98, 0.6, 1 )
		*/
		static readonly palegreen: number;

		/**
		* @value Color( 0.69, 0.93, 0.93, 1 )
		*/
		static readonly paleturquoise: number;

		/**
		* @value Color( 0.86, 0.44, 0.58, 1 )
		*/
		static readonly palevioletred: number;

		/**
		* @value Color( 1, 0.94, 0.84, 1 )
		*/
		static readonly papayawhip: number;

		/**
		* @value Color( 1, 0.85, 0.73, 1 )
		*/
		static readonly peachpuff: number;

		/**
		* @value Color( 0.8, 0.52, 0.25, 1 )
		*/
		static readonly peru: number;

		/**
		* @value Color( 1, 0.75, 0.8, 1 )
		*/
		static readonly pink: number;

		/**
		* @value Color( 0.87, 0.63, 0.87, 1 )
		*/
		static readonly plum: number;

		/**
		* @value Color( 0.69, 0.88, 0.9, 1 )
		*/
		static readonly powderblue: number;

		/**
		* @value Color( 0.63, 0.13, 0.94, 1 )
		*/
		static readonly purple: number;

		/**
		* @value Color( 0.4, 0.2, 0.6, 1 )
		*/
		static readonly rebeccapurple: number;

		/**
		* @value Color( 1, 0, 0, 1 )
		*/
		static readonly red: number;

		/**
		* @value Color( 0.74, 0.56, 0.56, 1 )
		*/
		static readonly rosybrown: number;

		/**
		* @value Color( 0.25, 0.41, 0.88, 1 )
		*/
		static readonly royalblue: number;

		/**
		* @value Color( 0.55, 0.27, 0.07, 1 )
		*/
		static readonly saddlebrown: number;

		/**
		* @value Color( 0.98, 0.5, 0.45, 1 )
		*/
		static readonly salmon: number;

		/**
		* @value Color( 0.96, 0.64, 0.38, 1 )
		*/
		static readonly sandybrown: number;

		/**
		* @value Color( 0.18, 0.55, 0.34, 1 )
		*/
		static readonly seagreen: number;

		/**
		* @value Color( 1, 0.96, 0.93, 1 )
		*/
		static readonly seashell: number;

		/**
		* @value Color( 0.63, 0.32, 0.18, 1 )
		*/
		static readonly sienna: number;

		/**
		* @value Color( 0.75, 0.75, 0.75, 1 )
		*/
		static readonly silver: number;

		/**
		* @value Color( 0.53, 0.81, 0.92, 1 )
		*/
		static readonly skyblue: number;

		/**
		* @value Color( 0.42, 0.35, 0.8, 1 )
		*/
		static readonly slateblue: number;

		/**
		* @value Color( 0.44, 0.5, 0.56, 1 )
		*/
		static readonly slategray: number;

		/**
		* @value Color( 1, 0.98, 0.98, 1 )
		*/
		static readonly snow: number;

		/**
		* @value Color( 0, 1, 0.5, 1 )
		*/
		static readonly springgreen: number;

		/**
		* @value Color( 0.27, 0.51, 0.71, 1 )
		*/
		static readonly steelblue: number;

		/**
		* @value Color( 0.82, 0.71, 0.55, 1 )
		*/
		static readonly tan: number;

		/**
		* @value Color( 0, 0.5, 0.5, 1 )
		*/
		static readonly teal: number;

		/**
		* @value Color( 0.85, 0.75, 0.85, 1 )
		*/
		static readonly thistle: number;

		/**
		* @value Color( 1, 0.39, 0.28, 1 )
		*/
		static readonly tomato: number;

		/**
		* @value Color( 0.25, 0.88, 0.82, 1 )
		*/
		static readonly turquoise: number;

		/**
		* @value Color( 0.93, 0.51, 0.93, 1 )
		*/
		static readonly violet: number;

		/**
		* @value Color( 0.5, 0.5, 0.5, 1 )
		*/
		static readonly webgray: number;

		/**
		* @value Color( 0, 0.5, 0, 1 )
		*/
		static readonly webgreen: number;

		/**
		* @value Color( 0.5, 0, 0, 1 )
		*/
		static readonly webmaroon: number;

		/**
		* @value Color( 0.5, 0, 0.5, 1 )
		*/
		static readonly webpurple: number;

		/**
		* @value Color( 0.96, 0.87, 0.7, 1 )
		*/
		static readonly wheat: number;

		/**
		* @value Color( 1, 1, 1, 1 )
		*/
		static readonly white: number;

		/**
		* @value Color( 0.96, 0.96, 0.96, 1 )
		*/
		static readonly whitesmoke: number;

		/**
		* @value Color( 1, 1, 0, 1 )
		*/
		static readonly yellow: number;

		/**
		* @value Color( 0.6, 0.8, 0.2, 1 )
		*/
		static readonly yellowgreen: number;


		/**
		 Red value (range 0 to 1).
		*/
		r: number;

		/**
		 Green value (range 0 to 1).
		*/
		g: number;

		/**
		 Blue value (range 0 to 1).
		*/
		b: number;

		/**
		 Alpha value (range 0 to 1).
		*/
		a: number;

		/**
		 HSV hue value (range 0 to 1).
		*/
		h: number;

		/**
		 HSV saturation value (range 0 to 1).
		*/
		s: number;

		/**
		 HSV value (range 0 to 1).
		*/
		v: number;

		/**
		 Red value (range 0 to 255).
		*/
		r8: number;

		/**
		 Green value (range 0 to 255).
		*/
		g8: number;

		/**
		 Blue value (range 0 to 255).
		*/
		b8: number;

		/**
		 Alpha value (range 0 to 255).
		*/
		a8: number;


		/**
		 Returns a new color resulting from blending this color over another. If the color is opaque, the result is also opaque. The second color may have a range of alpha values.
		 ```gdscript
		 var bg = Color(0.0, 1.0, 0.0, 0.5) # Green with alpha of 50%
		 var fg = Color(1.0, 0.0, 0.0, 0.5) # Red with alpha of 50%
		 var blended_color = bg.blend(fg) # Brown with alpha of 75%
		 ```
		*/
		blend(over: Color) : Color;

		/**
		 Returns the most contrasting color.
		 ```gdscript
		 var c = Color(0.3, 0.4, 0.9)
		 var contrasted_color = c.contrasted() # Equivalent to RGBA(204, 229, 102, 255)
		 ```
		*/
		contrasted() : Color;

		/**
		 Returns a new color resulting from making this color darker by the specified percentage (ratio from 0 to 1).
		 ```gdscript
		 var green = Color(0.0, 1.0, 0.0)
		 var darkgreen = green.darkened(0.2) # 20% darker than regular green
		 ```
		*/
		darkened(amount: number) : Color;

		/**
		 Constructs a color from an HSV profile. `h`, `s`, and `v` are values between 0 and 1.
		 ```gdscript
		 var c = Color.from_hsv(0.58, 0.5, 0.79, 0.8) # Equivalent to HSV(210, 50, 79, 0.8) or Color8(100, 151, 201, 0.8)
		 ```
		*/
		from_hsv(h: number, s: number, v: number, a?: number) : Color;

		/**
		 Returns the color's grayscale representation.
		 The gray value is calculated as `(r + g + b) / 3`.
		 ```gdscript
		 var c = Color(0.2, 0.45, 0.82)
		 var gray = c.gray() # a value of 0.466667
		 ```
		*/
		gray() : number;

		/**
		 Returns the inverted color `(1 - r, 1 - g, 1 - b, 1 - a)`.
		 ```gdscript
		 var c = Color(0.3, 0.4, 0.9)
		 var inverted_color = c.inverted() # a color of an RGBA(178, 153, 26, 255)
		 ```
		*/
		inverted() : Color;

		/**
		 Returns a new color resulting from making this color lighter by the specified percentage (ratio from 0 to 1).
		 ```gdscript
		 var green = Color(0.0, 1.0, 0.0)
		 var lightgreen = green.lightened(0.2) # 20% lighter than regular green
		 ```
		*/
		lightened(amount: number) : Color;

		/**
		 Returns the linear interpolation with another color. The interpolation factor `t` is between 0 and 1.
		 ```gdscript
		 var c1 = Color(1.0, 0.0, 0.0)
		 var c2 = Color(0.0, 1.0, 0.0)
		 var li_c = c1.linear_interpolate(c2, 0.5) # a color of an RGBA(128, 128, 0, 255)
		 ```
		*/
		linear_interpolate(b: Color, t: number) : Color;

		/**
		 Returns the color's 32-bit integer in ABGR format (each byte represents a component of the ABGR profile). ABGR is the reversed version of the default format.
		 ```gdscript
		 var c = Color(1, 0.5, 0.2)
		 print(c.to_abgr32()) # Prints 4281565439
		 ```
		*/
		to_abgr32() : number;

		/**
		 Returns the color's 64-bit integer in ABGR format (each word represents a component of the ABGR profile). ABGR is the reversed version of the default format.
		 ```gdscript
		 var c = Color(1, 0.5, 0.2)
		 print(c.to_abgr64()) # Prints -225178692812801
		 ```
		*/
		to_abgr64() : number;

		/**
		 Returns the color's 32-bit integer in ARGB format (each byte represents a component of the ARGB profile). ARGB is more compatible with DirectX.
		 ```gdscript
		 var c = Color(1, 0.5, 0.2)
		 print(c.to_argb32()) # Prints 4294934323
		 ```
		*/
		to_argb32() : number;

		/**
		 Returns the color's 64-bit integer in ARGB format (each word represents a component of the ARGB profile). ARGB is more compatible with DirectX.
		 ```gdscript
		 var c = Color(1, 0.5, 0.2)
		 print(c.to_argb64()) # Prints -2147470541
		 ```
		*/
		to_argb64() : number;

		/**
		 Returns the color's HTML hexadecimal color string in ARGB format (ex: `ff34f822`).
		 Setting `with_alpha` to `false` excludes alpha from the hexadecimal string.
		 ```gdscript
		 var c = Color(1, 1, 1, 0.5)
		 var s1 = c.to_html() # Results "7fffffff"
		 var s2 = c.to_html(false) # Results 'ffffff'
		 ```
		*/
		to_html(with_alpha?: boolean) : string;

		/**
		 Returns the color's 32-bit integer in RGBA format (each byte represents a component of the RGBA profile). RGBA is Godot's default format.
		 ```gdscript
		 var c = Color(1, 0.5, 0.2)
		 print(c.to_rgba32()) # Prints 4286526463
		 ```
		*/
		to_rgba32() : number;

		/**
		 Returns the color's 64-bit integer in RGBA format (each word represents a component of the RGBA profile). RGBA is Godot's default format.
		 ```gdscript
		 var c = Color(1, 0.5, 0.2)
		 print(c.to_rgba64()) # Prints -140736629309441
		 ```
		*/
		to_rgba64() : number;

	}

	/**
	 2D Axis-aligned bounding box.

	 Rect2 consists of a position, a size, and several utility functions. It is typically used for fast overlap tests.
	*/
	class Rect2 {

		constructor(x_or_pos?: number|Vector2, y_or_size?: number|Vector2, w?: number, h?:number);

		/**
		 Position (starting corner).
		*/
		position: Vector2;

		/**
		 Size from position to end.
		*/
		size: Vector2;

		/**
		 Ending corner.
		*/
		end: Vector2;

		equals(p_value: Rect2): boolean;


		/**
		 Returns a `Rect2` with equivalent position and area, modified so that the top-left corner is the origin and `width` and `height` are positive.
		*/
		abs() : Rect2;

		/**
		 Returns the intersection of this `Rect2` and b.
		*/
		clip(b: Rect2) : Rect2;

		/**
		 Returns `true` if this `Rect2` completely encloses another one.
		*/
		encloses(b: Rect2) : boolean;

		/**
		 Returns this `Rect2` expanded to include a given point.
		*/
		expand(to: Vector2) : Rect2;

		/**
		 Returns the area of the `Rect2`.
		*/
		get_area() : number;

		/**
		 Returns a copy of the `Rect2` grown a given amount of units towards all the sides.
		*/
		grow(by: number) : Rect2;

		/**
		 Returns a copy of the `Rect2` grown a given amount of units towards each direction individually.
		*/
		grow_individual(left: number, top: number, right: number,  bottom: number) : Rect2;

		/**
		 Returns a copy of the `Rect2` grown a given amount of units towards the [enum Margin] direction.
		*/
		grow_margin(margin: number, by: number) : Rect2;

		/**
		 Returns `true` if the `Rect2` is flat or empty.
		*/
		has_no_area() : boolean;

		/**
		 Returns `true` if the `Rect2` contains a point.
		*/
		has_point(point: Vector2) : boolean;

		/**
		 Returns `true` if the `Rect2` overlaps with another.
		*/
		intersects(b: Rect2) : boolean;

		/**
		 Returns a larger Rect2 that contains this Rect2 and `with`.
		*/
		merge(b: Rect2) : Rect2;

	}

	/**
	 Handle for a [Resource]'s unique ID.

	 The RID type is used to access the unique integer ID of a resource. They are opaque, so they do not grant access to the associated resource by themselves. They are used by and with the low-level Server classes such as [VisualServer].
	*/
	class RID {
		constructor(p_object?: Object)
		/** Returns the ID of the referenced resource.  */
		get_id() : number;

		equals(p_value: RID): boolean;
		
		less(p_value: RID): boolean;
		
		less_equal(p_value: RID): boolean;
	}
	
	/** 2D Transformation. 3x2 matrix.

	 Represents one or many transformations in 2D space such as translation, rotation, or scaling. It consists of a two [Vector2] x, y and [Vector2] "origin". It is similar to a 3x2 matrix. */
	class Transform2D {

		/** 
		 * @value Transform2D( 1, 0, 0, 1, 0, 0 )
		 */
		static readonly IDENTITY: number;

		/** 
		 * @value Transform2D( -1, 0, 0, 1, 0, 0 )
		 */
		static readonly FLIP_X: number;

		/** 
		 * @value Transform2D( 1, 0, 0, -1, 0, 0 )
		 */
		static readonly FLIP_Y: number;


		/** The X axis of 2x2 basis matrix containing 2 [Vector2]s as its columns: X axis and Y axis. These vectors can be interpreted as the basis vectors of local coordinate system traveling with the object. */
		x: Vector2;

		/** The Y axis of 2x2 basis matrix containing 2 [Vector2]s as its columns: X axis and Y axis. These vectors can be interpreted as the basis vectors of local coordinate system traveling with the object. */
		y: Vector2;

		/** The transform's translation offset. */
		origin: Vector2;


		/** Returns the inverse of the matrix. */
		affine_inverse() : Transform2D;

		/** Transforms the given vector by this transform's basis (no translation). */
		basis_xform(v: Vector2) : Vector2;

		/** Inverse-transforms the given vector by this transform's basis (no translation). */
		basis_xform_inv(v: Vector2) : Vector2;

		/** Returns the transform's origin (translation). */
		get_origin() : Vector2;

		/** Returns the transform's rotation (in radians). */
		get_rotation() : number;

		/** Returns the scale. */
		get_scale() : Vector2;

		/** Returns a transform interpolated between this transform and another by a given weight (0-1). */
		interpolate_with(transform: Transform2D, weight: number) : Transform2D;

		/** Returns the inverse of the transform, under the assumption that the transformation is composed of rotation and translation (no scaling, use affine_inverse for transforms with scaling). */
		inverse() : Transform2D;

		/** Returns the transform with the basis orthogonal (90 degrees), and normalized axis vectors. */
		orthonormalized() : Transform2D;

		/** Rotates the transform by the given angle (in radians). */
		rotated(phi: number) : Transform2D;

		/** Scales the transform by the given factor. */
		scaled(scale: Vector2) : Transform2D;

		/** Translates the transform by the given offset. */
		translated(offset: Vector2) : Transform2D;

		/** Transforms the given [Vector2] or [Rect2] by this transform. */
		xform(v: Vector2|Rect2) : Vector2|Rect2;

		/** Inverse-transforms the given [Vector2] or [Rect2] by this transform. */
		xform_inv(v: Vector2|Rect2) : Vector2|Rect2;
		
		equals(p_value: Transform2D): boolean;
		
		multiply(p_value: Transform2D): Transform2D;
		
		multiply_assign(p_value: Transform2D): Transform2D;

	}
}
