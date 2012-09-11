//
// Geometry shader source code to render fill walls (vertically extruded quads).
//

// "#extension" needs to be specified in the shader source *string* where it is used (this is not
// documented in the GLSL spec but is mentioned at http://www.opengl.org/wiki/GLSL_Core_Language).
#extension GL_EXT_geometry_shader4 : enable

// The depth range rendering is restricted to.
// If depth range is not restricted then this is the same as 'min_max_depth_radius'.
// Also the following conditions hold:
//	render_min_max_depth_radius_restriction.x >= min_max_depth_radius.x
//	render_min_max_depth_radius_restriction.y <= min_max_depth_radius.y
// ...in other words the depth range for rendering is always within the actual depth range.
uniform vec2 render_min_max_depth_radius_restriction;

#ifdef SURFACE_NORMALS
	// The world-space coordinates are interpolated across the wall geometry
	// so that the fragment shader can use them to lookup the surface fill mask texture.
	varying out vec3 world_position;

	// The surface normal for the front-face of the wall.
	varying out vec3 front_surface_normal;
#endif

void main (void)
{
	// The input primitive is a single segment (two vertices) of a GL_LINES.

	// Vertically extrude the line segment *start* surface point to the minimum and maximum depth radius of the scalar field.
	vec3 min_depth_start_point = render_min_max_depth_radius_restriction.x/*min*/ * gl_PositionIn[0].xyz;
	vec3 max_depth_start_point = render_min_max_depth_radius_restriction.y/*max*/ * gl_PositionIn[0].xyz;

	// Vertically extrude the line segment *end* surface point to the minimum and maximum depth radius of the scalar field.
	vec3 min_depth_end_point = render_min_max_depth_radius_restriction.x/*min*/ * gl_PositionIn[1].xyz;
	vec3 max_depth_end_point = render_min_max_depth_radius_restriction.y/*max*/ * gl_PositionIn[1].xyz;

#ifdef SURFACE_NORMALS
	// Calculate the front face normal.
	// The default for front-facing primitives is counter-clockwise - see glFrontFace (GLRenderer::gl_front_face).
	// So we need to make sure the surface normal calculated for the front face is the correct orientation
	// (ie, not the negative normal of the back face).
	//
	// Note that it might be possible to get a zero (or undefined due to 'normalize') normal vector if the
	// line segment points are too close together.
	// If this happens then the wall quad (two tris) will likely be close to degenerate and not generate any fragments anyway.
	// TODO: It might be better to detect this and not emit primitives.
	vec3 surface_normal = normalize(cross(gl_PositionIn[0].xyz, gl_PositionIn[1].xyz));
#endif

	//
	// Emit the wall quad as a triangle strip.
	//

	gl_Position = gl_ModelViewProjectionMatrix * vec4(min_depth_start_point, 1);
#ifdef SURFACE_NORMALS
	world_position = min_depth_start_point;
	front_surface_normal = surface_normal;
#endif
	EmitVertex();
	
	gl_Position = gl_ModelViewProjectionMatrix * vec4(max_depth_start_point, 1);
#ifdef SURFACE_NORMALS
	world_position = max_depth_start_point;
	front_surface_normal = surface_normal;
#endif
	EmitVertex();
	
	gl_Position = gl_ModelViewProjectionMatrix * vec4(min_depth_end_point, 1);
#ifdef SURFACE_NORMALS
	world_position = min_depth_end_point;
	front_surface_normal = surface_normal;
#endif
	EmitVertex();
	
	gl_Position = gl_ModelViewProjectionMatrix * vec4(max_depth_end_point, 1);
#ifdef SURFACE_NORMALS
	world_position = max_depth_end_point;
	front_surface_normal = surface_normal;
#endif
	EmitVertex();
}
