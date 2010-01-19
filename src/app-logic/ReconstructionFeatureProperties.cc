/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#include "ReconstructionFeatureProperties.h"

#include "model/PropertyName.h"

#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GmlTimePeriod.h"


GPlatesAppLogic::ReconstructionFeatureProperties::ReconstructionFeatureProperties(
		const double &recon_time) :
	d_recon_time(recon_time),
	d_feature_is_defined_at_recon_time(true)
{
}


bool
GPlatesAppLogic::ReconstructionFeatureProperties::initialise_pre_feature_properties(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	d_recon_plate_id = boost::none;
	d_feature_is_defined_at_recon_time = true;
	d_recon_plate_id = boost::none;
	d_time_of_appearance = boost::none;
	d_time_of_dissappearance = boost::none;

	return true;
}


void
GPlatesAppLogic::ReconstructionFeatureProperties::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period)
{
	static const GPlatesModel::PropertyName valid_time_property_name =
		GPlatesModel::PropertyName::create_gml("validTime");

	// Note that we're going to assume that we're in a property...
	if (current_top_level_propname() == valid_time_property_name)
	{
		// This time period is the "valid time" time period.
		if ( ! gml_time_period.contains(d_recon_time))
		{
			// Oh no!  This feature instance is not defined at the recon time!
			d_feature_is_defined_at_recon_time = false;
		}
		// Also, cache the time of appearance/dissappearance.
		d_time_of_appearance = gml_time_period.begin()->time_position();
		d_time_of_dissappearance = gml_time_period.end()->time_position();
	}
}


void
GPlatesAppLogic::ReconstructionFeatureProperties::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	gpml_constant_value.value()->accept_visitor(*this);
}


void
GPlatesAppLogic::ReconstructionFeatureProperties::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	static GPlatesModel::PropertyName reconstruction_plate_id_property_name =
			GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

	// Note that we're going to assume that we're in a property...
	if (current_top_level_propname() == reconstruction_plate_id_property_name)
	{
		// This plate ID is the reconstruction plate ID.
		d_recon_plate_id = gpml_plate_id.value();
	}
}