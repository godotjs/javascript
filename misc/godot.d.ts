
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
	
	//@ts-ignore
	const console: {
		log(...message): void;
		warn(...message): void;
		error(...message): void;
	}

	//@ts-ignore
	class Worker {
		onmessage(msg: any): void;
		postMessage(msg: any): void;
		terminate(): void;
		static abandonValue(value: any): number;
		static adoptValue(value_id: number): any;
	}
	
	function close(): void;
	//@ts-ignore
	function onmessage(msg: any): void;
	function postMessage(msg: any): void;
	function importScripts(...msg: string[]): void;
	
	const INSIDE_WORKER: true | undefined;
}

declare module godot {
	
	type GodotClass = new() => godot.Object;
	
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
	 * @param icon The icon of the class
	 */
	function set_script_meta(target: GodotClass, tool: boolean, icon?: string);
	
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
	 * Wait a signal of an object
	 * @param target The owner of the signal to wait
	 * @param signal The signal to wait
	 */
	function yield(target: godot.Object, signal: string): Promise<any[]>;
	
	const E: 2.7182818284590452353602874714;
	const LN2: 0.6931471805599453094172321215;
	const SQRT2: 1.4142135623730950488016887242;
	const SQRT12: 0.7071067811865475244008443621048490;
	
	const DEBUG_ENABLED: boolean;
	const TOOLS_ENABLED: boolean;
	const DEBUG_METHODS_ENABLED: boolean;
}

declare module godot {
	/** Vector used for 2D math.
	 2-element structure that can be used to represent positions in 2D space or any other pair of numeric values. */
	class Vector2 {
		
		constructor(x_or_pos?: number | Vector2, y?: number);

		/** Enumerated value for the X axis. */
		static readonly AXIS_X: 0;

		/** Enumerated value for the Y axis. */
		static readonly AXIS_Y: 1;

		/** Zero vector. 
		 * @value `Vector2( 0, 0 )` */
		static readonly ZERO: Vector2;

		/** One vector. 
		 * @value `Vector2( 1, 1 )` */
		static readonly ONE: Vector2;

		/** Infinity vector. 
		 * @value `Vector2( inf, inf )` */
		static readonly INF: Vector2;

		/** Left unit vector. 
		 * @value `Vector2( -1, 0 )` */
		static readonly LEFT: Vector2;

		/** Right unit vector. 
		 * @value `Vector2( 1, 0 )` */
		static readonly RIGHT: Vector2;

		/** Up unit vector. 
		 * @value `Vector2( 0, -1 )` */
		static readonly UP: Vector2;

		/** Down unit vector. 
		 * @value `Vector2( 0, 1 )` */
		static readonly DOWN: Vector2;


		/** The vector's X component. Also accessible by using the index position ``0``. */
		x: number;

		/** The vector's Y component. Also accessible by using the index position ``1``. */
		y: number;


		/** Returns a new vector with all components in absolute values (i.e. positive). */
		abs() : Vector2;

		/** Returns the vector's angle in radians with respect to the X axis, or `(1, 0)` vector.

		 Equivalent to the result of `@GDScript.atan2` when called with the vector's `x` and `y` as parameters: `atan2(x, y)`. */
		angle() : number;

		/** Returns the angle in radians between the two vectors. */
		angle_to(to: Vector2) : number;

		/** Returns the angle in radians between the line connecting the two points and the X coordinate. */
		angle_to_point(to: Vector2) : number;

		/** Returns the ratio of `x` to `y`. */
		aspect() : number;

		/** Returns the vector "bounced off" from a plane defined by the given normal. */
		bounce(n: Vector2) : Vector2;

		/** Returns the vector with all components rounded up. */
		ceil() : Vector2;

		/** Returns the vector with a maximum length. */
		clamped(length: number) : Vector2;

		/** Returns the 2-dimensional analog of the cross product with the given vector. */
		cross(p_with: Vector2) : number;

		/** Cubically interpolates between this vector and `b` using `pre_a` and `post_b` as handles, and returns the result at position `t`. `t` is in the range of `0.0 - 1.0`, representing the amount of interpolation. */
		cubic_interpolate(b: Vector2, pre_a: Vector2, post_b: Vector2, t: number) : Vector2;

		/** Returns the normalized vector pointing from this vector to `b`. */
		direction_to(b: Vector2) : Vector2;

		/** Returns the squared distance to vector `b`. Prefer this function over `distance_to` if you need to sort vectors or need the squared distance for some formula. */
		distance_squared_to(to: Vector2) : number;

		/** Returns the distance to vector `b`. */
		distance_to(to: Vector2) : number;

		/** Returns the dot product with vector `b`. */
		dot(p_with: Vector2) : number;

		/** Returns the vector with all components rounded down. */
		floor() : Vector2;

		/**  */
		is_equal_approx(v: Vector2) : boolean;

		/** Returns `true` if the vector is normalized. */
		is_normalized() : boolean;

		/** Returns the vector's length. */
		length() : number;

		/** Returns the vector's length squared. Prefer this method over `length` if you need to sort vectors or need the squared length for some formula. */
		length_squared() : number;

		/** Returns the result of the linear interpolation between this vector and `b` by amount `t`. `t` is in the range of `0.0 - 1.0`, representing the amount of interpolation. */
		linear_interpolate(b: Vector2, t: number) : Vector2;

		/** Moves the vector toward `to` by the fixed `delta` amount. */
		move_toward(to: Vector2, delta: number) : Vector2;

		/** Returns the vector scaled to unit length. Equivalent to `v / v.length()`. */
		normalized() : Vector2;

		/** Returns a vector composed of the `fposmod` of this vector's components and `mod`. */
		posmod(mod: number) : Vector2;

		/** Returns a vector composed of the `fposmod` of this vector's components and `modv`'s components. */
		posmodv(modv: Vector2) : Vector2;

		/** Returns the vector projected onto the vector `b`. */
		project(b: Vector2) : Vector2;

		/** Returns the vector reflected from a plane defined by the given normal. */
		reflect(n: Vector2) : Vector2;

		/** Returns the vector rotated by `phi` radians. See also `@GDScript.deg2rad`. */
		rotated(phi: number) : Vector2;

		/** Returns the vector with all components rounded to the nearest integer, with halfway cases rounded away from zero. */
		round() : Vector2;

		/** Returns the vector with each component set to one or negative one, depending on the signs of the components. */
		sign() : Vector2;

		/** Returns the result of spherical linear interpolation between this vector and `b`, by amount `t`. `t` is in the range of `0.0 - 1.0`, representing the amount of interpolation.

		 **Note:** Both vectors must be normalized. */
		slerp(b: Vector2, t: number) : Vector2;

		/** Returns the component of the vector along a plane defined by the given normal. */
		slide(n: Vector2) : Vector2;

		/** Returns the vector snapped to a grid with the given size. */
		snapped(by: Vector2) : Vector2;

		/** Returns a perpendicular vector. */
		tangent() : Vector2;

	}
	
	/** 2D axis-aligned bounding box.
	 Rect2 consists of a position, a size, and several utility functions. It is typically used for fast overlap tests. */
	class Rect2 {

		constructor(x_or_pos_or_rect?: number | Vector2 | Rect2, y_or_size?: number | Vector2, w?: number, h?: number);

		/** Position (starting corner). */
		position: Vector2;

		/** Size from position to end. */
		size: Vector2;

		/** Ending corner. */
		end: Vector2;


		/** Returns a `Rect2` with equivalent position and area, modified so that the top-left corner is the origin and `width` and `height` are positive. */
		abs() : Rect2;

		/** Returns the intersection of this `Rect2` and b. */
		clip(b: Rect2) : Rect2;

		/** Returns `true` if this `Rect2` completely encloses another one. */
		encloses(b: Rect2) : boolean;

		/** Returns this `Rect2` expanded to include a given point. */
		expand(to: Vector2) : Rect2;

		/** Returns the area of the `Rect2`. */
		get_area() : number;

		/** Returns a copy of the `Rect2` grown a given amount of units towards all the sides. */
		grow(by: number) : Rect2;

		/** Returns a copy of the `Rect2` grown a given amount of units towards each direction individually. */
		grow_individual(left: number, top: number, right: number,  bottom: number) : Rect2;

		/** Returns a copy of the `Rect2` grown a given amount of units towards the `Margin` direction. */
		grow_margin(margin: number, by: number) : Rect2;

		/** Returns `true` if the `Rect2` is flat or empty. */
		has_no_area() : boolean;

		/** Returns `true` if the `Rect2` contains a point. */
		has_point(point: Vector2) : boolean;

		/** Returns `true` if the `Rect2` overlaps with another. */
		intersects(b: Rect2) : boolean;

		/**  */
		is_equal_approx(rect: Rect2) : boolean;

		/** Returns a larger Rect2 that contains this Rect2 and `b`. */
		merge(b: Rect2) : Rect2;

	}

	/** Vector used for 3D math.
	 3-element structure that can be used to represent positions in 3D space or any other pair of numeric values. */
	class Vector3 {

		constructor(x_or_v3?: number | Vector3, y?: number, z?: number)

		/** Enumerated value for the X axis. Returned by `max_axis` and `min_axis`. */
		static readonly AXIS_X: 0;

		/** Enumerated value for the Y axis. Returned by `max_axis` and `min_axis`. */
		static readonly AXIS_Y: 1;

		/** Enumerated value for the Z axis. Returned by `max_axis` and `min_axis`. */
		static readonly AXIS_Z: 2;

		/** Zero vector. 
		 * @value `Vector3( 0, 0, 0 )` */
		static readonly ZERO: Vector3;

		/** One vector. 
		 * @value `Vector3( 1, 1, 1 )` */
		static readonly ONE: Vector3;

		/** Infinity vector. 
		 * @value `Vector3( inf, inf, inf )` */
		static readonly INF: Vector3;

		/** Left unit vector. 
		 * @value `Vector3( -1, 0, 0 )` */
		static readonly LEFT: Vector3;

		/** Right unit vector. 
		 * @value `Vector3( 1, 0, 0 )` */
		static readonly RIGHT: Vector3;

		/** Up unit vector. 
		 * @value `Vector3( 0, 1, 0 )` */
		static readonly UP: Vector3;

		/** Down unit vector. 
		 * @value `Vector3( 0, -1, 0 )` */
		static readonly DOWN: Vector3;

		/** Forward unit vector. 
		 * @value `Vector3( 0, 0, -1 )` */
		static readonly FORWARD: Vector3;

		/** Back unit vector. 
		 * @value `Vector3( 0, 0, 1 )` */
		static readonly BACK: Vector3;


		/** The vector's X component. Also accessible by using the index position ``0``. */
		x: number;

		/** The vector's Y component. Also accessible by using the index position ``1``. */
		y: number;

		/** The vector's Z component. Also accessible by using the index position ``2``. */
		z: number;


		/** Returns a new vector with all components in absolute values (i.e. positive). */
		abs() : Vector3;

		/** Returns the minimum angle to the given vector. */
		angle_to(to: Vector3) : number;

		/** Returns the vector "bounced off" from a plane defined by the given normal. */
		bounce(n: Vector3) : Vector3;

		/** Returns a new vector with all components rounded up. */
		ceil() : Vector3;

		/** Returns the cross product with `b`. */
		cross(b: Vector3) : Vector3;

		/** Performs a cubic interpolation between vectors `pre_a`, `a`, `b`, `post_b` (`a` is current), by the given amount `t`. `t` is in the range of `0.0 - 1.0`, representing the amount of interpolation. */
		cubic_interpolate(b: Vector3, pre_a: Vector3, post_b: Vector3, t: number) : Vector3;

		/** Returns the normalized vector pointing from this vector to `b`. */
		direction_to(b: Vector3) : Vector3;

		/** Returns the squared distance to `b`. Prefer this function over `distance_to` if you need to sort vectors or need the squared distance for some formula. */
		distance_squared_to(b: Vector3) : number;

		/** Returns the distance to `b`. */
		distance_to(b: Vector3) : number;

		/** Returns the dot product with `b`. */
		dot(b: Vector3) : number;

		/** Returns a new vector with all components rounded down. */
		floor() : Vector3;

		/** Returns the inverse of the vector. This is the same as `Vector3( 1.0 / v.x, 1.0 / v.y, 1.0 / v.z )`. */
		inverse() : Vector3;

		/**  */
		is_equal_approx(v: Vector3) : boolean;

		/** Returns `true` if the vector is normalized. */
		is_normalized() : boolean;

		/** Returns the vector's length. */
		length() : number;

		/** Returns the vector's length squared. Prefer this function over `length` if you need to sort vectors or need the squared length for some formula. */
		length_squared() : number;

		/** Returns the result of the linear interpolation between this vector and `b` by amount `t`. `t` is in the range of `0.0 - 1.0`, representing the amount of interpolation.. */
		linear_interpolate(b: Vector3, t: number) : Vector3;

		/** Returns the axis of the vector's largest value. See `AXIS_*` constants. */
		max_axis() : number;

		/** Returns the axis of the vector's smallest value. See `AXIS_*` constants. */
		min_axis() : number;

		/** Moves the vector toward `to` by the fixed `delta` amount. */
		move_toward(to: Vector3, delta: number) : Vector3;

		/** Returns the vector scaled to unit length. Equivalent to `v / v.length()`. */
		normalized() : Vector3;

		/** Returns the outer product with `b`. */
		outer(b: Vector3) : Basis;

		/** Returns a vector composed of the `fposmod` of this vector's components and `mod`. */
		posmod(mod: number) : Vector3;

		/** Returns a vector composed of the `fposmod` of this vector's components and `modv`'s components. */
		posmodv(modv: Vector3) : Vector3;

		/** Returns the vector projected onto the vector `b`. */
		project(b: Vector3) : Vector3;

		/** Returns the vector reflected from a plane defined by the given normal. */
		reflect(n: Vector3) : Vector3;

		/** Rotates the vector around a given axis by `phi` radians. The axis must be a normalized vector. */
		rotated(axis: Vector3, phi: number) : Vector3;

		/** Returns the vector with all components rounded to the nearest integer, with halfway cases rounded away from zero. */
		round() : Vector3;

		/** Returns the vector with each component set to one or negative one, depending on the signs of the components. */
		sign() : Vector3;

		/** Returns the result of spherical linear interpolation between this vector and `b`, by amount `t`. `t` is in the range of `0.0 - 1.0`, representing the amount of interpolation.

		 **Note:** Both vectors must be normalized. */
		slerp(b: Vector3, t: number) : Vector3;

		/** Returns the component of the vector along a plane defined by the given normal. */
		slide(n: Vector3) : Vector3;

		/** Returns a copy of the vector snapped to the lowest neared multiple. */
		snapped(by: Vector3) : Vector3;

		/** Returns a diagonal matrix with the vector as main diagonal. */
		to_diagonal_matrix() : Basis;

	}
	
	/** 3×3 matrix datatype.
	 3×3 matrix used for 3D rotation and scale. Contains 3 vector fields X, Y and Z as its columns, which can be interpreted as the local basis vectors of a transformation. Can also be accessed as array of 3D vectors. These vectors are orthogonal to each other, but are not necessarily normalized (due to scaling). Almost always used as an orthogonal basis for a `Transform`.

	 For such use, it is composed of a scaling and a rotation matrix, in that order (M = R.S). */
	class Basis {

		constructor(v0?: Basis|Vector3|Quat, v1?: number|Vector3, v2?:Vector3);
		
		/**  
		 * @value `Basis( 1, 0, 0, 0, 1, 0, 0, 0, 1 )` */
		static readonly IDENTITY: Basis;

		/**  
		 * @value `Basis( -1, 0, 0, 0, 1, 0, 0, 0, 1 )` */
		static readonly FLIP_X: Basis;

		/**  
		 * @value `Basis( 1, 0, 0, 0, -1, 0, 0, 0, 1 )` */
		static readonly FLIP_Y: Basis;

		/**  
		 * @value `Basis( 1, 0, 0, 0, 1, 0, 0, 0, -1 )` */
		static readonly FLIP_Z: Basis;


		/** The basis matrix's X vector. */
		x: Vector3;

		/** The basis matrix's Y vector. */
		y: Vector3;

		/** The basis matrix's Z vector. */
		z: Vector3;


		/** Returns the determinant of the matrix. */
		determinant() : number;

		/** Assuming that the matrix is a proper rotation matrix (orthonormal matrix with determinant +1), return Euler angles (in the YXZ convention: first Z, then X, and Y last). Returned vector contains the rotation angles in the format (X angle, Y angle, Z angle). */
		get_euler() : Vector3;

		/** This function considers a discretization of rotations into 24 points on unit sphere, lying along the vectors (x,y,z) with each component being either -1,0 or 1, and returns the index of the point best representing the orientation of the object. It is mainly used by the grid map editor. For further details, refer to Godot source code. */
		get_orthogonal_index() : number;

		/**  */
		get_rotation_quat() : Quat;

		/** Assuming that the matrix is the combination of a rotation and scaling, return the absolute value of scaling factors along each axis. */
		get_scale() : Vector3;

		/** Returns the inverse of the matrix. */
		inverse() : Basis;

		/**  */
		is_equal_approx(b: Basis, epsilon?: number) : boolean;

		/** Returns the orthonormalized version of the matrix (useful to call from time to time to avoid rounding error for orthogonal matrices). This performs a Gram-Schmidt orthonormalization on the basis of the matrix. */
		orthonormalized() : Basis;

		/** Introduce an additional rotation around the given axis by phi (radians). The axis must be a normalized vector. */
		rotated(axis: Vector3, phi: number) : Basis;

		/** Introduce an additional scaling specified by the given 3D scaling factor. */
		scaled(scale: Vector3) : Basis;

		/** Assuming that the matrix is a proper rotation matrix, slerp performs a spherical-linear interpolation with another rotation matrix. */
		slerp(b: Basis, t: number) : Basis;

		/** Transposed dot product with the X axis of the matrix. */
		tdotx(p_with: Vector3) : number;

		/** Transposed dot product with the Y axis of the matrix. */
		tdoty(p_with: Vector3) : number;

		/** Transposed dot product with the Z axis of the matrix. */
		tdotz(p_with: Vector3) : number;

		/** Returns the transposed version of the matrix. */
		transposed() : Basis;

		/** Returns a vector transformed (multiplied) by the matrix. */
		xform(v: Vector3) : Vector3;

		/** Returns a vector transformed (multiplied) by the transposed matrix.

		 **Note:** This results in a multiplication by the inverse of the matrix only if it represents a rotation-reflection. */
		xform_inv(v: Vector3) : Vector3;

	}
	
	/** 3D transformation (3×4 matrix).
	 Represents one or many transformations in 3D space such as translation, rotation, or scaling. It consists of a `basis` and an `origin`. It is similar to a 3×4 matrix. */
	class Transform {
		
		constructor(v0?: Transform|Vector3|Basis, v1?:Vector3, v2?:Vector3, v3?:Vector3);

		/** `Transform` with no translation, rotation or scaling applied. When applied to other data structures, `IDENTITY` performs no transformation. 
		 * @value `Transform( 1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 )` */
		static readonly IDENTITY: Transform;

		/** `Transform` with mirroring applied perpendicular to the YZ plane. 
		 * @value `Transform( -1, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0, 0 )` */
		static readonly FLIP_X: Transform;

		/** `Transform` with mirroring applied perpendicular to the XZ plane. 
		 * @value `Transform( 1, 0, 0, 0, -1, 0, 0, 0, 1, 0, 0, 0 )` */
		static readonly FLIP_Y: Transform;

		/** `Transform` with mirroring applied perpendicular to the XY plane. 
		 * @value `Transform( 1, 0, 0, 0, 1, 0, 0, 0, -1, 0, 0, 0 )` */
		static readonly FLIP_Z: Transform;


		/** The basis is a matrix containing 3 `Vector3` as its columns: X axis, Y axis, and Z axis. These vectors can be interpreted as the basis vectors of local coordinate system traveling with the object. */
		basis: Basis;

		/** The translation offset of the transform. */
		origin: Vector3;


		/** Returns the inverse of the transform, under the assumption that the transformation is composed of rotation, scaling and translation. */
		affine_inverse() : Transform;

		/** Interpolates the transform to other Transform by weight amount (0-1). */
		interpolate_with(transform: Transform, weight: number) : Transform;

		/** Returns the inverse of the transform, under the assumption that the transformation is composed of rotation and translation (no scaling, use affine_inverse for transforms with scaling). */
		inverse() : Transform;

		/**  */
		is_equal_approx(transform: Transform) : boolean;

		/** Returns a copy of the transform rotated such that its -Z axis points towards the `target` position.

		 The transform will first be rotated around the given `up` vector, and then fully aligned to the target by a further rotation around an axis perpendicular to both the `target` and `up` vectors.

		 Operations take place in global space. */
		looking_at(target: Vector3, up: Vector3) : Transform;

		/** Returns the transform with the basis orthogonal (90 degrees), and normalized axis vectors. */
		orthonormalized() : Transform;

		/** Rotates the transform around given axis by phi. The axis must be a normalized vector. */
		rotated(axis: Vector3, phi: number) : Transform;

		/** Scales the transform by the specified 3D scaling factors. */
		scaled(scale: Vector3) : Transform;

		/** Translates the transform by the specified offset. */
		translated(ofs: Vector3) : Transform;

		/** Transforms the given `Vector3`, `Plane`, `AABB`, or `PoolVector3Array` by this transform. */
		xform(v: any) : any;

		/** Inverse-transforms the given `Vector3`, `Plane`, `AABB`, or `PoolVector3Array` by this transform. */
		xform_inv(v: any) : any;

	}

	/** 2D transformation (3×2 matrix).
	 Represents one or many transformations in 2D space such as translation, rotation, or scaling. It consists of two `x` and `y` `Vector2`s and an `origin`. It is similar to a 3×2 matrix. */
	class Transform2D {
		
		constructor(t2_or_e0?: Transform2D|Vector2|Transform|number, v1?: Vector2, v2?: Vector2);

		/** `Transform2D` with no translation, rotation or scaling applied. When applied to other data structures, `IDENTITY` performs no transformation. 
		 * @value `Transform2D( 1, 0, 0, 1, 0, 0 )` */
		static readonly IDENTITY: Transform2D;

		/** `Transform2D` with mirroring applied parallel to the X axis. 
		 * @value `Transform2D( -1, 0, 0, 1, 0, 0 )` */
		static readonly FLIP_X: Transform2D;

		/** `Transform2D` with mirroring applied parallel to the Y axis. 
		 * @value `Transform2D( 1, 0, 0, -1, 0, 0 )` */
		static readonly FLIP_Y: Transform2D;


		/** The X axis of 2×2 basis matrix containing 2 `Vector2`s as its columns: X axis and Y axis. These vectors can be interpreted as the basis vectors of local coordinate system traveling with the object. */
		x: Vector2;

		/** The Y axis of 2×2 basis matrix containing 2 `Vector2`s as its columns: X axis and Y axis. These vectors can be interpreted as the basis vectors of local coordinate system traveling with the object. */
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

		/**  */
		is_equal_approx(transform: Transform2D) : boolean;

		/** Returns the transform with the basis orthogonal (90 degrees), and normalized axis vectors. */
		orthonormalized() : Transform2D;

		/** Rotates the transform by the given angle (in radians). */
		rotated(phi: number) : Transform2D;

		/** Scales the transform by the given factor. */
		scaled(scale: Vector2) : Transform2D;

		/** Translates the transform by the given offset. */
		translated(offset: Vector2) : Transform2D;

		/** Transforms the given `Vector2`, `Rect2`, or `PoolVector2Array` by this transform. */
		xform(v: any) : any;

		/** Inverse-transforms the given `Vector2`, `Rect2`, or `PoolVector2Array` by this transform. */
		xform_inv(v: any) : any;

	}
	
	/** Color in RGBA format with some support for ARGB format.
	 A color is represented by red, green, and blue `(r, g, b)` components. Additionally, `a` represents the alpha component, often used for transparency. Values are in floating-point and usually range from 0 to 1. Some properties (such as `CanvasItem.modulate`) may accept values greater than 1.

	 You can also create a color from standardized color names by using `@GDScript.ColorN`. */
	class Color {
		
		constructor(r_or_code_or_color: number| string | Color, g?: number, b?: number, a?: number);

		/**  
		 * @value `Color( 0.75, 0.75, 0.75, 1 )` */
		static readonly gray: Color;

		/**  
		 * @value `Color( 0.94, 0.97, 1, 1 )` */
		static readonly aliceblue: Color;

		/**  
		 * @value `Color( 0.98, 0.92, 0.84, 1 )` */
		static readonly antiquewhite: Color;

		/**  
		 * @value `Color( 0, 1, 1, 1 )` */
		static readonly aqua: Color;

		/**  
		 * @value `Color( 0.5, 1, 0.83, 1 )` */
		static readonly aquamarine: Color;

		/**  
		 * @value `Color( 0.94, 1, 1, 1 )` */
		static readonly azure: Color;

		/**  
		 * @value `Color( 0.96, 0.96, 0.86, 1 )` */
		static readonly beige: Color;

		/**  
		 * @value `Color( 1, 0.89, 0.77, 1 )` */
		static readonly bisque: Color;

		/**  
		 * @value `Color( 0, 0, 0, 1 )` */
		static readonly black: Color;

		/**  
		 * @value `Color( 1, 0.92, 0.8, 1 )` */
		static readonly blanchedalmond: Color;

		/**  
		 * @value `Color( 0, 0, 1, 1 )` */
		static readonly blue: Color;

		/**  
		 * @value `Color( 0.54, 0.17, 0.89, 1 )` */
		static readonly blueviolet: Color;

		/**  
		 * @value `Color( 0.65, 0.16, 0.16, 1 )` */
		static readonly brown: Color;

		/**  
		 * @value `Color( 0.87, 0.72, 0.53, 1 )` */
		static readonly burlywood: Color;

		/**  
		 * @value `Color( 0.37, 0.62, 0.63, 1 )` */
		static readonly cadetblue: Color;

		/**  
		 * @value `Color( 0.5, 1, 0, 1 )` */
		static readonly chartreuse: Color;

		/**  
		 * @value `Color( 0.82, 0.41, 0.12, 1 )` */
		static readonly chocolate: Color;

		/**  
		 * @value `Color( 1, 0.5, 0.31, 1 )` */
		static readonly coral: Color;

		/**  
		 * @value `Color( 0.39, 0.58, 0.93, 1 )` */
		static readonly cornflower: Color;

		/**  
		 * @value `Color( 1, 0.97, 0.86, 1 )` */
		static readonly cornsilk: Color;

		/**  
		 * @value `Color( 0.86, 0.08, 0.24, 1 )` */
		static readonly crimson: Color;

		/**  
		 * @value `Color( 0, 1, 1, 1 )` */
		static readonly cyan: Color;

		/**  
		 * @value `Color( 0, 0, 0.55, 1 )` */
		static readonly darkblue: Color;

		/**  
		 * @value `Color( 0, 0.55, 0.55, 1 )` */
		static readonly darkcyan: Color;

		/**  
		 * @value `Color( 0.72, 0.53, 0.04, 1 )` */
		static readonly darkgoldenrod: Color;

		/**  
		 * @value `Color( 0.66, 0.66, 0.66, 1 )` */
		static readonly darkgray: Color;

		/**  
		 * @value `Color( 0, 0.39, 0, 1 )` */
		static readonly darkgreen: Color;

		/**  
		 * @value `Color( 0.74, 0.72, 0.42, 1 )` */
		static readonly darkkhaki: Color;

		/**  
		 * @value `Color( 0.55, 0, 0.55, 1 )` */
		static readonly darkmagenta: Color;

		/**  
		 * @value `Color( 0.33, 0.42, 0.18, 1 )` */
		static readonly darkolivegreen: Color;

		/**  
		 * @value `Color( 1, 0.55, 0, 1 )` */
		static readonly darkorange: Color;

		/**  
		 * @value `Color( 0.6, 0.2, 0.8, 1 )` */
		static readonly darkorchid: Color;

		/**  
		 * @value `Color( 0.55, 0, 0, 1 )` */
		static readonly darkred: Color;

		/**  
		 * @value `Color( 0.91, 0.59, 0.48, 1 )` */
		static readonly darksalmon: Color;

		/**  
		 * @value `Color( 0.56, 0.74, 0.56, 1 )` */
		static readonly darkseagreen: Color;

		/**  
		 * @value `Color( 0.28, 0.24, 0.55, 1 )` */
		static readonly darkslateblue: Color;

		/**  
		 * @value `Color( 0.18, 0.31, 0.31, 1 )` */
		static readonly darkslategray: Color;

		/**  
		 * @value `Color( 0, 0.81, 0.82, 1 )` */
		static readonly darkturquoise: Color;

		/**  
		 * @value `Color( 0.58, 0, 0.83, 1 )` */
		static readonly darkviolet: Color;

		/**  
		 * @value `Color( 1, 0.08, 0.58, 1 )` */
		static readonly deeppink: Color;

		/**  
		 * @value `Color( 0, 0.75, 1, 1 )` */
		static readonly deepskyblue: Color;

		/**  
		 * @value `Color( 0.41, 0.41, 0.41, 1 )` */
		static readonly dimgray: Color;

		/**  
		 * @value `Color( 0.12, 0.56, 1, 1 )` */
		static readonly dodgerblue: Color;

		/**  
		 * @value `Color( 0.7, 0.13, 0.13, 1 )` */
		static readonly firebrick: Color;

		/**  
		 * @value `Color( 1, 0.98, 0.94, 1 )` */
		static readonly floralwhite: Color;

		/**  
		 * @value `Color( 0.13, 0.55, 0.13, 1 )` */
		static readonly forestgreen: Color;

		/**  
		 * @value `Color( 1, 0, 1, 1 )` */
		static readonly fuchsia: Color;

		/**  
		 * @value `Color( 0.86, 0.86, 0.86, 1 )` */
		static readonly gainsboro: Color;

		/**  
		 * @value `Color( 0.97, 0.97, 1, 1 )` */
		static readonly ghostwhite: Color;

		/**  
		 * @value `Color( 1, 0.84, 0, 1 )` */
		static readonly gold: Color;

		/**  
		 * @value `Color( 0.85, 0.65, 0.13, 1 )` */
		static readonly goldenrod: Color;

		/**  
		 * @value `Color( 0, 1, 0, 1 )` */
		static readonly green: Color;

		/**  
		 * @value `Color( 0.68, 1, 0.18, 1 )` */
		static readonly greenyellow: Color;

		/**  
		 * @value `Color( 0.94, 1, 0.94, 1 )` */
		static readonly honeydew: Color;

		/**  
		 * @value `Color( 1, 0.41, 0.71, 1 )` */
		static readonly hotpink: Color;

		/**  
		 * @value `Color( 0.8, 0.36, 0.36, 1 )` */
		static readonly indianred: Color;

		/**  
		 * @value `Color( 0.29, 0, 0.51, 1 )` */
		static readonly indigo: Color;

		/**  
		 * @value `Color( 1, 1, 0.94, 1 )` */
		static readonly ivory: Color;

		/**  
		 * @value `Color( 0.94, 0.9, 0.55, 1 )` */
		static readonly khaki: Color;

		/**  
		 * @value `Color( 0.9, 0.9, 0.98, 1 )` */
		static readonly lavender: Color;

		/**  
		 * @value `Color( 1, 0.94, 0.96, 1 )` */
		static readonly lavenderblush: Color;

		/**  
		 * @value `Color( 0.49, 0.99, 0, 1 )` */
		static readonly lawngreen: Color;

		/**  
		 * @value `Color( 1, 0.98, 0.8, 1 )` */
		static readonly lemonchiffon: Color;

		/**  
		 * @value `Color( 0.68, 0.85, 0.9, 1 )` */
		static readonly lightblue: Color;

		/**  
		 * @value `Color( 0.94, 0.5, 0.5, 1 )` */
		static readonly lightcoral: Color;

		/**  
		 * @value `Color( 0.88, 1, 1, 1 )` */
		static readonly lightcyan: Color;

		/**  
		 * @value `Color( 0.98, 0.98, 0.82, 1 )` */
		static readonly lightgoldenrod: Color;

		/**  
		 * @value `Color( 0.83, 0.83, 0.83, 1 )` */
		static readonly lightgray: Color;

		/**  
		 * @value `Color( 0.56, 0.93, 0.56, 1 )` */
		static readonly lightgreen: Color;

		/**  
		 * @value `Color( 1, 0.71, 0.76, 1 )` */
		static readonly lightpink: Color;

		/**  
		 * @value `Color( 1, 0.63, 0.48, 1 )` */
		static readonly lightsalmon: Color;

		/**  
		 * @value `Color( 0.13, 0.7, 0.67, 1 )` */
		static readonly lightseagreen: Color;

		/**  
		 * @value `Color( 0.53, 0.81, 0.98, 1 )` */
		static readonly lightskyblue: Color;

		/**  
		 * @value `Color( 0.47, 0.53, 0.6, 1 )` */
		static readonly lightslategray: Color;

		/**  
		 * @value `Color( 0.69, 0.77, 0.87, 1 )` */
		static readonly lightsteelblue: Color;

		/**  
		 * @value `Color( 1, 1, 0.88, 1 )` */
		static readonly lightyellow: Color;

		/**  
		 * @value `Color( 0, 1, 0, 1 )` */
		static readonly lime: Color;

		/**  
		 * @value `Color( 0.2, 0.8, 0.2, 1 )` */
		static readonly limegreen: Color;

		/**  
		 * @value `Color( 0.98, 0.94, 0.9, 1 )` */
		static readonly linen: Color;

		/**  
		 * @value `Color( 1, 0, 1, 1 )` */
		static readonly magenta: Color;

		/**  
		 * @value `Color( 0.69, 0.19, 0.38, 1 )` */
		static readonly maroon: Color;

		/**  
		 * @value `Color( 0.4, 0.8, 0.67, 1 )` */
		static readonly mediumaquamarine: Color;

		/**  
		 * @value `Color( 0, 0, 0.8, 1 )` */
		static readonly mediumblue: Color;

		/**  
		 * @value `Color( 0.73, 0.33, 0.83, 1 )` */
		static readonly mediumorchid: Color;

		/**  
		 * @value `Color( 0.58, 0.44, 0.86, 1 )` */
		static readonly mediumpurple: Color;

		/**  
		 * @value `Color( 0.24, 0.7, 0.44, 1 )` */
		static readonly mediumseagreen: Color;

		/**  
		 * @value `Color( 0.48, 0.41, 0.93, 1 )` */
		static readonly mediumslateblue: Color;

		/**  
		 * @value `Color( 0, 0.98, 0.6, 1 )` */
		static readonly mediumspringgreen: Color;

		/**  
		 * @value `Color( 0.28, 0.82, 0.8, 1 )` */
		static readonly mediumturquoise: Color;

		/**  
		 * @value `Color( 0.78, 0.08, 0.52, 1 )` */
		static readonly mediumvioletred: Color;

		/**  
		 * @value `Color( 0.1, 0.1, 0.44, 1 )` */
		static readonly midnightblue: Color;

		/**  
		 * @value `Color( 0.96, 1, 0.98, 1 )` */
		static readonly mintcream: Color;

		/**  
		 * @value `Color( 1, 0.89, 0.88, 1 )` */
		static readonly mistyrose: Color;

		/**  
		 * @value `Color( 1, 0.89, 0.71, 1 )` */
		static readonly moccasin: Color;

		/**  
		 * @value `Color( 1, 0.87, 0.68, 1 )` */
		static readonly navajowhite: Color;

		/**  
		 * @value `Color( 0, 0, 0.5, 1 )` */
		static readonly navyblue: Color;

		/**  
		 * @value `Color( 0.99, 0.96, 0.9, 1 )` */
		static readonly oldlace: Color;

		/**  
		 * @value `Color( 0.5, 0.5, 0, 1 )` */
		static readonly olive: Color;

		/**  
		 * @value `Color( 0.42, 0.56, 0.14, 1 )` */
		static readonly olivedrab: Color;

		/**  
		 * @value `Color( 1, 0.65, 0, 1 )` */
		static readonly orange: Color;

		/**  
		 * @value `Color( 1, 0.27, 0, 1 )` */
		static readonly orangered: Color;

		/**  
		 * @value `Color( 0.85, 0.44, 0.84, 1 )` */
		static readonly orchid: Color;

		/**  
		 * @value `Color( 0.93, 0.91, 0.67, 1 )` */
		static readonly palegoldenrod: Color;

		/**  
		 * @value `Color( 0.6, 0.98, 0.6, 1 )` */
		static readonly palegreen: Color;

		/**  
		 * @value `Color( 0.69, 0.93, 0.93, 1 )` */
		static readonly paleturquoise: Color;

		/**  
		 * @value `Color( 0.86, 0.44, 0.58, 1 )` */
		static readonly palevioletred: Color;

		/**  
		 * @value `Color( 1, 0.94, 0.84, 1 )` */
		static readonly papayawhip: Color;

		/**  
		 * @value `Color( 1, 0.85, 0.73, 1 )` */
		static readonly peachpuff: Color;

		/**  
		 * @value `Color( 0.8, 0.52, 0.25, 1 )` */
		static readonly peru: Color;

		/**  
		 * @value `Color( 1, 0.75, 0.8, 1 )` */
		static readonly pink: Color;

		/**  
		 * @value `Color( 0.87, 0.63, 0.87, 1 )` */
		static readonly plum: Color;

		/**  
		 * @value `Color( 0.69, 0.88, 0.9, 1 )` */
		static readonly powderblue: Color;

		/**  
		 * @value `Color( 0.63, 0.13, 0.94, 1 )` */
		static readonly purple: Color;

		/**  
		 * @value `Color( 0.4, 0.2, 0.6, 1 )` */
		static readonly rebeccapurple: Color;

		/**  
		 * @value `Color( 1, 0, 0, 1 )` */
		static readonly red: Color;

		/**  
		 * @value `Color( 0.74, 0.56, 0.56, 1 )` */
		static readonly rosybrown: Color;

		/**  
		 * @value `Color( 0.25, 0.41, 0.88, 1 )` */
		static readonly royalblue: Color;

		/**  
		 * @value `Color( 0.55, 0.27, 0.07, 1 )` */
		static readonly saddlebrown: Color;

		/**  
		 * @value `Color( 0.98, 0.5, 0.45, 1 )` */
		static readonly salmon: Color;

		/**  
		 * @value `Color( 0.96, 0.64, 0.38, 1 )` */
		static readonly sandybrown: Color;

		/**  
		 * @value `Color( 0.18, 0.55, 0.34, 1 )` */
		static readonly seagreen: Color;

		/**  
		 * @value `Color( 1, 0.96, 0.93, 1 )` */
		static readonly seashell: Color;

		/**  
		 * @value `Color( 0.63, 0.32, 0.18, 1 )` */
		static readonly sienna: Color;

		/**  
		 * @value `Color( 0.75, 0.75, 0.75, 1 )` */
		static readonly silver: Color;

		/**  
		 * @value `Color( 0.53, 0.81, 0.92, 1 )` */
		static readonly skyblue: Color;

		/**  
		 * @value `Color( 0.42, 0.35, 0.8, 1 )` */
		static readonly slateblue: Color;

		/**  
		 * @value `Color( 0.44, 0.5, 0.56, 1 )` */
		static readonly slategray: Color;

		/**  
		 * @value `Color( 1, 0.98, 0.98, 1 )` */
		static readonly snow: Color;

		/**  
		 * @value `Color( 0, 1, 0.5, 1 )` */
		static readonly springgreen: Color;

		/**  
		 * @value `Color( 0.27, 0.51, 0.71, 1 )` */
		static readonly steelblue: Color;

		/**  
		 * @value `Color( 0.82, 0.71, 0.55, 1 )` */
		static readonly tan: Color;

		/**  
		 * @value `Color( 0, 0.5, 0.5, 1 )` */
		static readonly teal: Color;

		/**  
		 * @value `Color( 0.85, 0.75, 0.85, 1 )` */
		static readonly thistle: Color;

		/**  
		 * @value `Color( 1, 0.39, 0.28, 1 )` */
		static readonly tomato: Color;

		/**  
		 * @value `Color( 1, 1, 1, 0 )` */
		static readonly transparent: Color;

		/**  
		 * @value `Color( 0.25, 0.88, 0.82, 1 )` */
		static readonly turquoise: Color;

		/**  
		 * @value `Color( 0.93, 0.51, 0.93, 1 )` */
		static readonly violet: Color;

		/**  
		 * @value `Color( 0.5, 0.5, 0.5, 1 )` */
		static readonly webgray: Color;

		/**  
		 * @value `Color( 0, 0.5, 0, 1 )` */
		static readonly webgreen: Color;

		/**  
		 * @value `Color( 0.5, 0, 0, 1 )` */
		static readonly webmaroon: Color;

		/**  
		 * @value `Color( 0.5, 0, 0.5, 1 )` */
		static readonly webpurple: Color;

		/**  
		 * @value `Color( 0.96, 0.87, 0.7, 1 )` */
		static readonly wheat: Color;

		/**  
		 * @value `Color( 1, 1, 1, 1 )` */
		static readonly white: Color;

		/**  
		 * @value `Color( 0.96, 0.96, 0.96, 1 )` */
		static readonly whitesmoke: Color;

		/**  
		 * @value `Color( 1, 1, 0, 1 )` */
		static readonly yellow: Color;

		/**  
		 * @value `Color( 0.6, 0.8, 0.2, 1 )` */
		static readonly yellowgreen: Color;


		/** Red value (range 0 to 1). */
		r: number;

		/** Green value (range 0 to 1). */
		g: number;

		/** Blue value (range 0 to 1). */
		b: number;

		/** Alpha value (range 0 to 1). */
		a: number;

		/** HSV hue value (range 0 to 1). */
		h: number;

		/** HSV saturation value (range 0 to 1). */
		s: number;

		/** HSV value (range 0 to 1). */
		v: number;

		/** Red value (range 0 to 255). */
		r8: number;

		/** Green value (range 0 to 255). */
		g8: number;

		/** Blue value (range 0 to 255). */
		b8: number;

		/** Alpha value (range 0 to 255). */
		a8: number;


		/** Returns a new color resulting from blending this color over another. If the color is opaque, the result is also opaque. The second color may have a range of alpha values.

		 

		 	var bg = Color(0.0, 1.0, 0.0, 0.5) # Green with alpha of 50%
		 	var fg = Color(1.0, 0.0, 0.0, 0.5) # Red with alpha of 50%
		 	var blended_color = bg.blend(fg) # Brown with alpha of 75%
		  */
		blend(over: Color) : Color;

		/** Returns the most contrasting color.

		 

		 	var c = Color(0.3, 0.4, 0.9)
		 	var contrasted_color = c.contrasted() # Equivalent to RGBA(204, 229, 102, 255)
		  */
		contrasted() : Color;

		/** Returns a new color resulting from making this color darker by the specified percentage (ratio from 0 to 1).

		 

		 	var green = Color(0.0, 1.0, 0.0)
		 	var darkgreen = green.darkened(0.2) # 20% darker than regular green
		  */
		darkened(amount: number) : Color;

		/** Constructs a color from an HSV profile. `h`, `s`, and `v` are values between 0 and 1.

		 

		 	var c = Color.from_hsv(0.58, 0.5, 0.79, 0.8) # Equivalent to HSV(210, 50, 79, 0.8) or Color8(100, 151, 201, 0.8)
		  */
		from_hsv(h: number, s: number, v: number, a?: number) : Color;

		/** Returns the color's grayscale representation.

		 The gray value is calculated as `(r + g + b) / 3`.

		 

		 	var c = Color(0.2, 0.45, 0.82)
		 	var gray = c.gray() # A value of 0.466667
		  */
		gray() : number;

		/** Returns the inverted color `(1 - r, 1 - g, 1 - b, a)`.

		 

		 	var c = Color(0.3, 0.4, 0.9)
		 	var inverted_color = c.inverted() # A color of an RGBA(178, 153, 26, 255)
		  */
		inverted() : Color;

		/**  */
		is_equal_approx(color: Color) : boolean;

		/** Returns a new color resulting from making this color lighter by the specified percentage (ratio from 0 to 1).

		 

		 	var green = Color(0.0, 1.0, 0.0)
		 	var lightgreen = green.lightened(0.2) # 20% lighter than regular green
		  */
		lightened(amount: number) : Color;

		/** Returns the linear interpolation with another color. The interpolation factor `t` is between 0 and 1.

		 

		 	var c1 = Color(1.0, 0.0, 0.0)
		 	var c2 = Color(0.0, 1.0, 0.0)
		 	var li_c = c1.linear_interpolate(c2, 0.5) # A color of an RGBA(128, 128, 0, 255)
		  */
		linear_interpolate(b: Color, t: number) : Color;

		/** Returns the color's 32-bit integer in ABGR format (each byte represents a component of the ABGR profile). ABGR is the reversed version of the default format.

		 

		 	var c = Color(1, 0.5, 0.2)
		 	print(c.to_abgr32()) # Prints 4281565439
		  */
		to_abgr32() : number;

		/** Returns the color's 64-bit integer in ABGR format (each word represents a component of the ABGR profile). ABGR is the reversed version of the default format.

		 

		 	var c = Color(1, 0.5, 0.2)
		 	print(c.to_abgr64()) # Prints -225178692812801
		  */
		to_abgr64() : number;

		/** Returns the color's 32-bit integer in ARGB format (each byte represents a component of the ARGB profile). ARGB is more compatible with DirectX.

		 

		 	var c = Color(1, 0.5, 0.2)
		 	print(c.to_argb32()) # Prints 4294934323
		  */
		to_argb32() : number;

		/** Returns the color's 64-bit integer in ARGB format (each word represents a component of the ARGB profile). ARGB is more compatible with DirectX.

		 

		 	var c = Color(1, 0.5, 0.2)
		 	print(c.to_argb64()) # Prints -2147470541
		  */
		to_argb64() : number;

		/** Returns the color's HTML hexadecimal color string in ARGB format (ex: `ff34f822`).

		 Setting `with_alpha` to `false` excludes alpha from the hexadecimal string.

		 

		 	var c = Color(1, 1, 1, 0.5)
		 	var s1 = c.to_html() # Returns "7fffffff"
		 	var s2 = c.to_html(false) # Returns "ffffff"
		  */
		to_html(with_alpha?: boolean) : string;

		/** Returns the color's 32-bit integer in RGBA format (each byte represents a component of the RGBA profile). RGBA is Godot's default format.

		 

		 	var c = Color(1, 0.5, 0.2)
		 	print(c.to_rgba32()) # Prints 4286526463
		  */
		to_rgba32() : number;

		/** Returns the color's 64-bit integer in RGBA format (each word represents a component of the RGBA profile). RGBA is Godot's default format.

		 

		 	var c = Color(1, 0.5, 0.2)
		 	print(c.to_rgba64()) # Prints -140736629309441
		  */
		to_rgba64() : number;

	}
	
	/** Handle for a `Resource`'s unique ID.
	 The RID type is used to access the unique integer ID of a resource. They are opaque, which means they do not grant access to the associated resource by themselves. They are used by and with the low-level Server classes such as `VisualServer`. */
	class RID {




		/** Returns the ID of the referenced resource. */
		get_id() : number;

	}
	
	/** Plane in hessian form.
	 Plane represents a normalized plane equation. Basically, "normal" is the normal of the plane (a,b,c normalized), and "d" is the distance from the origin to the plane (in the direction of "normal"). "Over" or "Above" the plane is considered the side of the plane towards where the normal is pointing. */
	class Plane {

		constructor(x_or_plane?: number| Plane | Vector3, y?: number | Vector3, z?: number | Vector3, d?: number);
		
		/**  
		 * @value `Plane( 1, 0, 0, 0 )` */
		static readonly PLANE_YZ: Plane;

		/**  
		 * @value `Plane( 0, 1, 0, 0 )` */
		static readonly PLANE_XZ: Plane;

		/**  
		 * @value `Plane( 0, 0, 1, 0 )` */
		static readonly PLANE_XY: Plane;


		/**  */
		normal: Vector3;

		/**  */
		x: number;

		/**  */
		y: number;

		/**  */
		z: number;

		/**  */
		d: number;


		/** Returns the center of the plane. */
		center() : Vector3;

		/** Returns the shortest distance from the plane to the position `point`. */
		distance_to(point: Vector3) : number;

		/** Returns a point on the plane. */
		get_any_point() : Vector3;

		/** Returns `true` if `point` is inside the plane (by a very minimum `epsilon` threshold). */
		has_point(point: Vector3, epsilon?: number) : boolean;

		/** Returns the intersection point of the three planes `b`, `c` and this plane. If no intersection is found, `null` is returned. */
		intersect_3(b: Plane, c: Plane) : Vector3;

		/** Returns the intersection point of a ray consisting of the position `from` and the direction normal `dir` with this plane. If no intersection is found, `null` is returned. */
		intersects_ray(p_from: Vector3, dir: Vector3) : Vector3;

		/** Returns the intersection point of a segment from position `begin` to position `end` with this plane. If no intersection is found, `null` is returned. */
		intersects_segment(begin: Vector3, end: Vector3) : Vector3;

		/**  */
		is_equal_approx(plane: Plane) : boolean;

		/** Returns `true` if `point` is located above the plane. */
		is_point_over(point: Vector3) : boolean;

		/** Returns a copy of the plane, normalized. */
		normalized() : Plane;

		/** Returns the orthogonal projection of point `p` into a point in the plane. */
		project(point: Vector3) : Vector3;

	}
	
	/** Quaternion.
	 A unit quaternion used for representing 3D rotations.

	 It is similar to `Basis`, which implements matrix representation of rotations, and can be parametrized using both an axis-angle pair or Euler angles. But due to its compactness and the way it is stored in memory, certain operations (obtaining axis-angle and performing SLERP, in particular) are more efficient and robust against floating-point errors.

	 Quaternions need to be (re)normalized. */
	class Quat {

		constructor(x_or_quat_or_dir_basis?: number|Quat|Vector3|Basis, y_or_w?: number, z?: number, w?: number);

		/**  
		 * @value `Quat( 0, 0, 0, 1 )` */
		static readonly IDENTITY: Quat;


		/** X component of the quaternion. */
		x: number;

		/** Y component of the quaternion. */
		y: number;

		/** Z component of the quaternion. */
		z: number;

		/** W component of the quaternion. */
		w: number;


		/** Performs a cubic spherical-linear interpolation with another quaternion. */
		cubic_slerp(b: Quat, pre_a: Quat, post_b: Quat, t: number) : Quat;

		/** Returns the dot product of two quaternions. */
		dot(b: Quat) : number;

		/** Returns Euler angles (in the YXZ convention: first Z, then X, and Y last) corresponding to the rotation represented by the unit quaternion. Returned vector contains the rotation angles in the format (X angle, Y angle, Z angle). */
		get_euler() : Vector3;

		/** Returns the inverse of the quaternion. */
		inverse() : Quat;

		/**  */
		is_equal_approx(quat: Quat) : boolean;

		/** Returns whether the quaternion is normalized or not. */
		is_normalized() : boolean;

		/** Returns the length of the quaternion. */
		length() : number;

		/** Returns the length of the quaternion, squared. */
		length_squared() : number;

		/** Returns a copy of the quaternion, normalized to unit length. */
		normalized() : Quat;

		/** Sets the quaternion to a rotation which rotates around axis by the specified angle, in radians. The axis must be a normalized vector. */
		set_axis_angle(axis: Vector3, angle: number) : void;

		/** Sets the quaternion to a rotation specified by Euler angles (in the YXZ convention: first Z, then X, and Y last), given in the vector format as (X angle, Y angle, Z angle). */
		set_euler(euler: Vector3) : void;

		/** Performs a spherical-linear interpolation with another quaternion. */
		slerp(b: Quat, t: number) : Quat;

		/** Performs a spherical-linear interpolation with another quaterion without checking if the rotation path is not bigger than 90°. */
		slerpni(b: Quat, t: number) : Quat;

		/** Transforms the vector `v` by this quaternion. */
		xform(v: Vector3) : Vector3;

	}
	
	/** Axis-Aligned Bounding Box.
	 AABB consists of a position, a size, and several utility functions. It is typically used for fast overlap tests. */
	class AABB {
		
		constructor(position_or_aabb?: Vector3 | AABB, size?: Vector3);


		/** Beginning corner. */
		position: Vector3;

		/** Size from position to end. */
		size: Vector3;

		/** Ending corner. */
		end: Vector3;


		/** Returns `true` if this `AABB` completely encloses another one. */
		encloses(p_with: AABB) : boolean;

		/** Returns this `AABB` expanded to include a given point. */
		expand(to_point: Vector3) : AABB;

		/** Gets the area of the `AABB`. */
		get_area() : number;

		/** Gets the position of the 8 endpoints of the `AABB` in space. */
		get_endpoint(idx: number) : Vector3;

		/** Returns the normalized longest axis of the `AABB`. */
		get_longest_axis() : Vector3;

		/** Returns the index of the longest axis of the `AABB` (according to `Vector3`::AXIS* enum). */
		get_longest_axis_index() : number;

		/** Returns the scalar length of the longest axis of the `AABB`. */
		get_longest_axis_size() : number;

		/** Returns the normalized shortest axis of the `AABB`. */
		get_shortest_axis() : Vector3;

		/** Returns the index of the shortest axis of the `AABB` (according to `Vector3`::AXIS* enum). */
		get_shortest_axis_index() : number;

		/** Returns the scalar length of the shortest axis of the `AABB`. */
		get_shortest_axis_size() : number;

		/** Returns the support point in a given direction. This is useful for collision detection algorithms. */
		get_support(dir: Vector3) : Vector3;

		/** Returns a copy of the `AABB` grown a given amount of units towards all the sides. */
		grow(by: number) : AABB;

		/** Returns `true` if the `AABB` is flat or empty. */
		has_no_area() : boolean;

		/** Returns `true` if the `AABB` is empty. */
		has_no_surface() : boolean;

		/** Returns `true` if the `AABB` contains a point. */
		has_point(point: Vector3) : boolean;

		/** Returns the intersection between two `AABB`. An empty AABB (size 0,0,0) is returned on failure. */
		intersection(p_with: AABB) : AABB;

		/** Returns `true` if the `AABB` overlaps with another. */
		intersects(p_with: AABB) : boolean;

		/** Returns `true` if the `AABB` is on both sides of a plane. */
		intersects_plane(plane: Plane) : boolean;

		/** Returns `true` if the `AABB` intersects the line segment between `from` and `to`. */
		intersects_segment(p_from: Vector3, to: Vector3) : boolean;

		/**  */
		is_equal_approx(aabb: AABB) : boolean;

		/** Returns a larger AABB that contains this AABB and `with`. */
		merge(p_with: AABB) : AABB;

	}
	
	
	/** A pooled `Array` of bytes.
	 An `Array` specifically designed to hold bytes. Optimized for memory usage, does not fragment the memory.

	 **Note:** This type is passed by value and not by reference. */
	class PoolByteArray {
		
		constructor(source?: number[]);
		
		/** Appends an element at the end of the array (alias of `push_back`). */
		append(byte: number) : void;

		/** Appends a `PoolByteArray` at the end of this array. */
		append_array(array: PoolByteArray) : void;

		/** Returns a new `PoolByteArray` with the data compressed. Set the compression mode using one of `File.CompressionMode`'s constants. */
		compress(compression_mode?: number) : PoolByteArray;

		/** Returns a new `PoolByteArray` with the data decompressed. Set `buffer_size` to the size of the uncompressed data. Set the compression mode using one of `File.CompressionMode`'s constants. */
		decompress(buffer_size: number, compression_mode?: number) : PoolByteArray;

		/** Returns `true` if the array is empty. */
		empty() : boolean;

		/** Returns a copy of the array's contents as `String`. Fast alternative to `get_string_from_utf8` if the content is ASCII-only. Unlike the UTF-8 function this function maps every byte to a character in the array. Multibyte sequences will not be interpreted correctly. For parsing user input always use `get_string_from_utf8`. */
		get_string_from_ascii() : string;

		/** Returns a copy of the array's contents as `String`. Slower than `get_string_from_ascii` but supports UTF-8 encoded data. Use this function if you are unsure about the source of the data. For user input this function should always be preferred. */
		get_string_from_utf8() : string;

		/**  */
		hex_encode() : string;

		/** Inserts a new element at a given position in the array. The position must be valid, or at the end of the array (`idx == size()`). */
		insert(idx: number, byte: number) : number;

		/** Reverses the order of the elements in the array. */
		invert() : void;

		/** Appends an element at the end of the array. */
		push_back(byte: number) : void;

		/** Removes an element from the array by index. */
		remove(idx: number) : void;

		/** Sets the size of the array. If the array is grown, reserves elements at the end of the array. If the array is shrunk, truncates the array to the new size. */
		resize(idx: number) : void;

		/** Changes the byte at the given index. */
		set(idx: number, byte: number) : void;
		
		/** Get the `byte` at the given index */
		get(idx: number): number;

		/** Returns the size of the array. */
		size() : number;

		/** Returns the slice of the `PoolByteArray` between indices (inclusive) as a new `PoolByteArray`. Any negative index is considered to be from the end of the array. */
		subarray(p_from: number, to: number) : PoolByteArray;

	}

	/** A pooled `Array` of `Color`.
	 An `Array` specifically designed to hold `Color`. Optimized for memory usage, does not fragment the memory.

	 **Note:** This type is passed by value and not by reference. */
	class PoolColorArray {


		constructor(source?: Color[]);

		/** Appends an element at the end of the array (alias of `push_back`). */
		append(color: Color) : void;

		/** Appends a `PoolColorArray` at the end of this array. */
		append_array(array: PoolColorArray) : void;

		/** Returns `true` if the array is empty. */
		empty() : boolean;

		/** Inserts a new element at a given position in the array. The position must be valid, or at the end of the array (`idx == size()`). */
		insert(idx: number, color: Color) : number;

		/** Reverses the order of the elements in the array. */
		invert() : void;

		/** Appends a value to the array. */
		push_back(color: Color) : void;

		/** Removes an element from the array by index. */
		remove(idx: number) : void;

		/** Sets the size of the array. If the array is grown, reserves elements at the end of the array. If the array is shrunk, truncates the array to the new size. */
		resize(idx: number) : void;

		/** Changes the `Color` at the given index. */
		set(idx: number, color: Color) : void;
		
		/** Get the `Color` at the given index */
		get(idx: number): Color;

		/** Returns the size of the array. */
		size() : number;

	}

	/** A pooled `Array` of integers (`int`).
	 An `Array` specifically designed to hold integer values (`int`). Optimized for memory usage, does not fragment the memory.

	 **Note:** This type is passed by value and not by reference.

	 **Note:** This type is limited to signed 32-bit integers, which means it can only take values in the interval ``-2^31, 2^31 - 1``, i.e. ``-2147483648, 2147483647``. Exceeding those bounds will wrap around. In comparison, `int` uses signed 64-bit integers which can hold much larger values. */
	class PoolIntArray {


		constructor(source?: number[]);

		/** Appends an element at the end of the array (alias of `push_back`). */
		append(integer: number) : void;

		/** Appends a `PoolIntArray` at the end of this array. */
		append_array(array: PoolIntArray) : void;

		/** Returns `true` if the array is empty. */
		empty() : boolean;

		/** Inserts a new int at a given position in the array. The position must be valid, or at the end of the array (`idx == size()`). */
		insert(idx: number, integer: number) : number;

		/** Reverses the order of the elements in the array. */
		invert() : void;

		/** Appends a value to the array. */
		push_back(integer: number) : void;

		/** Removes an element from the array by index. */
		remove(idx: number) : void;

		/** Sets the size of the array. If the array is grown, reserves elements at the end of the array. If the array is shrunk, truncates the array to the new size. */
		resize(idx: number) : void;

		/** Changes the int at the given index. */
		set(idx: number, integer: number) : void;
		
		/** Get the `int` at the given index */
		get(idx: number): number;

		/** Returns the array size. */
		size() : number;

	}

	/** A pooled `Array` of reals (`float`).
	 An `Array` specifically designed to hold floating-point values (`float`). Optimized for memory usage, does not fragment the memory.

	 **Note:** This type is passed by value and not by reference. */
	class PoolRealArray {

		constructor(source?: number[]);

		/** Appends an element at the end of the array (alias of `push_back`). */
		append(value: number) : void;

		/** Appends a `PoolRealArray` at the end of this array. */
		append_array(array: PoolRealArray) : void;

		/** Returns `true` if the array is empty. */
		empty() : boolean;

		/** Inserts a new element at a given position in the array. The position must be valid, or at the end of the array (`idx == size()`). */
		insert(idx: number, value: number) : number;

		/** Reverses the order of the elements in the array. */
		invert() : void;

		/** Appends an element at the end of the array. */
		push_back(value: number) : void;

		/** Removes an element from the array by index. */
		remove(idx: number) : void;

		/** Sets the size of the array. If the array is grown, reserves elements at the end of the array. If the array is shrunk, truncates the array to the new size. */
		resize(idx: number) : void;

		/** Changes the float at the given index. */
		set(idx: number, value: number) : void;
		
		/** Get the `number` at the given index */
		get(idx: number): number;

		/** Returns the size of the array. */
		size() : number;

	}

	/** A pooled `Array` of `String`.
	 An `Array` specifically designed to hold `String`s. Optimized for memory usage, does not fragment the memory.

	 **Note:** This type is passed by value and not by reference. */
	class PoolStringArray {


		constructor(source?: string[]);

		/** Appends an element at the end of the array (alias of `push_back`). */
		append(p_string: string) : void;

		/** Appends a `PoolStringArray` at the end of this array. */
		append_array(array: PoolStringArray) : void;

		/** Returns `true` if the array is empty. */
		empty() : boolean;

		/** Inserts a new element at a given position in the array. The position must be valid, or at the end of the array (`idx == size()`). */
		insert(idx: number, p_string: string) : number;

		/** Reverses the order of the elements in the array. */
		invert() : void;

		/** Returns a `String` with each element of the array joined with the given `delimiter`. */
		join(delimiter: string) : string;

		/** Appends a string element at end of the array. */
		push_back(p_string: string) : void;

		/** Removes an element from the array by index. */
		remove(idx: number) : void;

		/** Sets the size of the array. If the array is grown, reserves elements at the end of the array. If the array is shrunk, truncates the array to the new size. */
		resize(idx: number) : void;

		/** Changes the `String` at the given index. */
		set(idx: number, p_string: string) : void;
		
		/** Get the `string` at the given index */
		get(idx: number): string;

		/** Returns the size of the array. */
		size() : number;

	}

	/** A pooled `Array` of `Vector2`.
	 An `Array` specifically designed to hold `Vector2`. Optimized for memory usage, does not fragment the memory.

	 **Note:** This type is passed by value and not by reference. */
	class PoolVector2Array {


		constructor(source?: Vector2[]);

		/** Appends an element at the end of the array (alias of `push_back`). */
		append(vector2: Vector2) : void;

		/** Appends a `PoolVector2Array` at the end of this array. */
		append_array(array: PoolVector2Array) : void;

		/** Returns `true` if the array is empty. */
		empty() : boolean;

		/** Inserts a new element at a given position in the array. The position must be valid, or at the end of the array (`idx == size()`). */
		insert(idx: number, vector2: Vector2) : number;

		/** Reverses the order of the elements in the array. */
		invert() : void;

		/** Inserts a `Vector2` at the end. */
		push_back(vector2: Vector2) : void;

		/** Removes an element from the array by index. */
		remove(idx: number) : void;

		/** Sets the size of the array. If the array is grown, reserves elements at the end of the array. If the array is shrunk, truncates the array to the new size. */
		resize(idx: number) : void;

		/** Changes the `Vector2` at the given index. */
		set(idx: number, vector2: Vector2) : void;
		
		/** Get the `Vector2` at the given index */
		get(idx: number): Vector2;

		/** Returns the size of the array. */
		size() : number;

	}

	/** A pooled `Array` of `Vector3`.
	 An `Array` specifically designed to hold `Vector3`. Optimized for memory usage, does not fragment the memory.

	 **Note:** This type is passed by value and not by reference. */
	class PoolVector3Array {


		constructor(source?: Vector3[]);

		/** Appends an element at the end of the array (alias of `push_back`). */
		append(vector3: Vector3) : void;

		/** Appends a `PoolVector3Array` at the end of this array. */
		append_array(array: PoolVector3Array) : void;

		/** Returns `true` if the array is empty. */
		empty() : boolean;

		/** Inserts a new element at a given position in the array. The position must be valid, or at the end of the array (`idx == size()`). */
		insert(idx: number, vector3: Vector3) : number;

		/** Reverses the order of the elements in the array. */
		invert() : void;

		/** Inserts a `Vector3` at the end. */
		push_back(vector3: Vector3) : void;

		/** Removes an element from the array by index. */
		remove(idx: number) : void;

		/** Sets the size of the array. If the array is grown, reserves elements at the end of the array. If the array is shrunk, truncates the array to the new size. */
		resize(idx: number) : void;

		/** Changes the `Vector3` at the given index. */
		set(idx: number, vector3: Vector3) : void;
		
		/** Get the `Vector3` at the given index */
		get(idx: number): Vector3;

		/** Returns the size of the array. */
		size() : number;

	}
}
