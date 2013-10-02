/* $Id$ */

/**
 * @file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 Geological Survey of Norway
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

#include <iterator>
#include <QObject>

#include "qt-widgets/HellingerDialog.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryProximity.h"
#include "FitToPole.h"


GPlatesCanvasTools::FitToPole::FitToPole(
		const status_bar_callback_type &status_bar_callback,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
		GPlatesViewOperations::RenderedGeometryCollection::MainLayerType main_rendered_layer_type,
		GPlatesQtWidgets::HellingerDialog &hellinger_dialog) :
	CanvasTool(status_bar_callback),
	d_rendered_geom_collection_ptr(&rendered_geom_collection),
	d_hellinger_dialog_ptr(&hellinger_dialog),
	d_mouse_is_over_editable_pick(false),
	d_pick_is_being_dragged(false)
{
}

void
GPlatesCanvasTools::FitToPole::handle_activation()
{
	set_status_bar_message(QT_TR_NOOP("Click to select a pick. Shift+click to edit a pick."));
}

void
GPlatesCanvasTools::FitToPole::handle_deactivation()
{

}


void
GPlatesCanvasTools::FitToPole::handle_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	if (!is_on_earth)
	{
		return;
	}


	GPlatesMaths::ProximityCriteria proximity_criteria(
			point_on_sphere,
			proximity_inclusion_threshold);
	std::vector<GPlatesViewOperations::RenderedGeometryProximityHit> sorted_hits;
	if (GPlatesViewOperations::test_proximity(
				sorted_hits,
				proximity_criteria,
				*d_hellinger_dialog_ptr->get_pick_layer()))
	{
		const unsigned int index = sorted_hits.front().d_rendered_geom_index;
		d_hellinger_dialog_ptr->set_selected_pick(index);
	}
	else
	{
		d_hellinger_dialog_ptr->clear_selection_layer();
	}

	if (d_hellinger_dialog_ptr->is_in_new_point_state())
	{
		//Place the new point here.
		qDebug() << "HLC: updating edit layer";
		d_hellinger_dialog_ptr->update_edit_layer(point_on_sphere);
	}
}



void
GPlatesCanvasTools::FitToPole::handle_move_without_drag(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	GPlatesMaths::ProximityCriteria proximity_criteria(
			point_on_sphere,
			proximity_inclusion_threshold);
	std::vector<GPlatesViewOperations::RenderedGeometryProximityHit> sorted_hits;

	// Check editing layer first
	if (d_hellinger_dialog_ptr->get_editing_layer()->is_active())
	{
		if (GPlatesViewOperations::test_proximity(
					sorted_hits,
					proximity_criteria,
					*d_hellinger_dialog_ptr->get_editing_layer()))
		{
			qDebug() << "moving over editable geom";
			d_mouse_is_over_editable_pick = true;
			d_hellinger_dialog_ptr->set_enlarged_edit_geometry();
		}
		else
		{
			qDebug() << "not moving over editable geom";
			d_mouse_is_over_editable_pick = false;
			d_hellinger_dialog_ptr->set_enlarged_edit_geometry(false);
		}
	}

	sorted_hits.clear();
	if (GPlatesViewOperations::test_proximity(
				sorted_hits,
				proximity_criteria,
				*d_hellinger_dialog_ptr->get_pick_layer()))
	{
		const unsigned int index = sorted_hits.front().d_rendered_geom_index;
		d_hellinger_dialog_ptr->set_hovered_pick(index);
	}
	else
	{
		d_hellinger_dialog_ptr->clear_hovered_layer();
	}
}

void
GPlatesCanvasTools::FitToPole::handle_shift_left_click(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{
	if (!is_on_earth)
	{
		return;
	}

	GPlatesMaths::ProximityCriteria proximity_criteria(
			point_on_sphere,
			proximity_inclusion_threshold);
	std::vector<GPlatesViewOperations::RenderedGeometryProximityHit> sorted_hits;
	if (GPlatesViewOperations::test_proximity(
				sorted_hits,
				proximity_criteria,
				*d_hellinger_dialog_ptr->get_pick_layer()))
	{
		const unsigned int index = sorted_hits.front().d_rendered_geom_index;
		d_hellinger_dialog_ptr->set_selected_pick(index);

		if (!d_hellinger_dialog_ptr->is_in_new_point_state())
		{
			d_hellinger_dialog_ptr->edit_current_pick();
		}
	}
	else
	{
		d_hellinger_dialog_ptr->clear_selection_layer();
	}
}

void
GPlatesCanvasTools::FitToPole::handle_left_press(
		const GPlatesMaths::PointOnSphere &point_on_sphere,
		bool is_on_earth,
		double proximity_inclusion_threshold)
{

	if (!d_mouse_is_over_editable_pick)
	{
		return;
	}


	GPlatesMaths::ProximityCriteria proximity_criteria(
			point_on_sphere,
			proximity_inclusion_threshold);
	std::vector<GPlatesViewOperations::RenderedGeometryProximityHit> sorted_hits;

	// Check editing layer first
	if (d_hellinger_dialog_ptr->get_editing_layer()->is_active())
	{
		if (GPlatesViewOperations::test_proximity(
					sorted_hits,
					proximity_criteria,
					*d_hellinger_dialog_ptr->get_editing_layer()))
		{
			d_pick_is_being_dragged = true;
		}
		else
		{
			d_mouse_is_over_editable_pick = false;
		}
	}
}

void
GPlatesCanvasTools::FitToPole::handle_left_release_after_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclusion_threshold,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
{
	d_pick_is_being_dragged = false;
	d_hellinger_dialog_ptr->set_enlarged_edit_geometry(false);
	d_hellinger_dialog_ptr->update_edit_layer(current_point_on_sphere);

}

void
GPlatesCanvasTools::FitToPole::handle_left_drag(
		const GPlatesMaths::PointOnSphere &initial_point_on_sphere,
		bool was_on_earth,
		double initial_proximity_inclusion_threshold,
		const GPlatesMaths::PointOnSphere &current_point_on_sphere,
		bool is_on_earth,
		double current_proximity_inclusion_threshold,
		const boost::optional<GPlatesMaths::PointOnSphere> &centre_of_viewport)
{
	if (d_pick_is_being_dragged)
	{
		d_hellinger_dialog_ptr->update_edit_layer(current_point_on_sphere);
	}
}

void
GPlatesCanvasTools::FitToPole::paint()
{
#if 0
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
#endif
}



