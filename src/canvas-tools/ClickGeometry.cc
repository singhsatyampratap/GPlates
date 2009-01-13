/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008 The University of Sydney, Australia
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

#include "ClickGeometry.h"

#include "gui/ProximityTests.h"
#include "global/InternalInconsistencyException.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/FeaturePropertiesDialog.h"
#include "view-operations/RenderedGeometryCollection.h"


const GPlatesCanvasTools::ClickGeometry::non_null_ptr_type
GPlatesCanvasTools::ClickGeometry::create(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesGui::Globe &globe,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas,
		const GPlatesQtWidgets::ViewportWindow &view_state,
		GPlatesGui::FeatureTableModel &clicked_table_model,
		GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog,
		GPlatesGui::FeatureFocus &feature_focus,
		GPlatesGui::GeometryFocusHighlight &geometry_focus_highlight)
{
	return ClickGeometry::non_null_ptr_type(
			new ClickGeometry(
					rendered_geom_collection,
					globe,
					globe_canvas,
					view_state,
					clicked_table_model,
					fp_dialog,
					feature_focus,
					geometry_focus_highlight),
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesCanvasTools::ClickGeometry::ClickGeometry(
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesGui::FeatureTableModel &clicked_table_model_,
		GPlatesQtWidgets::FeaturePropertiesDialog &fp_dialog_,
		GPlatesGui::FeatureFocus &feature_focus_,
		GPlatesGui::GeometryFocusHighlight &geometry_focus_highlight_):
	CanvasTool(globe_, globe_canvas_),
	d_rendered_geom_collection(&rendered_geom_collection),
	d_view_state_ptr(&view_state_),
	d_clicked_table_model_ptr(&clicked_table_model_),
	d_fp_dialog_ptr(&fp_dialog_),
	d_feature_focus_ptr(&feature_focus_),
	d_geometry_focus_highlight(&geometry_focus_highlight_)
{
}

void
GPlatesCanvasTools::ClickGeometry::handle_activation()
{
	// FIXME:  We may have to adjust the message if we are using a Map View.
	d_view_state_ptr->status_message(QObject::tr(
			"Click a geometry to choose a feature."
			" Shift+click to query immediately."
			" Ctrl+drag to re-orient the globe."));

	// Activate the geometry focus hightlight layer.
	d_rendered_geom_collection->set_main_layer_active(
			GPlatesViewOperations::RenderedGeometryCollection::GEOMETRY_FOCUS_HIGHLIGHT_LAYER);

	// We explicitly draw the focused geometry (if any) because currently it only
	// gets drawn when the feature is given focus which may not happen when switching
	// tools.
	d_geometry_focus_highlight->draw_focused_geometry();
}


void
GPlatesCanvasTools::ClickGeometry::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	d_view_state_ptr->highlight_first_clicked_feature_table_row();


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

	if (sorted_hits.size() == 0) {
		// User clicked on empty space! Clear the currently focused feature.
		d_feature_focus_ptr->unset_focus();
		emit no_hits_found();
		return;
	}
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

#if 0  // It seems it's not necessary to set the feature focus here, as it's already being set elsewhere.
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
GPlatesCanvasTools::ClickGeometry::handle_shift_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	handle_left_click(click_pos_on_globe, oriented_click_pos_on_globe, is_on_globe);
	// If there is a feature focused, we'll assume that the user wants to look at it in detail.
	if (d_feature_focus_ptr->is_valid()) {
		fp_dialog().choose_query_widget_and_open();
	}
}
