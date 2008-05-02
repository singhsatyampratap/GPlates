/* $Id$ */

/**
 * @file 
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

#include "StructurePropertyCreatorMap.h"

#define GET_PROP_VAL_NAME(create_prop) GPlatesFileIO::PropertyCreationUtils::create_prop##_as_prop_val


using GPlatesPropertyValues::TemplateTypeParameterType;

GPlatesFileIO::StructurePropertyCreatorMap *
GPlatesFileIO::StructurePropertyCreatorMap::d_instance = NULL;


GPlatesFileIO::StructurePropertyCreatorMap *
GPlatesFileIO::StructurePropertyCreatorMap::instance()
{
	if (d_instance == NULL) {
		d_instance = new StructurePropertyCreatorMap();
	}
	return d_instance;
}


GPlatesFileIO::StructurePropertyCreatorMap::StructurePropertyCreatorMap()
{
	d_map[TemplateTypeParameterType::create_xsi("boolean")] =
		GET_PROP_VAL_NAME(create_xs_boolean);
	d_map[TemplateTypeParameterType::create_xsi("integer")] =
		GET_PROP_VAL_NAME(create_xs_integer);
	d_map[TemplateTypeParameterType::create_xsi("double")] =
		GET_PROP_VAL_NAME(create_xs_double);
	d_map[TemplateTypeParameterType::create_xsi("string")] =
		GET_PROP_VAL_NAME(create_xs_string);

	d_map[TemplateTypeParameterType::create_gpml("measure")] = 
		GET_PROP_VAL_NAME(create_measure);
	d_map[TemplateTypeParameterType::create_gpml("revisionId")] = 
		GET_PROP_VAL_NAME(create_gpml_revision_id);
	d_map[TemplateTypeParameterType::create_gpml("plateId")] = 
		GET_PROP_VAL_NAME(create_plate_id);
	d_map[TemplateTypeParameterType::create_gml("TimeInstant")] = 
		GET_PROP_VAL_NAME(create_time_instant);
	d_map[TemplateTypeParameterType::create_gml("TimePeriod")] = 
		GET_PROP_VAL_NAME(create_time_period);
	d_map[TemplateTypeParameterType::create_gpml("PolarityChronId")] = 
		GET_PROP_VAL_NAME(create_polarity_chron_id);
	d_map[TemplateTypeParameterType::create_gpml("OldPlatesHeader")] = 
		GET_PROP_VAL_NAME(create_old_plates_header);
	d_map[TemplateTypeParameterType::create_gpml("ConstantValue")] = 
		GET_PROP_VAL_NAME(create_constant_value);
	d_map[TemplateTypeParameterType::create_gpml("HotSpotTrailMark")] = 
		GET_PROP_VAL_NAME(create_hot_spot_trail_mark);
	d_map[TemplateTypeParameterType::create_gpml("IrregularSampling")] = 
		GET_PROP_VAL_NAME(create_irregular_sampling);
	d_map[TemplateTypeParameterType::create_gpml("PiecewiseAggregation")] = 
		GET_PROP_VAL_NAME(create_piecewise_aggregation);
	d_map[TemplateTypeParameterType::create_gpml("AxisAngleFiniteRotation")] = 
		GET_PROP_VAL_NAME(create_finite_rotation);
	d_map[TemplateTypeParameterType::create_gpml("ZeroFiniteRotation")] = 
		GET_PROP_VAL_NAME(create_finite_rotation);
	d_map[TemplateTypeParameterType::create_gpml("FiniteRotationSlerp")] = 
		GET_PROP_VAL_NAME(create_finite_rotation_slerp);
	d_map[TemplateTypeParameterType::create_gpml("InterpolationFunction")] = 
		GET_PROP_VAL_NAME(create_interpolation_function);
	d_map[TemplateTypeParameterType::create_gpml("FeatureReference")] = 
		GET_PROP_VAL_NAME(create_feature_reference);
	d_map[TemplateTypeParameterType::create_gml("OrientableCurve")] = 
		GET_PROP_VAL_NAME(create_orientable_curve);
	d_map[TemplateTypeParameterType::create_gml("LineString")] = 
		GET_PROP_VAL_NAME(create_line_string);
	d_map[TemplateTypeParameterType::create_gml("Point")] = 
		GET_PROP_VAL_NAME(create_point);

}