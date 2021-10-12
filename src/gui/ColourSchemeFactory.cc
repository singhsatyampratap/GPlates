/* $Id$ */

/**
 * @file 
 * Contains the implementation of the ColourSchemeFactory class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#include "AgeColourPalettes.h"
#include "ColourSchemeFactory.h"
#include "FeatureColourPalette.h"
#include "GenericColourScheme.h"
#include "PlateIdColourPalettes.h"
#include "SingleColourScheme.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "maths/Real.h"
#include "model/types.h"
#include "model/FeatureHandle.h"
#include "model/FeatureType.h"
#include "property-values/GeoTimeInstant.h"

#include <boost/optional.hpp>

namespace
{
	/**
	 * Extracts the plate ID for use by GenericColourScheme.
	 */
	class PlateIdPropertyExtractor
	{
	public:
		
		typedef GPlatesModel::integer_plate_id_type return_type;

		const boost::optional<return_type>
		operator()(
				const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
		{
			return GPlatesAppLogic::ReconstructionGeometryUtils::get_plate_id(
						&reconstruction_geometry);
		}
	};

	/**
	 * Extracts the age for use by GenericColourScheme.
	 */
	class AgePropertyExtractor
	{
	public:

		typedef GPlatesMaths::Real return_type;

		AgePropertyExtractor(
				const GPlatesAppLogic::ApplicationState &application_state) :
			d_application_state(application_state)
		{  }

		const boost::optional<return_type>
		operator()(
				const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
		{
			boost::optional<GPlatesPropertyValues::GeoTimeInstant> geo_time =
				GPlatesAppLogic::ReconstructionGeometryUtils::get_time_of_formation(
					&reconstruction_geometry);
			if (!geo_time)
			{
				return boost::none;
			}

			if (geo_time->is_distant_past())
			{
				// Distant past.
				// Cannot calculate 'age' from the point of view of the current reconstruction time.
				return GPlatesMaths::Real::negative_infinity();
			}
			else if (geo_time->is_distant_future())
			{
				// Distant future.
				return GPlatesMaths::Real::positive_infinity();
			}
			else
			{
				// Has a real time of formation.
				return GPlatesMaths::Real(
						geo_time->value() - d_application_state.get_current_reconstruction_time());
			}
		}
	
	private:

		const GPlatesAppLogic::ApplicationState &d_application_state;
	};

	/**
	 * Extracts the feature type for use by GenericColourScheme.
	 */
	class FeaturePropertyExtractor
	{
	public:

		typedef GPlatesModel::FeatureType return_type;

		const boost::optional<return_type>
		operator()(
				const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const
		{
			GPlatesModel::FeatureHandle::weak_ref feature_ref;
			if (!GPlatesAppLogic::ReconstructionGeometryUtils::get_feature_ref(
						&reconstruction_geometry, feature_ref))
			{
				return boost::none;
			}
			else
			{
				return feature_ref->feature_type();
			}
		}
	};
}


GPlatesGui::ColourScheme::non_null_ptr_type
GPlatesGui::ColourSchemeFactory::create_single_colour_scheme(
		const Colour &colour)
{
	return new SingleColourScheme(colour);
}


GPlatesGui::ColourScheme::non_null_ptr_type
GPlatesGui::ColourSchemeFactory::create_default_plate_id_colour_scheme()
{
	return new GenericColourScheme<PlateIdPropertyExtractor>(
			new DefaultPlateIdColourPalette());
}


GPlatesGui::ColourScheme::non_null_ptr_type
GPlatesGui::ColourSchemeFactory::create_regional_plate_id_colour_scheme()
{
	return new GenericColourScheme<PlateIdPropertyExtractor>(
			new RegionalPlateIdColourPalette());
}


GPlatesGui::ColourScheme::non_null_ptr_type
GPlatesGui::ColourSchemeFactory::create_default_age_colour_scheme(
		const GPlatesAppLogic::ApplicationState &application_state)
{
	return new GenericColourScheme<AgePropertyExtractor>(
			new DefaultAgeColourPalette(),
			AgePropertyExtractor(application_state));
}


GPlatesGui::ColourScheme::non_null_ptr_type
GPlatesGui::ColourSchemeFactory::create_monochrome_age_colour_scheme(
		const GPlatesAppLogic::ApplicationState &application_state)
{
	return new GenericColourScheme<AgePropertyExtractor>(
			new MonochromeAgeColourPalette(),
			AgePropertyExtractor(application_state));
}


GPlatesGui::ColourScheme::non_null_ptr_type
GPlatesGui::ColourSchemeFactory::create_custom_age_colour_scheme(
		const GPlatesAppLogic::ApplicationState &application_state,
		ColourPalette<GPlatesMaths::Real> *palette)
{
	return new GenericColourScheme<AgePropertyExtractor>(
			palette,
			AgePropertyExtractor(application_state));
}


GPlatesGui::ColourScheme::non_null_ptr_type
GPlatesGui::ColourSchemeFactory::create_default_feature_colour_scheme()
{
	return new GenericColourScheme<FeaturePropertyExtractor>(
			new FeatureColourPalette());
}

