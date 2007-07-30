/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include "ModelUtility.h"
#include "DummyTransactionHandle.h"
#include "FeatureHandle.h"
#include "FeatureRevision.h"
#include "GmlLineString.h"
#include "GmlOrientableCurve.h"
#include "GmlTimePeriod.h"
#include "GpmlConstantValue.h"
#include "GpmlFiniteRotationSlerp.h"
#include "GpmlFiniteRotation.h"
#include "GpmlIrregularSampling.h"
#include "GpmlPlateId.h"
#include "GpmlTimeSample.h"
#include "InlinePropertyContainer.h"
#include "XsString.h"

const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_reconstruction_plate_id(
		const unsigned long &plate_id)
{
	PropertyValue::non_null_ptr_type gpml_plate_id =
			GpmlPlateId::create(plate_id);

	UnicodeString template_type_parameter_type_string("gpml:plateId");
	TemplateTypeParameterType template_type_parameter_type(template_type_parameter_type_string);
	PropertyValue::non_null_ptr_type gpml_plate_id_constant_value =
			GpmlConstantValue::create(gpml_plate_id, template_type_parameter_type);

	UnicodeString property_name_string("gpml:reconstructionPlateId");
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gpml_plate_id_constant_value, xml_attributes);

	return inline_property_container;
}


const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_reference_frame_plate_id(
		const unsigned long &plate_id,
		const char *which_reference_frame)
{
	PropertyValue::non_null_ptr_type gpml_plate_id =
			GpmlPlateId::create(plate_id);

	UnicodeString property_name_string(which_reference_frame);
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gpml_plate_id, xml_attributes);

	return inline_property_container;
}


const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_centre_line_of(
		const double *points,
		unsigned num_points)
{
	std::vector<double> gml_pos_list(points, points + num_points);
	PropertyValue::non_null_ptr_type gml_line_string =
			GmlLineString::create(gml_pos_list);

	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name("orientation");
	XmlAttributeValue xml_attribute_value("+");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));
	PropertyValue::non_null_ptr_type gml_orientable_curve =
			GmlOrientableCurve::create(gml_line_string, xml_attributes);

	UnicodeString template_type_parameter_type_string("gml:OrientableCurve");
	TemplateTypeParameterType template_type_parameter_type(template_type_parameter_type_string);
	PropertyValue::non_null_ptr_type gml_orientable_curve_constant_value =
			GpmlConstantValue::create(gml_orientable_curve, template_type_parameter_type);

	UnicodeString property_name_string("gpml:centreLineOf");
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes2;
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gml_orientable_curve_constant_value, xml_attributes2);

	return inline_property_container;
}


const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_valid_time(
		const GeoTimeInstant &geo_time_instant_begin,
		const GeoTimeInstant &geo_time_instant_end)
{
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name("frame");
	XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	GmlTimeInstant::non_null_ptr_type gml_time_instant_begin =
			GmlTimeInstant::create(geo_time_instant_begin, xml_attributes);

	GmlTimeInstant::non_null_ptr_type gml_time_instant_end =
			GmlTimeInstant::create(geo_time_instant_end, xml_attributes);

	PropertyValue::non_null_ptr_type gml_time_period =
			GmlTimePeriod::create(gml_time_instant_begin, gml_time_instant_end);

	UnicodeString property_name_string("gml:validTime");
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes2;
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gml_time_period, xml_attributes2);

	return inline_property_container;
}


const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_description(
		const UnicodeString &description)
{
	PropertyValue::non_null_ptr_type gml_description = XsString::create(description);

	UnicodeString property_name_string("gml:description");
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gml_description, xml_attributes);

	return inline_property_container;
}


const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_name(
		const UnicodeString &name,
		const UnicodeString &codespace)
{
	PropertyValue::non_null_ptr_type gml_name = XsString::create(name);

	UnicodeString property_name_string("gml:name");
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name("codeSpace");
	XmlAttributeValue xml_attribute_value(codespace);
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gml_name, xml_attributes);

	return inline_property_container;
}

const GPlatesModel::PropertyContainer::non_null_ptr_type
GPlatesModel::ModelUtility::create_total_reconstruction_pole(
		const TotalReconstructionPoleData *five_tuples,
		unsigned num_five_tuples)
{
	std::vector<GpmlTimeSample> time_samples;
	TemplateTypeParameterType value_type("gpml:FiniteRotation");

	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
	XmlAttributeName xml_attribute_name("frame");
	XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
	xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

	for (unsigned i = 0; i < num_five_tuples; ++i) {
		std::pair<double, double> gpml_euler_pole =
				std::make_pair(five_tuples[i].lon_of_euler_pole, five_tuples[i].lat_of_euler_pole);
		GpmlFiniteRotation::non_null_ptr_type gpml_finite_rotation =
				GpmlFiniteRotation::create(gpml_euler_pole,
				five_tuples[i].rotation_angle);

		GeoTimeInstant geo_time_instant(five_tuples[i].time);
		GmlTimeInstant::non_null_ptr_type gml_time_instant =
				GmlTimeInstant::create(geo_time_instant, xml_attributes);

		UnicodeString comment_string(five_tuples[i].comment);
		XsString::non_null_ptr_type gml_description =
				XsString::create(comment_string);

		time_samples.push_back(GpmlTimeSample(gpml_finite_rotation, gml_time_instant,
				get_intrusive_ptr(gml_description), value_type));
	}

	GpmlInterpolationFunction::non_null_ptr_type gpml_finite_rotation_slerp =
			GpmlFiniteRotationSlerp::create(value_type);

	PropertyValue::non_null_ptr_type gpml_irregular_sampling =
			GpmlIrregularSampling::create(time_samples,
			GPlatesContrib::get_intrusive_ptr(gpml_finite_rotation_slerp), value_type);

	UnicodeString property_name_string("gpml:totalReconstructionPole");
	PropertyName property_name(property_name_string);
	std::map<XmlAttributeName, XmlAttributeValue> xml_attributes2;
	PropertyContainer::non_null_ptr_type inline_property_container =
			InlinePropertyContainer::create(property_name,
			gpml_irregular_sampling, xml_attributes2);

	return inline_property_container;
}


const GPlatesModel::FeatureHandle::weak_ref
GPlatesModel::ModelUtility::create_total_recon_seq(
		ModelInterface &model,
		FeatureCollectionHandle::weak_ref &target_collection,
		const unsigned long &fixed_plate_id,
		const unsigned long &moving_plate_id,
		const TotalReconstructionPoleData *five_tuples,
		unsigned num_five_tuples)
{
	UnicodeString feature_type_string("gpml:TotalReconstructionSequence");
	FeatureType feature_type(feature_type_string);
	FeatureHandle::weak_ref feature_handle =
			model.create_feature(feature_type, target_collection);

	PropertyContainer::non_null_ptr_type total_reconstruction_pole_container =
			create_total_reconstruction_pole(five_tuples, num_five_tuples);
	PropertyContainer::non_null_ptr_type fixed_reference_frame_container =
			create_reference_frame_plate_id(fixed_plate_id, "gpml:fixedReferenceFrame");
	PropertyContainer::non_null_ptr_type moving_reference_frame_container =
			create_reference_frame_plate_id(moving_plate_id, "gpml:movingReferenceFrame");

	DummyTransactionHandle pc1(__FILE__, __LINE__);
	feature_handle->append_property_container(total_reconstruction_pole_container, pc1);
	pc1.commit();

	DummyTransactionHandle pc2(__FILE__, __LINE__);
	feature_handle->append_property_container(fixed_reference_frame_container, pc2);
	pc2.commit();

	DummyTransactionHandle pc3(__FILE__, __LINE__);
	feature_handle->append_property_container(moving_reference_frame_container, pc3);
	pc3.commit();

	return feature_handle;
}