/* $Id$ */

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

#include "DigitiseGeometry.h"

#include "qt-widgets/GlobeCanvas.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/DigitisationWidget.h"
#include "gui/RenderedGeometryLayers.h"
#include "maths/LatLonPointConversions.h"


GPlatesCanvasTools::DigitiseGeometry::DigitiseGeometry(
		GPlatesGui::Globe &globe_,
		GPlatesQtWidgets::GlobeCanvas &globe_canvas_,
		GPlatesGui::RenderedGeometryLayers &layers,
		const GPlatesQtWidgets::ViewportWindow &view_state_,
		GPlatesQtWidgets::DigitisationWidget &digitisation_widget_,
		GPlatesQtWidgets::DigitisationWidget::GeometryType geom_type_):
	CanvasTool(globe_, globe_canvas_),
	d_layers_ptr(&layers),
	d_view_state_ptr(&view_state_),
	d_digitisation_widget_ptr(&digitisation_widget_),
	d_default_geom_type(geom_type_)
{  }


void
GPlatesCanvasTools::DigitiseGeometry::handle_activation()
{
	// Clicking these canvas tools changes the type of geometry the user
	// wishes to create, and may adjust the current table of coordinates accordingly.
	d_digitisation_widget_ptr->change_geometry_type(d_default_geom_type);

	// FIXME:  We may have to adjust the message if we are using a Map View.
	if (d_digitisation_widget_ptr->geometry_type() ==
			GPlatesQtWidgets::DigitisationWidget::MULTIPOINT) {
		d_view_state_ptr->status_message(QObject::tr(
				"Click to draw a new point."
				" Ctrl+drag to re-orient the globe."));
	} else {
		d_view_state_ptr->status_message(QObject::tr(
				"Click to draw a new vertex."
				" Ctrl+drag to re-orient the globe."));
	}

	d_layers_ptr->show_only_digitisation_layer();
	globe_canvas().update_canvas();
}


void
GPlatesCanvasTools::DigitiseGeometry::handle_deactivation()
{  }


void
GPlatesCanvasTools::DigitiseGeometry::handle_left_click(
		const GPlatesMaths::PointOnSphere &click_pos_on_globe,
		const GPlatesMaths::PointOnSphere &oriented_click_pos_on_globe,
		bool is_on_globe)
{
	const GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(
			oriented_click_pos_on_globe);
	
	// Plain and simple append point to digitisation widget, default geometry.
	d_digitisation_widget_ptr->append_point_to_geometry(llp.latitude(), llp.longitude());
}
