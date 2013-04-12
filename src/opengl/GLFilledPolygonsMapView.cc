/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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
#include <QDebug>

#include "GLFilledPolygonsMapView.h"

#include "GLContext.h"
#include "GLRenderer.h"

#include "utils/Profile.h"


GPlatesOpenGL::GLFilledPolygonsMapView::GLFilledPolygonsMapView(
		GLRenderer &renderer)
{
	create_polygons_vertex_array(renderer);
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::render(
		GLRenderer &renderer,
		const filled_polygons_type &filled_polygons)
{
	PROFILE_FUNC();

	// Make sure we leave the OpenGL state the way it was.
	GLRenderer::StateBlockScope save_restore_state(renderer);

	// If there are no filled polygons to render then return early.
	if (filled_polygons.d_polygon_vertex_elements.empty())
	{
		return;
	}

	// We need a stencil buffer so return early if there's not one.
	//
	// Note that we don't have an 'is_supported()' method that tests for this because pretty much all
	// hardware should support a stencil buffer (and software implementations should also support).
	if (!renderer.get_context().get_qgl_format().stencil())
	{
		// Only emit warning message once.
		static bool emitted_warning = false;
		if (!emitted_warning)
		{
			qWarning() <<
					"Filled polygons NOT supported by this OpenGL system - \n"
					"  requires stencil buffer - Most graphics hardware for over a decade supports this.";
			emitted_warning = true;
		}

		return;
	}

	// Write the vertices/indices of all filled polygons (gathered by the client) into our
	// vertex buffer and vertex element buffer.
	write_filled_polygon_meshes_to_vertex_array(renderer, filled_polygons);

	// Clear the stencil buffer.
	renderer.gl_clear_stencil();
	renderer.gl_clear(GL_STENCIL_BUFFER_BIT);

	// Set the alpha-blend state since filled polygon could have a transparent colour.
	// Set up alpha blending for pre-multiplied alpha.
	// This has (src,dst) blend factors of (1, 1-src_alpha) instead of (src_alpha, 1-src_alpha).
	// This is where the RGB channels have already been multiplied by the alpha channel.
	// See class GLVisualRasterSource for why this is done.
	renderer.gl_blend_func(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

	// Enable stencil writes (this is the default OpenGL state anyway).
	renderer.gl_stencil_mask(~0);

	// Enable stencil testing.
	renderer.gl_enable(GL_STENCIL_TEST);

	// Bind the vertex array before using it to draw.
	d_polygons_vertex_array->gl_bind(renderer);

	// Iterate over the filled drawables and render each one into the scene.
	filled_polygon_seq_type::const_iterator filled_drawables_iter =
			filled_polygons.d_polygon_filled_drawables.begin();
	filled_polygon_seq_type::const_iterator filled_drawables_end =
			filled_polygons.d_polygon_filled_drawables.end();
	for ( ; filled_drawables_iter != filled_drawables_end; ++filled_drawables_iter)
	{
		const filled_polygon_type &filled_drawable = *filled_drawables_iter;

		// Set the stencil function to always pass.
		renderer.gl_stencil_func(GL_ALWAYS, 0, ~0);
		// Set the stencil operation to invert the stencil buffer value every time a pixel is
		// rendered (this means we get 1 where a pixel is covered by an odd number of triangles
		// and 0 by an even number of triangles).
		renderer.gl_stencil_op(GL_KEEP, GL_KEEP, GL_INVERT);

		// Disable colour writes and alpha blending.
		// We only want to modify the stencil buffer on this pass.
		renderer.gl_color_mask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
		renderer.gl_enable(GL_BLEND, false);

		// Render the current filled polygon as a polygon (fan) mesh.
		d_polygons_vertex_array->gl_draw_range_elements(
				renderer,
				GL_TRIANGLES,
				filled_drawable.d_polygon_mesh_drawable.start,
				filled_drawable.d_polygon_mesh_drawable.end,
				filled_drawable.d_polygon_mesh_drawable.count,
				GLVertexElementTraits<polygon_vertex_element_type>::type,
				filled_drawable.d_polygon_mesh_drawable.indices_offset);

		// Set the stencil function to pass only if the stencil buffer value is non-zero.
		// This means we only draw into the tile texture for pixels 'interior' to the filled polygon.
		renderer.gl_stencil_func(GL_NOTEQUAL, 0, ~0);
		// Set the stencil operation to set the stencil buffer to zero in preparation
		// for the next polygon (also avoids multiple alpha-blending due to overlapping fan
		// triangles as mentioned below).
		renderer.gl_stencil_op(GL_KEEP, GL_KEEP, GL_ZERO);

		// Re-enable colour writes and alpha blending.
		renderer.gl_color_mask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		renderer.gl_enable(GL_BLEND, true);

		// Render the current filled polygon as a polygon (fan) mesh again.
		// This drawable covers at least all interior pixels of the filled polygon.
		// It also can covers exterior pixels of the filled polygon.
		// However only the interior pixels (where stencil buffer is non-zero) will
		// pass the stencil test and get written into the tile (colour) texture.
		// The drawable also can render pixels multiple times due to overlapping fan triangles.
		// To avoid alpha blending each pixel more than once, the above stencil operation zeros
		// the stencil buffer value of each pixel that passes the stencil test such that the next
		// overlapping pixel will then fail the stencil test (avoiding multiple-alpha-blending).
		d_polygons_vertex_array->gl_draw_range_elements(
				renderer,
				GL_TRIANGLES,
				filled_drawable.d_polygon_mesh_drawable.start,
				filled_drawable.d_polygon_mesh_drawable.end,
				filled_drawable.d_polygon_mesh_drawable.count,
				GLVertexElementTraits<polygon_vertex_element_type>::type,
				filled_drawable.d_polygon_mesh_drawable.indices_offset);
	}
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::create_polygons_vertex_array(
		GLRenderer &renderer)
{
	d_polygons_vertex_array = GLVertexArray::create(renderer);

	// Set up the vertex element buffer.
	GLBuffer::shared_ptr_type vertex_element_buffer_data = GLBuffer::create(renderer);
	d_polygons_vertex_element_buffer = GLVertexElementBuffer::create(renderer, vertex_element_buffer_data);
	// Attach vertex element buffer to the vertex array.
	d_polygons_vertex_array->set_vertex_element_buffer(renderer, d_polygons_vertex_element_buffer);

	// Set up the vertex buffer.
	GLBuffer::shared_ptr_type vertex_buffer_data = GLBuffer::create(renderer);
	d_polygons_vertex_buffer = GLVertexBuffer::create(renderer, vertex_buffer_data);

	// Attach polygons vertex buffer to the vertex array.
	//
	// Later we'll be allocating a vertex buffer large enough to contain all polygons and
	// rendering each polygon with its own OpenGL draw call.
	bind_vertex_buffer_to_vertex_array<polygon_vertex_type>(
			renderer,
			*d_polygons_vertex_array,
			d_polygons_vertex_buffer);
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::write_filled_polygon_meshes_to_vertex_array(
		GLRenderer &renderer,
		const filled_polygons_type &filled_polygons)
{
	// Use 'stream' since each filled polygon is accessed only twice...

	GLBuffer::shared_ptr_type vertex_element_buffer_data = d_polygons_vertex_element_buffer->get_buffer();
	vertex_element_buffer_data->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_ELEMENT_ARRAY_BUFFER,
			filled_polygons.d_polygon_vertex_elements,
			// Use 'stream' since each filled polygon is accessed only twice...
			GLBuffer::USAGE_STREAM_DRAW);

	GLBuffer::shared_ptr_type vertex_buffer_data = d_polygons_vertex_buffer->get_buffer();
	vertex_buffer_data->gl_buffer_data(
			renderer,
			GLBuffer::TARGET_ARRAY_BUFFER,
			filled_polygons.d_polygon_vertices,
			// Use 'stream' since each filled polygon is accessed only twice...
			GLBuffer::USAGE_STREAM_DRAW);

	//qDebug() << "Writing triangles: " << filled_polygons.d_polygon_vertex_elements.size() / 3;
}


void
GPlatesOpenGL::GLFilledPolygonsMapView::FilledPolygons::add_filled_polygon(
		const std::vector<QPointF> &line_geometry,
		const GPlatesGui::Colour &colour)
{
	PROFILE_FUNC();

	const unsigned int num_points = line_geometry.size();

	// Need at least three points for a polygon.
	if (num_points < 3)
	{
		return;
	}

	unsigned int n;

	// Calculate centroid of polygon.
	QPointF centroid;
	for (n = 0; n < num_points; ++n)
	{
		centroid += line_geometry[n];
	}
	centroid /= num_points;

	// Alpha blending will be set up for pre-multiplied alpha.
	const GPlatesGui::rgba8_t pre_multiplied_alpha_colour =
			GPlatesGui::Colour::to_rgba8(
					GPlatesGui::Colour(
							colour.red() * colour.alpha(),
							colour.green() * colour.alpha(),
							colour.blue() * colour.alpha(),
							colour.alpha()));

	//
	// Create the OpenGL coloured vertices for the filled polygon (fan) mesh.
	//

	const GLsizei base_vertex_element_index = d_polygon_vertex_elements.size();
	const polygon_vertex_element_type base_vertex_index = d_polygon_vertices.size();
	polygon_vertex_element_type vertex_index = base_vertex_index;

	// First vertex is the centroid.
	d_polygon_vertices.push_back(
			polygon_vertex_type(
					centroid.x(),
					centroid.y(),
					0/*z*/,
					pre_multiplied_alpha_colour));
	++vertex_index;

	// The remaining vertices form the boundary.
	for (n = 0; n < num_points; ++n, ++vertex_index)
	{
		d_polygon_vertices.push_back(
				polygon_vertex_type(
						line_geometry[n].x(),
						line_geometry[n].y(),
						0/*z*/,
						pre_multiplied_alpha_colour));

		d_polygon_vertex_elements.push_back(base_vertex_index); // Centroid.
		d_polygon_vertex_elements.push_back(vertex_index); // Current boundary point.
		d_polygon_vertex_elements.push_back(vertex_index + 1); // Next boundary point.
	}

	// Wraparound back to the first boundary vertex to close off the polygon (in case it's a polyline).
	d_polygon_vertices.push_back(
			polygon_vertex_type(
					line_geometry[0].x(),
					line_geometry[0].y(),
					0/*z*/,
					pre_multiplied_alpha_colour));

	// Create the filled polygon (fan) mesh drawable.
	const filled_polygon_type::Drawable polygon_mesh_drawable(
			base_vertex_index/*start*/,
			vertex_index/*end*/, // ...the last vertex index
			d_polygon_vertex_elements.size() - base_vertex_element_index /*count*/,
			base_vertex_element_index * sizeof(polygon_vertex_element_type)/*indices_offset*/);

	d_polygon_filled_drawables.push_back(filled_polygon_type(polygon_mesh_drawable));
}