/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009 The University of Sydney, Australia
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

#include "CanvasToolChoice.h"

#include "canvas-tools/CanvasToolType.h"
#include "canvas-tools/ReorientGlobe.h"
#include "canvas-tools/ZoomGlobe.h"
#include "canvas-tools/ClickGeometry.h"
#include "canvas-tools/DeleteVertex.h"
#include "canvas-tools/DigitiseGeometry.h"
#include "canvas-tools/InsertVertex.h"
#include "canvas-tools/MoveGeometry.h"
#include "canvas-tools/MoveVertex.h"
#include "canvas-tools/ManipulatePole.h"
#include "canvas-tools/BuildTopology.h"
#include "canvas-tools/EditTopology.h"

#include "qt-widgets/DigitisationWidget.h"
#include "qt-widgets/PlateClosureWidget.h"
#include "qt-widgets/BuildTopologyWidget.h"
#include "qt-widgets/EditTopologyWidget.h"

#include "view-operations/RenderedGeometryCollection.h"


GPlatesGui::CanvasToolChoice::CanvasToolChoice(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::GeometryOperationTarget &geometry_operation_target,
		GPlatesViewOperations::ActiveGeometryOperation &active_geometry_operation,
		GPlatesGui::ChooseCanvasTool &choose_canvas_tool,
		const GPlatesViewOperations::QueryProximityThreshold &query_proximity_threshold,
		Globe &globe,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas,
		const GPlatesQtWidgets::ViewportWindow &view_state,
		FeatureTableModel &clicked_table_model,
		FeatureTableModel &feature_table_model_segments,
		GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog,
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesQtWidgets::ReconstructionPoleWidget &pole_widget,
		GPlatesQtWidgets::PlateClosureWidget &plate_closure_widget,
		GPlatesQtWidgets::BuildTopologyWidget &build_topology_widget,
		GPlatesQtWidgets::EditTopologyWidget &edit_topology_widget,
		GPlatesGui::GeometryFocusHighlight &geometry_focus_highlight):
d_reorient_globe_tool_ptr(GPlatesCanvasTools::ReorientGlobe::create(
		globe,
		globe_canvas,
		view_state)),
d_zoom_globe_tool_ptr(GPlatesCanvasTools::ZoomGlobe::create(
		globe,
		globe_canvas,
		view_state)),
d_click_geometry_tool_ptr(GPlatesCanvasTools::ClickGeometry::create(
		rendered_geom_collection,
		globe,
		globe_canvas,
		view_state,
		clicked_table_model,
		fp_dialog,
		feature_focus,
		geometry_focus_highlight)),
d_digitise_polyline_tool_ptr(GPlatesCanvasTools::DigitiseGeometry::create(
		GPlatesViewOperations::GeometryType::POLYLINE,
		geom_builder_tool_target,
		rendered_geom_collection,
		rendered_geom_factory,
		choose_canvas_tool,
		globe,
		globe_canvas,
		view_state)),
d_digitise_multipoint_tool_ptr(GPlatesCanvasTools::DigitiseGeometry::create(
		GPlatesViewOperations::GeometryType::MULTIPOINT,
		geom_builder_tool_target,
		rendered_geom_collection,
		rendered_geom_factory,
		choose_canvas_tool,
		globe,
		globe_canvas,
		view_state)),
d_digitise_polygon_tool_ptr(GPlatesCanvasTools::DigitiseGeometry::create(
		GPlatesViewOperations::GeometryType::POLYGON,
		geom_builder_tool_target,
		rendered_geom_collection,
		rendered_geom_factory,
		choose_canvas_tool,
		globe,
		globe_canvas,
		view_state)),
d_move_geometry_tool_ptr(GPlatesCanvasTools::MoveGeometry::create(
		globe,
		globe_canvas,
		view_state)),
d_move_vertex_tool_ptr(GPlatesCanvasTools::MoveVertex::create(
		geom_builder_tool_target,
		rendered_geom_collection,
		rendered_geom_factory,
		choose_canvas_tool,
		query_proximity_threshold,
		globe,
		globe_canvas,
		view_state)),
d_manipulate_pole_tool_ptr(GPlatesCanvasTools::ManipulatePole::create(
		rendered_geom_collection,
		globe,
		globe_canvas,
		view_state,
		pole_widget)),
d_plate_closure_platepolygon_tool_ptr( GPlatesCanvasTools::PlateClosure::create(
		rendered_geom_collection,
		globe, 
		globe_canvas, 
		view_state, 
		clicked_table_model, 
		feature_table_model_segments, 
		plate_closure_widget, 
		GPlatesQtWidgets::PlateClosureWidget::PLATEPOLYGON, 
		feature_focus)),
d_build_topology_tool_ptr( GPlatesCanvasTools::BuildTopology::create(
		rendered_geom_collection,
		globe, 
		globe_canvas, 
		view_state, 
		clicked_table_model, 
		feature_table_model_segments, 
		build_topology_widget, 
		GPlatesQtWidgets::BuildTopologyWidget::PLATEPOLYGON, 
		feature_focus)),
d_edit_topology_tool_ptr( GPlatesCanvasTools::EditTopology::create(
		rendered_geom_collection,
		globe, 
		globe_canvas, 
		view_state, 
		clicked_table_model, 
		feature_table_model_segments, 
		edit_topology_widget, 
		GPlatesQtWidgets::EditTopologyWidget::PLATEPOLYGON, 
		feature_focus)),
d_tool_choice_ptr(d_reorient_globe_tool_ptr)
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	tool_choice().handle_activation();
}


void
GPlatesGui::CanvasToolChoice::change_tool_if_necessary(
		CanvasTool::non_null_ptr_type new_tool_choice)
{
	if (new_tool_choice == d_tool_choice_ptr) {
		// The specified tool is already chosen.  Nothing to do here.
		return;
	}

	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	d_tool_choice_ptr->handle_deactivation();
	d_tool_choice_ptr = new_tool_choice;
	d_tool_choice_ptr->handle_activation();
}

