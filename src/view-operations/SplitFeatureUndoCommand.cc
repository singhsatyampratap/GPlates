/* $Id$ */

/**
 * \file 
 * 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
#include "SplitFeatureUndoCommand.h"

void
GPlatesViewOperations::SplitFeatureUndoCommand::redo()
{
	RenderedGeometryCollection::UpdateGuard update_guard;

	GPlatesModel::FeatureHandle::weak_ref feature_ref = 
		d_feature_focus->focused_feature();

	if(!feature_ref.is_valid())
	{
		return;
	}

	d_feature_collection_ref = 
		*GPlatesAppLogic::get_feature_collection_containing_feature(
				d_view_state->get_application_state().get_feature_collection_file_state(),
				feature_ref);
	
	if(!d_feature_collection_ref.is_valid())
	{
		return;
	}

	// We exclude geometry property when cloning the feature because the new
	// split geometry property will be appended to the cloned feature later.
	d_new_feature_1 = 
		GPlatesModel::ModelUtils::deep_clone_feature(
				feature_ref,
				d_feature_collection_ref,
				d_view_state->get_application_state().get_model_interface(),
				&GPlatesFeatureVisitors::is_not_geometry_property);

	d_new_feature_2 = 
		GPlatesModel::ModelUtils::deep_clone_feature(
				feature_ref,
				d_feature_collection_ref,
				d_view_state->get_application_state().get_model_interface(),
				&GPlatesFeatureVisitors::is_not_geometry_property);

	GPlatesModel::FeatureHandle::children_iterator property_iter = 
		*GPlatesFeatureVisitors::find_first_geometry_property(feature_ref);

	GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_on_sphere =
		GPlatesFeatureVisitors::find_first_geometry(property_iter);

	std::vector<GPlatesMaths::PointOnSphere> points;
	GPlatesUtils::GeometryUtil::get_geometry_points(
			*geometry_on_sphere,
			points);

	GeometryBuilder::PointIndex point_index_to_split;
	if (d_oriented_pos_on_globe)
	{
		points.insert(
				points.begin()+ d_point_index_to_insert_at, 
				*d_oriented_pos_on_globe);
	}
	
	point_index_to_split = d_point_index_to_insert_at + 1;
	
	//if the point is at the beginning or the end of the ployline, we do nothing but return
	if (d_point_index_to_insert_at == 0 ||
		(points.begin() + d_point_index_to_insert_at + 1) == points.end())
	{
		return;
	}
	
	GPlatesModel::PropertyName property_name = 
		(*property_iter)->property_name(); 

	// TODO: currently the ployline type has been hardcoded here, 
	// we need to support other geometry type in the future
	GPlatesModel::ModelUtils::append_property_value_to_feature(
			*GPlatesUtils::GeometryUtil::create_geometry_property_value(
					points.begin(), 
					points.begin() + point_index_to_split,
					GPlatesViewOperations::GeometryType::POLYLINE),  
			property_name,
			*d_new_feature_1);
	
	GPlatesModel::ModelUtils::append_property_value_to_feature(
			*GPlatesUtils::GeometryUtil::create_geometry_property_value(
					points.begin() + point_index_to_split -1, 
					points.end(),
					GPlatesViewOperations::GeometryType::POLYLINE),  
			property_name,
			*d_new_feature_2);

	d_feature_focus->set_focus(
			*d_new_feature_1,
			*GPlatesFeatureVisitors::find_first_geometry_property(
					*d_new_feature_1));
	d_feature_focus->announce_modification_of_focused_feature();
}

void
GPlatesViewOperations::SplitFeatureUndoCommand::undo()
{
	RenderedGeometryCollection::UpdateGuard update_guard;
	
	GPlatesModel::ModelUtils::remove_feature(
			d_feature_collection_ref, *d_new_feature_1);
	GPlatesModel::ModelUtils::remove_feature(
			d_feature_collection_ref, *d_new_feature_2);
}
