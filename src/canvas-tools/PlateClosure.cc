/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include <queue>
#include <QLocale>

#include "PlateClosure.h"

#include "gui/ProximityTests.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/PlateClosureWidget.h"
#include "maths/LatLonPointConversions.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "global/InternalInconsistencyException.h"

GPlatesCanvasTools::PlateClosure::PlateClosure(
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
				const GPlatesQtWidgets::ViewportWindow &view_state_,
				GPlatesGui::FeatureTableModel &clicked_table_model_,	
				GPlatesGui::FeatureTableModel &segments_table_model_,	
				GPlatesQtWidgets::PlateClosureWidget &plate_closure_widget,
				GPlatesQtWidgets::PlateClosureWidget::GeometryType geom_type,
				GPlatesGui::FeatureFocus &feature_focus):
	CanvasTool(globe_, globe_canvas_),
	d_rendered_geom_collection(&rendered_geom_collection),
	d_view_state_ptr(&view_state_),
	d_clicked_table_model_ptr(&clicked_table_model_),
	d_segments_table_model_ptr(&segments_table_model_),
	d_plate_closure_widget_ptr(&plate_closure_widget),
	d_default_geom_type(geom_type),
	d_feature_focus_ptr(&feature_focus)
{
}


	

void
GPlatesCanvasTools::PlateClosure::handle_activation()
{
	// Clicking these canvas tools changes the type of geometry the user
	// wishes to create, and may adjust the current table of coordinates accordingly.
	d_plate_closure_widget_ptr->change_geometry_type(d_default_geom_type);

	// FIXME:  We may have to adjust the message if we are using a Map View.
	if (d_plate_closure_widget_ptr->geometry_type() ==
			GPlatesQtWidgets::PlateClosureWidget::PLATEPOLYGON) {
		d_view_state_ptr->status_message(QObject::tr(
				"Click on features to choose segments for the boundary."
				" Ctrl+drag to re-orient the globe."));
	} else {
		d_view_state_ptr->status_message(QObject::tr(
				"Click to draw a new vertex."
				" Ctrl+drag to reorient the globe."));
	}

	// Activate the pole manipulation rendered layer.
	d_rendered_geom_collection->set_main_layer_active(
		GPlatesViewOperations::RenderedGeometryCollection::TOPOLOGY_TOOL_LAYER);

	// hide other tools ; show plate closure 
	// d_layers_ptr->show_only_plate_closure_layer();

	// also show geom. focus 
	// d_layers_ptr->show_geometry_focus_layer();

	globe_canvas().update_canvas();

#if 0
// FIXME: something is wrong with connect's arguments below, but I can't see what?
// This connection was moved to ViewportWindow, and uses the canvas_tool_adapter 
// to come back to this tool ... it seems so round about, but it works.

 	// If the user creates a new feature with the PlateClosureWidget, 
	// then we need to create, and append property values to the feature
 	QObject::connect( 
		&( d_plate_closure_widget_ptr->create_feature_dialog() ),
 		SIGNAL(feature_created(GPlatesModel::FeatureHandle::weak_ref)),
 		this,
 		SLOT( append_property_values_to_feature( GPlatesModel::FeatureHandle::weak_ref ) )
		);
#endif
}


void
GPlatesCanvasTools::PlateClosure::handle_deactivation()
{  }


void
GPlatesCanvasTools::PlateClosure::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
std::cout << "GPlatesCanvasTools::PlateClosure::handle_left_click" << std::endl;

	const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(
		oriented_click_pos_on_globe);

	// send the click point to the widget
	d_plate_closure_widget_ptr->set_click_point( llp.latitude(), llp.longitude() );

	//
	// From ClickGeometry
	//
	double proximity_inclusion_threshold =
			globe_canvas().current_proximity_inclusion_threshold(click_pos_on_globe);
	
	// What did the user click on just now?
	std::priority_queue<GPlatesGui::ProximityTests::ProximityHit> sorted_hits;

	GPlatesGui::ProximityTests::find_close_rfgs(sorted_hits, view_state().reconstruction(),
			oriented_click_pos_on_globe, proximity_inclusion_threshold);
	
	// Give the user some useful feedback in the status bar.
	if (sorted_hits.size() == 0) {
		d_view_state_ptr->status_message(tr("Clicked %1 geometries.").arg(sorted_hits.size()));
	} else if (sorted_hits.size() == 1) {
		d_view_state_ptr->status_message(tr("Clicked %1 geometry.").arg(sorted_hits.size()));
	} else {
		d_view_state_ptr->status_message(tr("Clicked %1 geometries.").arg(sorted_hits.size()));
	}

	// Clear the 'Clicked' FeatureTableModel, ready to be populated (or not).
	d_clicked_table_model_ptr->clear();

	// Un-highlight all the rows of the 'Segments' Table 
	d_view_state_ptr->highlight_sections_table_clear();


	if (sorted_hits.size() == 0) {
		d_view_state_ptr->status_message(tr("Clicked 0 geometries."));
		// User clicked on empty space! Clear the currently focused feature.
		d_feature_focus_ptr->unset_focus();
		emit no_hits_found();
		return;
	}

//std::cout << "GPlatesCanvasTools::PlateClosure::handle_left_click >0 geometries " << std::endl;

	// Populate the 'Clicked' FeatureTableModel.
	d_clicked_table_model_ptr->begin_insert_features(0, static_cast<int>(sorted_hits.size()) - 1);
	while ( ! sorted_hits.empty())
	{
		d_clicked_table_model_ptr->geometry_sequence().push_back(
				sorted_hits.top().d_recon_geometry);
		sorted_hits.pop();
	}
	d_clicked_table_model_ptr->end_insert_features();
	d_view_state_ptr->highlight_first_clicked_feature_table_row();
	emit sorted_hits_updated();

	int click_index = d_clicked_table_model_ptr->current_index().row();

	// Highlight the 'Segments' Feature Table if needed

	// get the feature_id of the currently selected feature
	GPlatesModel::FeatureId clicked_fid;

	GPlatesModel::ReconstructionGeometry *clicked_rg = 
		d_clicked_table_model_ptr->geometry_sequence().at(click_index).get();
	GPlatesModel::ReconstructedFeatureGeometry *clicked_rfg =
		dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(clicked_rg);

	// FIXME: check for the cast and valid ref before doing this:
	// GPlatesModel::FeatureId clicked_fid = clicked_rfg->feature_ref()->feature_id();

	if (clicked_rfg) 
	{
//std::cout << "GPlatesCanvasTools::PlateClosure::handle_left_click CLICKED rfg YES" << std::endl;
		GPlatesModel::FeatureHandle::weak_ref clicked_ref = clicked_rfg->feature_ref();
		if ( clicked_ref.is_valid()) {
//std::cout << "GPlatesCanvasTools::PlateClosure::handle_left_click CLICKED ref YES" << std::endl;
			clicked_fid = clicked_ref->feature_id();
		} else{
//std::cout << "GPlatesCanvasTools::PlateClosure::handle_left_click CLICKED ref NO" << std::endl;
		}
	} else {
//std::cout << "GPlatesCanvasTools::PlateClosure::handle_left_click CLICKED rfg YES" << std::endl;
	}

	// clear any previous selection
	d_view_state_ptr->highlight_sections_table_clear();

	// loop over each geom in the Segments Table
	std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>::iterator iter;
	std::vector<GPlatesModel::ReconstructionGeometry::non_null_ptr_type>::iterator end;
	iter = d_segments_table_model_ptr->geometry_sequence().begin();	
	end = d_segments_table_model_ptr->geometry_sequence().end();
	int i = 0;
	for ( ; iter != end ; ++iter)
	{
//std::cout << "iiiiiiiiiiiiiiiiiiiiiiiiiiii=" << i << std::endl;
		d_view_state_ptr->highlight_sections_table_row(i, false);

		GPlatesModel::ReconstructionGeometry *index_rg = iter->get();
		GPlatesModel::ReconstructedFeatureGeometry *index_rfg =
			dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(index_rg);

		if (index_rfg) {
//std::cout << "GPlatesCanvasTools::PlateClosure::handle_left_click INDEX rfg YES" << std::endl;
			GPlatesModel::FeatureHandle::weak_ref index_ref = index_rfg->feature_ref();
			if ( index_ref.is_valid()) {
//std::cout << "GPlatesCanvasTools::PlateClosure::handle_left_click INDEX ref YES" << std::endl;
				GPlatesModel::FeatureId index_fid = index_ref->feature_id();
				if (clicked_fid == index_fid) {
// std::cout << "=============================" << i << std::endl;
					d_view_state_ptr->highlight_sections_table_row(i, true);
				}
			} else {
//std::cout << "GPlatesCanvasTools::PlateClosure::handle_left_click INDEX ref NO" << std::endl;
			}
		} else {
//std::cout << "GPlatesCanvasTools::PlateClosure::handle_left_click INDEX rfg NO" << std::endl;
		}
		++i;
	}

//std::cout << "GPlatesCanvasTools::PlateClosure::handle_left_click 2" << std::endl;

#if 0  
// FIXME: remove this 
	// It seems it's not necessary to set the feature focus here, 
	// as it's already being set elsewhere.
	// Update the focused feature.

	GPlatesModel::ReconstructionGeometry *rg = sorted_hits.top().d_recon_geometry.get();
	// We use a dynamic cast here (despite the fact that dynamic casts are generally considered
	// bad form) because we only care about one specific derivation.  There's no "if ... else
	// if ..." chain, so I think it's not super-bad form.  (The "if ... else if ..." chain
	// would imply that we should be using polymorphism -- specifically, the double-dispatch of
	// the Visitor pattern -- rather than updating the "if ... else if ..." chain each time a
	// new derivation is added.)
	GPlatesModel::ReconstructedFeatureGeometry *rfg =
			dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(rg);
	if (rfg) {
		GPlatesModel::FeatureHandle::weak_ref feature_ref = rfg->feature_ref();
		if ( ! feature_ref.is_valid()) {
			// FIXME:  Replace this exception with a problem-specific exception which
			// doesn't contain a string.
			throw GPlatesGlobal::InternalInconsistencyException(__FILE__, __LINE__,
					"Invalid FeatureHandle::weak_ref returned from proximity tests.");
		}
		d_feature_focus_ptr->set_focus(feature_ref, rfg);
	}
#endif
}


void
GPlatesCanvasTools::PlateClosure::handle_create_new_feature(
	GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	// finalize the new feature with the boundary prop value
	d_plate_closure_widget_ptr->append_boundary_to_feature( feature_ref );
}
