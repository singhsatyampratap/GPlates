/* $Id$ */

/**
 * \file 
 * Contains a collection of functors that extract properties from ReconstructionGeometry.
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

#include "PropertyExtractors.h"


const boost::optional<GPlatesAppLogic::PlateIdPropertyExtractor::return_type>
GPlatesAppLogic::PlateIdPropertyExtractor::operator()(
		const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
{
	return ReconstructionGeometryUtils::get_plate_id(
				&reconstruction_geometry);
}


const boost::optional<GPlatesAppLogic::AgePropertyExtractor::return_type>
GPlatesAppLogic::AgePropertyExtractor::operator()(
		const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
{
	boost::optional<GPlatesPropertyValues::GeoTimeInstant> geo_time =
		ReconstructionGeometryUtils::get_time_of_formation(
			&reconstruction_geometry);
	if (!geo_time)
	{
		return boost::none;
	}

	if (geo_time->is_distant_past())
	{
		// Distant past.
		// Cannot calculate 'age' from the point of view of the current reconstruction time.
		return GPlatesMaths::Real::positive_infinity();
	}
	else if (geo_time->is_distant_future())
	{
		// Distant future.
		return GPlatesMaths::Real::negative_infinity();
	}
	else
	{
		// Has a real time of formation.
		return GPlatesMaths::Real(
				geo_time->value() - d_application_state.get_current_reconstruction_time());
	}
}


const boost::optional<GPlatesAppLogic::FeatureTypePropertyExtractor::return_type>
GPlatesAppLogic::FeatureTypePropertyExtractor::operator()(
		const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
{
	GPlatesModel::FeatureHandle::weak_ref feature_ref;
	if (!ReconstructionGeometryUtils::get_feature_ref(
				&reconstruction_geometry, feature_ref))
	{
		return boost::none;
	}
	else
	{
		return feature_ref->feature_type();
	}
}
