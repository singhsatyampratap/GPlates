/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#include <boost/none.hpp>

#include "Colour.h"
#include "GeometryFocusHighlight.h"

#include "app-logic/ReconstructionGeometryFinder.h"
#include "gui/FeatureFocus.h"
#include "gui/Symbol.h"
#include "presentation/ReconstructionGeometryRenderer.h"
#include "view-operations/RenderedGeometryCollection.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryParameters.h"


GPlatesGui::GeometryFocusHighlight::GeometryFocusHighlight(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		const GPlatesGui::symbol_map_type &symbol_map):
d_rendered_geom_collection(&rendered_geom_collection),
d_highlight_layer_ptr(
		rendered_geom_collection.get_main_rendered_layer(
				GPlatesViewOperations::RenderedGeometryCollection::GEOMETRY_FOCUS_HIGHLIGHT_LAYER)),
d_symbol_map(symbol_map)
{
}

void
GPlatesGui::GeometryFocusHighlight::set_focus(
		GPlatesGui::FeatureFocus &feature_focus)
{
	if (d_focused_geometry == feature_focus.associated_reconstruction_geometry() &&
		d_feature == feature_focus.focused_feature())
	{
		// No change, so nothing to do.
		return;
	}

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Else, presumably the focused geometry has changed.
	d_feature = feature_focus.focused_feature();
	d_focused_geometry = feature_focus.associated_reconstruction_geometry();
	draw_focused_geometry();
}


void
GPlatesGui::GeometryFocusHighlight::draw_focused_geometry()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Activate our layer.
	d_highlight_layer_ptr->set_active();

	// Clear all geometries from layer before adding them.
	d_highlight_layer_ptr->clear_rendered_geometries();

	if (!d_focused_geometry)
	{
		// There's no focused geometry so there's nothing to draw.
		// NOTE: we do this after clearing the rendered geometries so that
		// any previously focused geometry is no longer highlighted.
		return;
	}

	//
	// Since a feature can have multiple geometry properties we need
	// to highlight them all even though only one geometry was clicked by the user.
	//

	// The reconstruction tree used to reconstruct the focused geometry.
	const GPlatesAppLogic::ReconstructionTree *reconstruction_tree =
			d_focused_geometry->reconstruction_tree().get();

	// Iterate through the ReconstructionGeometry's of the focused feature that were
	// reconstructed using the same 'reconstruction_tree' as the clicked geometry.
	GPlatesAppLogic::ReconstructionGeometryFinder rgFinder(reconstruction_tree);
	rgFinder.find_rgs_of_feature(d_feature);

	// FIXME: Probably should use the same styling params used to draw
	// the original geometries rather than use some of the defaults.
	GPlatesPresentation::ReconstructionGeometryRenderer::RenderParams render_style_params;
	render_style_params.reconstruction_line_width_hint =
			GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_LINE_WIDTH_HINT;
	render_style_params.reconstruction_point_size_hint =
			GPlatesViewOperations::RenderedLayerParameters::GEOMETRY_FOCUS_POINT_SIZE_HINT;

	GPlatesAppLogic::ReconstructionGeometryFinder::rg_container_type::const_iterator rgIter;
	for (rgIter = rgFinder.found_rgs_begin();
		rgIter != rgFinder.found_rgs_end();
		++rgIter)
	{
		const GPlatesAppLogic::ReconstructionGeometry *reconstruction_geometry = rgIter->get();

		// If the RG is the same as the focused geometry (the geometry that the
		// user clicked on) then highlight it in a different colour.
		const GPlatesGui::Colour &highlight_colour =
				(reconstruction_geometry == d_focused_geometry.get())
				? GPlatesViewOperations::FocusedFeatureParameters
						::CLICKED_GEOMETRY_OF_FOCUSED_FEATURE_COLOUR
				: GPlatesViewOperations::FocusedFeatureParameters
						::NON_CLICKED_GEOMETRY_OF_FOCUSED_FEATURE_COLOUR;

		// This creates the RenderedGeometry's using the highlight colour.
		GPlatesPresentation::ReconstructionGeometryRenderer highlighted_geometry_renderer(
				render_style_params,
				highlight_colour, 
				boost::none,
				d_symbol_map);

		highlighted_geometry_renderer.begin_render();

		reconstruction_geometry->accept_visitor(highlighted_geometry_renderer);

		highlighted_geometry_renderer.end_render(*d_highlight_layer_ptr);
	}
}
