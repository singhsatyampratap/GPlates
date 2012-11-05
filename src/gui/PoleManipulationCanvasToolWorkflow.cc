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

#include "PoleManipulationCanvasToolWorkflow.h"

#include "Dialogs.h"
#include "FeatureFocus.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/TopologyUtils.h"

#include "canvas-tools/CanvasToolAdapterForGlobe.h"
#include "canvas-tools/CanvasToolAdapterForMap.h"
#include "canvas-tools/ManipulatePole.h"

#include "global/GPlatesAssert.h"

#include "presentation/ViewState.h"

#include "qt-widgets/GlobeAndMapWidget.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/MapView.h"
#include "qt-widgets/ReconstructionViewWidget.h"
#include "qt-widgets/TaskPanel.h"
#include "qt-widgets/ViewportWindow.h"

#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesGui
{
	namespace
	{
		/**
		 * The main rendered layer used by this canvas tool workflow.
		 */
		const GPlatesViewOperations::RenderedGeometryCollection::MainLayerType WORKFLOW_RENDER_LAYER =
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER;
	}
}

GPlatesGui::PoleManipulationCanvasToolWorkflow::PoleManipulationCanvasToolWorkflow(
		CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window) :
	CanvasToolWorkflow(
			viewport_window.globe_canvas(),
			viewport_window.map_view(),
			CanvasToolWorkflows::WORKFLOW_POLE_MANIPULATION,
			// The tool to start off with...
			CanvasToolWorkflows::TOOL_MANIPULATE_POLE),
	d_feature_focus(view_state.get_feature_focus()),
	d_rendered_geom_collection(view_state.get_rendered_geometry_collection())
{
	create_canvas_tools(
			canvas_tool_workflows,
			status_bar_callback,
			view_state,
			viewport_window);

	// Listen for focus feature signals.
	QObject::connect(
		&d_feature_focus,
		SIGNAL(focus_changed(
				GPlatesGui::FeatureFocus &)),
		this,
		SLOT(feature_focus_changed(
				GPlatesGui::FeatureFocus &)));
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::create_canvas_tools(
		CanvasToolWorkflows &canvas_tool_workflows,
		const GPlatesCanvasTools::CanvasTool::status_bar_callback_type &status_bar_callback,
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ViewportWindow &viewport_window)
{
	//
	// Manipulate pole canvas tool.
	//

	GPlatesCanvasTools::ManipulatePole::non_null_ptr_type manipulate_pole_tool =
			GPlatesCanvasTools::ManipulatePole::create(
					status_bar_callback,
					view_state.get_rendered_geometry_collection(),
					viewport_window.task_panel_ptr()->modify_reconstruction_pole_widget());
	// For the globe view.
	d_globe_manipulate_pole_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForGlobe(
					manipulate_pole_tool,
					viewport_window.globe_canvas().globe(),
					viewport_window.globe_canvas()));
	// For the map view.
	d_map_manipulate_pole_tool.reset(
			new GPlatesCanvasTools::CanvasToolAdapterForMap(
					manipulate_pole_tool,
					viewport_window.map_view().map_canvas(),
					viewport_window.map_view(),
					view_state.get_map_transform()));
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::initialise()
{
	// Set the initial enable/disable state for our canvas tools.
	//
	// These tools are always enabled regardless of the current state.
	//
	// NOTE: If you are updating the tool in 'update_enable_state()' then you
	// don't need to enable/disable it here.

	update_enable_state();
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::activate_workflow()
{
	// Activate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, true/*active*/);
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::deactivate_workflow()
{
	// Deactivate the main rendered layer.
	d_rendered_geom_collection.set_main_layer_active(WORKFLOW_RENDER_LAYER, false/*active*/);
}


boost::optional< std::pair<GPlatesGui::GlobeCanvasTool *, GPlatesGui::MapCanvasTool *> >
GPlatesGui::PoleManipulationCanvasToolWorkflow::get_selected_globe_and_map_canvas_tools(
			CanvasToolWorkflows::ToolType selected_tool) const
{
	switch (selected_tool)
	{
	case CanvasToolWorkflows::TOOL_MANIPULATE_POLE:
		return std::make_pair(d_globe_manipulate_pole_tool.get(), d_map_manipulate_pole_tool.get());

	default:
		break;
	}

	return boost::none;
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::feature_focus_changed(
		GPlatesGui::FeatureFocus &feature_focus)
{
	update_enable_state();
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::update_enable_state()
{
	update_manipulate_pole_tool();
}


void
GPlatesGui::PoleManipulationCanvasToolWorkflow::update_manipulate_pole_tool()
{
	const GPlatesModel::FeatureHandle::const_weak_ref focused_feature = d_feature_focus.focused_feature();

	bool enable_manipulate_pole_tool = false;

	// Enable pole manipulation if there's a focused feature that is not topological.
	if (focused_feature.is_valid())
	{
		if (!GPlatesAppLogic::TopologyUtils::is_topological_geometry_feature(focused_feature))
		{
			enable_manipulate_pole_tool = true;
		}
	}

	emit_canvas_tool_enabled(CanvasToolWorkflows::TOOL_MANIPULATE_POLE, enable_manipulate_pole_tool);
}