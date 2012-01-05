/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#include <iostream>
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <opengl/OpenGL.h>

#include "MapRenderedGeometryCollectionPainter.h"

#include "MapRenderedGeometryLayerPainter.h"

#include "opengl/GLRenderer.h"

#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryUtils.h"


GPlatesGui::MapRenderedGeometryCollectionPainter::MapRenderedGeometryCollectionPainter(
		const MapProjection::non_null_ptr_to_const_type &map_projection,
		const GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
		const GPlatesPresentation::VisualLayers &visual_layers,
		RenderSettings &render_settings,
		const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
		ColourScheme::non_null_ptr_type colour_scheme) :
	d_map_projection(map_projection),
	d_rendered_geometry_collection(rendered_geometry_collection),
	d_persistent_opengl_objects(persistent_opengl_objects),
	d_visual_layers(visual_layers),
	d_render_settings(render_settings),
	d_text_renderer_ptr(text_renderer_ptr),
	d_layer_painter(false/*use_depth_buffer*/),
	d_colour_scheme(colour_scheme),
	d_scale(1.0f)
{  }


GPlatesGui::MapRenderedGeometryCollectionPainter::cache_handle_type
GPlatesGui::MapRenderedGeometryCollectionPainter::paint(
		GPlatesOpenGL::GLRenderer &renderer,
		const double &viewport_zoom_factor)
{
	// Make sure we leave the OpenGL state the way it was.
	GPlatesOpenGL::GLRenderer::StateBlockScope save_restore_globe_state_scope(renderer);

	// Initialise our paint parameters so our visit methods can access them.
	d_paint_params = PaintParams(renderer, viewport_zoom_factor);

	// Draw the layers.
	d_rendered_geometry_collection.accept_visitor(*this);

	// Get the cache handle for all the rendered layers.
	const cache_handle_type cache_handle = d_paint_params->d_cache_handle;

	// These parameters are only used for the duration of this 'paint()' method.
	d_paint_params = boost::none;

	return cache_handle;
}


bool
GPlatesGui::MapRenderedGeometryCollectionPainter::visit_rendered_geometry_layer(
		const GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer)
{
	// If layer is not active then we don't want to visit it.
	if (!rendered_geometry_layer.is_active())
	{
		return false;
	}

	if (rendered_geometry_layer.is_empty())
	{
		return false;
	}

	// Draw the current rendered geometry layer.
	MapRenderedGeometryLayerPainter rendered_geom_layer_painter(
			d_map_projection,
			rendered_geometry_layer,
			d_persistent_opengl_objects,
			d_paint_params->d_inverse_viewport_zoom_factor,
			d_render_settings,
			d_text_renderer_ptr,
			d_colour_scheme);
	rendered_geom_layer_painter.set_scale(d_scale);

	// Paint the layer.
	const cache_handle_type layer_cache =
			rendered_geom_layer_painter.paint(*d_paint_params->d_renderer, d_layer_painter);

	// Cache the layer's painting.
	d_paint_params->d_cache_handle->push_back(layer_cache);

	// We've already visited the rendered geometry layer so don't visit its rendered geometries.
	return false;
}


void
GPlatesGui::MapRenderedGeometryCollectionPainter::set_scale(
		float scale)
{
	d_scale = scale;
}


boost::optional<GPlatesPresentation::VisualLayers::rendered_geometry_layer_seq_type>
GPlatesGui::MapRenderedGeometryCollectionPainter::get_custom_child_layers_order(
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType parent_layer)
{
	if (parent_layer == GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER)
	{
		return d_visual_layers.get_layer_order();
	}
	else
	{
		return boost::none;
	}
}