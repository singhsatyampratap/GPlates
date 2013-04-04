/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2012 The University of Sydney, Australia
 *
 * This file is part of GPlates.
 *
 * GPlates is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License, version 2, as published by
 * the Free Software Foundation.
 *
 * GPlates is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

#include "GLLight.h"

#include "GLContext.h"
#include "GLFrameBufferObject.h"
#include "GLRenderer.h"
#include "GLShaderProgramUtils.h"
#include "GLUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "maths/Vector3D.h"


namespace GPlatesOpenGL
{
	namespace
	{
		/**
		 * Vertex shader source code to render light direction into cube texture for a 2D map view.
		 */
		const QString RENDER_MAP_VIEW_LIGHT_DIRECTION_VERTEX_SHADER_SOURCE_FILE_NAME =
				":/opengl/light/render_map_view_light_direction_vertex_shader.glsl";

		/**
		 * Fragment shader source code to render light direction into cube texture for a 2D map view.
		 */
		const QString RENDER_MAP_VIEW_LIGHT_DIRECTION_FRAGMENT_SHADER_SOURCE_FILE_NAME =
				":/opengl/light/render_map_view_light_direction_fragment_shader.glsl";
	}
}


bool
GPlatesOpenGL::GLLight::is_supported(
		GLRenderer &renderer)
{
	static bool supported = false;

	// Only test for support the first time we're called.
	static bool tested_for_support = false;
	if (!tested_for_support)
	{
		tested_for_support = true;

		const GLCapabilities &capabilities = renderer.get_capabilities();

		// Need cube map texture and vertex/fragment shader and framebuffer object support.
		if (!capabilities.texture.gl_ARB_texture_cube_map ||
			!capabilities.shader.gl_ARB_vertex_shader ||
			!capabilities.shader.gl_ARB_fragment_shader ||
			!capabilities.framebuffer.gl_EXT_framebuffer_object)
		{
			return false;
		}

		//
		// Try to compile our most complex fragment shader program.
		// If that fails then it could be exceeding some resource limit on the runtime system
		// such as number of shader instructions allowed.
		//

		GLShaderProgramUtils::ShaderSource fragment_shader_source;
		fragment_shader_source.add_shader_source_from_file(
				RENDER_MAP_VIEW_LIGHT_DIRECTION_FRAGMENT_SHADER_SOURCE_FILE_NAME);
		
		GLShaderProgramUtils::ShaderSource vertex_shader_source;
		vertex_shader_source.add_shader_source_from_file(
				RENDER_MAP_VIEW_LIGHT_DIRECTION_VERTEX_SHADER_SOURCE_FILE_NAME);

		// Attempt to create the test shader program.
		if (!GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
				renderer, vertex_shader_source, fragment_shader_source))
		{
			return false;
		}

		// If we get this far then we have support.
		supported = true;
	}

	return supported;
}


GPlatesOpenGL::GLLight::non_null_ptr_type
GPlatesOpenGL::GLLight::create(
		GLRenderer &renderer,
		const GPlatesGui::SceneLightingParameters &scene_lighting_params,
		const GLMatrix &view_orientation,
		boost::optional<GPlatesGui::MapProjection::non_null_ptr_to_const_type> map_projection)
{
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			is_supported(renderer),
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(
			new GLLight(
					renderer,
					scene_lighting_params,
					view_orientation,
					map_projection));
}


GPlatesOpenGL::GLLight::GLLight(
		GLRenderer &renderer,
		const GPlatesGui::SceneLightingParameters &scene_lighting_params,
		const GLMatrix &view_orientation,
		boost::optional<GPlatesGui::MapProjection::non_null_ptr_to_const_type> map_projection) :
	d_scene_lighting_params(scene_lighting_params),
	d_view_orientation(view_orientation),
	d_globe_view_light_direction(scene_lighting_params.get_light_direction()/*not necessarily in world-space yet!*/),
	d_map_projection(map_projection),
	d_light_direction_cube_texture_dimension(256),
	d_map_view_light_direction_cube_texture(GLTexture::create(renderer))
{
	create_shader_programs(renderer);

	create_map_view_light_direction_cube_texture(renderer);

	if (d_map_projection)
	{
		update_map_view(renderer);
	}
	else
	{
		// Make sure globe-view light direction is in world-space (and not view-space).
		update_globe_view(renderer);
	}
}


void
GPlatesOpenGL::GLLight::set_scene_lighting(
		GLRenderer &renderer,
		const GPlatesGui::SceneLightingParameters &scene_lighting_params,
		const GLMatrix &view_orientation,
		boost::optional<GPlatesGui::MapProjection::non_null_ptr_to_const_type> map_projection)
{
	bool update = false;

	// If the map projection has changed in any way then we need to update.
	// This includes switching between globe and map views.
	if (map_projection != d_map_projection)
	{
		update = true;

		d_map_projection = map_projection;
	}

	if (view_orientation != d_view_orientation)
	{
		// If the light direction is attached to the view frame then we need to update.
		if (scene_lighting_params.is_light_direction_attached_to_view_frame())
		{
			update = true;
		}

		d_view_orientation = view_orientation;
	}

	if (scene_lighting_params != d_scene_lighting_params)
	{
		// If any of the lighting parameters have changed then we need to update.
		update = true;

		d_scene_lighting_params = scene_lighting_params;
	}

	if (update)
	{
		if (d_map_projection)
		{
			// Regenerate the light direction cube map texture.
			update_map_view(renderer);
		}
		else
		{
			// Update the world-space light direction.
			update_globe_view(renderer);
		}

		// Let clients know in case they need to flush and regenerate their cache.
		d_subject_token.invalidate();
	}
}


void
GPlatesOpenGL::GLLight::update_map_view(
		GLRenderer &renderer)
{
	if (!d_map_projection)
	{
		return;
	}

	//
	// The light direction is constant in 2D map view but varies across the globe.
	// So we capture the variation in a hardware cube map texture.
	//

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(
			renderer,
			// We're rendering to a render target so reset to the default OpenGL state...
			true/*reset_to_default_state*/);

	// Used to draw a full-screen quad into render texture.
	const GLCompiledDrawState::non_null_ptr_to_const_type full_screen_quad_drawable =
			renderer.get_context().get_shared_state()->get_full_screen_2D_textured_quad(renderer);

	// Classify our frame buffer object according to texture format/dimensions.
	GLFrameBufferObject::Classification framebuffer_object_classification;
	framebuffer_object_classification.set_dimensions(
			d_map_view_light_direction_cube_texture->get_width().get(),
			d_map_view_light_direction_cube_texture->get_height().get());
	framebuffer_object_classification.set_texture_internal_format(
			d_map_view_light_direction_cube_texture->get_internal_format().get());

	// Acquire and bind a frame buffer object.
	GLFrameBufferObject::shared_ptr_type framebuffer_object =
			renderer.get_context().get_non_shared_state()->acquire_frame_buffer_object(
					renderer,
					framebuffer_object_classification);
	renderer.gl_bind_frame_buffer(framebuffer_object);

	// Bind the shader program for rendering light direction for the 2D map views.
	renderer.gl_bind_program_object(d_render_map_view_light_direction_program_object.get());

	// If the light direction is attached to the view frame then specify a non-identity view transform.
	if (d_scene_lighting_params.is_light_direction_attached_to_view_frame())
	{
		// NOTE: We're hijacking the GL_MODELVIEW matrix to store the view transform so we can get
		// OpenGL to calculate the matrix inverse for us. This is not the normal usage for GL_MODELVIEW.
		renderer.gl_load_matrix(GL_MODELVIEW, d_view_orientation);
	}

	// Set the view-space light direction (which is world-space if light not attached to view-space).
	// The shader program will transform it to world-space.
	d_render_map_view_light_direction_program_object.get()->gl_uniform3f(
			renderer,
			"view_space_light_direction",
			d_scene_lighting_params.get_light_direction());

	// Render to the entire texture of each cube face.
	renderer.gl_viewport(0, 0, d_light_direction_cube_texture_dimension, d_light_direction_cube_texture_dimension);

	// Render to all six faces of the cube texture.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GLenum face_target = static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + face);

		// Begin rendering to the 2D texture of the current cube face.
		framebuffer_object->gl_attach_texture_2D(
				renderer, face_target, d_map_view_light_direction_cube_texture, 0/*level*/, GL_COLOR_ATTACHMENT0_EXT);

		renderer.gl_clear_color(); // Clear colour to all zeros.
		renderer.gl_clear(GL_COLOR_BUFFER_BIT); // Clear only the colour buffer.

		// Render the lat/lon mesh.
		renderer.apply_compiled_draw_state(*full_screen_quad_drawable);
	}

	// Detach from the framebuffer object before we return it to the framebuffer object cache.
	framebuffer_object->gl_detach_all(renderer);
}


void
GPlatesOpenGL::GLLight::update_globe_view(
		GLRenderer &renderer)
{
	if (d_map_projection)
	{
		return;
	}

	if (d_scene_lighting_params.is_light_direction_attached_to_view_frame())
	{
		// The light direction is in view-space.
		const GPlatesMaths::UnitVector3D &view_space_light_direction =
				d_scene_lighting_params.get_light_direction();
		const double light_x = view_space_light_direction.x().dval();
		const double light_y = view_space_light_direction.y().dval();
		const double light_z = view_space_light_direction.z().dval();

		// Need to reverse rotate back to world-space.
		// We'll assume the view orientation matrix stores only a 3x3 rotation.
		// In which case the inverse matrix is the transpose.
		const GLMatrix &view = d_view_orientation;

		// Multiply the view-space light direction by the transpose of the 3x3 view orientation.
		d_globe_view_light_direction = GPlatesMaths::Vector3D(
				view.get_element(0,0) * light_x + view.get_element(1,0) * light_y + view.get_element(2,0) * light_z,
				view.get_element(0,1) * light_x + view.get_element(1,1) * light_y + view.get_element(2,1) * light_z,
				view.get_element(0,2) * light_x + view.get_element(1,2) * light_y + view.get_element(2,2) * light_z)
						.get_normalisation();
	}
	else // light direction is attached to world-space...
	{
		d_globe_view_light_direction = d_scene_lighting_params.get_light_direction();
	}
}


void
GPlatesOpenGL::GLLight::create_shader_programs(
		GLRenderer &renderer)
{
	d_render_map_view_light_direction_program_object =
			GLShaderProgramUtils::compile_and_link_vertex_fragment_program(
					renderer,
					GLShaderProgramUtils::ShaderSource::create_shader_source_from_file(
							RENDER_MAP_VIEW_LIGHT_DIRECTION_VERTEX_SHADER_SOURCE_FILE_NAME),
					GLShaderProgramUtils::ShaderSource::create_shader_source_from_file(
							RENDER_MAP_VIEW_LIGHT_DIRECTION_FRAGMENT_SHADER_SOURCE_FILE_NAME));

	// The client should have called 'is_supported()' which verifies vertex/fragment shader support
	// and that the most complex shader compiles - so that should not be the reason for failure.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_render_map_view_light_direction_program_object,
			GPLATES_ASSERTION_SOURCE);
}


void
GPlatesOpenGL::GLLight::create_map_view_light_direction_cube_texture(
		GLRenderer &renderer)
{
	// Using nearest-neighbour filtering since the 'pixelation' of the light direction is not
	// noticeable once it goes through the dot product with the surface normals.
	// Also it enables us to have distinctly different light directions on either side of the
	// central meridian which we'll make go through the centre of some of the faces of the cube
	// (which is along a boundary between two columns of pixels - provided texture dimension is even).
	d_map_view_light_direction_cube_texture->gl_tex_parameteri(
			renderer, GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	d_map_view_light_direction_cube_texture->gl_tex_parameteri(
			renderer, GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

	// Clamp texture coordinates to centre of edge texels.
	// Not strictly necessary for nearest-neighbour filtering but it is if later we change to use
	// linear filtering to avoid seams.
	if (GLEW_EXT_texture_edge_clamp || GLEW_SGIS_texture_edge_clamp)
	{
		d_map_view_light_direction_cube_texture->gl_tex_parameteri(
				renderer, GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		d_map_view_light_direction_cube_texture->gl_tex_parameteri(
				renderer, GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	}
	else
	{
		d_map_view_light_direction_cube_texture->gl_tex_parameteri(
				renderer, GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_S, GL_CLAMP);
		d_map_view_light_direction_cube_texture->gl_tex_parameteri(
				renderer, GL_TEXTURE_CUBE_MAP_ARB, GL_TEXTURE_WRAP_T, GL_CLAMP);
	}

	// Create the texture but don't load any data into it.
	// Leave it uninitialised because we will be rendering into it to initialise it.
	//
	// NOTE: Since the image data is NULL it doesn't really matter what 'format' and 'type' are -
	// just use values that are compatible with all internal formats to avoid a possible error.

	// Initialise all six faces of the cube texture.
	for (unsigned int face = 0; face < 6; ++face)
	{
		const GLenum face_target = static_cast<GLenum>(GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB + face);

		d_map_view_light_direction_cube_texture->gl_tex_image_2D(
				renderer, face_target, 0, GL_RGBA8,
				d_light_direction_cube_texture_dimension, d_light_direction_cube_texture_dimension,
				0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	}

	// Check there are no OpenGL errors.
	GLUtils::assert_no_gl_errors(GPLATES_ASSERTION_SOURCE);
}
