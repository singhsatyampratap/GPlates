/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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
 
#ifndef GPLATES_PROPERTYVALUES_GPMLOLDPLATESHEADER_H
#define GPLATES_PROPERTYVALUES_GPMLOLDPLATESHEADER_H

#include "model/PropertyValue.h"
#include "model/types.h"
#include "TextContent.h"

namespace GPlatesPropertyValues 
{

	class GpmlOldPlatesHeader:
			public GPlatesModel::PropertyValue
	{
	public:

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlOldPlatesHeader>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlOldPlatesHeader>
				non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlOldPlatesHeader>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlOldPlatesHeader>
				non_null_ptr_to_const_type;

		virtual
		~GpmlOldPlatesHeader() 
		{  }
		
		static
		const non_null_ptr_type
		create(		
				unsigned int region_number,
				unsigned int reference_number,
				unsigned int string_number,
				const UnicodeString &geographic_description,
				GPlatesModel::integer_plate_id_type plate_id_number,
				double age_of_appearance,
				double age_of_disappearance,
				const UnicodeString &data_type_code,
				unsigned int data_type_code_number,
				const UnicodeString &data_type_code_number_additional,
				GPlatesModel::integer_plate_id_type conjugate_plate_id_number,
				unsigned int colour_code,
				unsigned int number_of_points)
		{
			non_null_ptr_type ptr(
					*(new GpmlOldPlatesHeader(region_number, reference_number, 
					string_number, geographic_description, plate_id_number, 
					age_of_appearance, age_of_disappearance, data_type_code,
					data_type_code_number, data_type_code_number_additional,
					conjugate_plate_id_number, colour_code, number_of_points)));
					
			return ptr;
		}

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const 
		{
			GPlatesModel::PropertyValue::non_null_ptr_type dup(*(new GpmlOldPlatesHeader(*this)));
			return dup;
		}

		unsigned int
		region_number() const 
		{
			return d_region_number;
		}

		unsigned int
		reference_number() const 
		{
			return d_reference_number;
		}
		
		unsigned int
		string_number() const 
		{
			return d_string_number;
		}
		
		const UnicodeString &
		geographic_description() const 
		{
			return d_geographic_description.get();
		}
		
		GPlatesModel::integer_plate_id_type
		plate_id_number() const 
		{
			return d_plate_id_number;
		}
		
		double
		age_of_appearance() const 
		{
			return d_age_of_appearance;
		}
		
		double
		age_of_disappearance() const 
		{
			return d_age_of_disappearance;
		}
		
		const UnicodeString & 
		data_type_code() const 
		{
			return d_data_type_code.get();
		}
		
		unsigned int
		data_type_code_number() const 
		{
			return d_data_type_code_number;
		}
		
		const UnicodeString & 
		data_type_code_number_additional() const 
		{
			return d_data_type_code_number_additional.get();
		}
		
		GPlatesModel::integer_plate_id_type
		conjugate_plate_id_number() const 
		{
			return d_conjugate_plate_id_number;
		}
		
		unsigned int
		colour_code() const 
		{
			return d_colour_code;
		}
			
		unsigned int
		number_of_points() const 
		{
			return d_number_of_points;
		}

		
		/**
		 * Accept a ConstFeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::ConstFeatureVisitor &visitor) const {
			visitor.visit_gpml_old_plates_header(*this);
		}

		/**
		 * Accept a FeatureVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				GPlatesModel::FeatureVisitor &visitor) {
			visitor.visit_gpml_old_plates_header(*this);
		}

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlOldPlatesHeader(
				unsigned int region_number_,
				unsigned int reference_number_,
				unsigned int string_number_,
				const UnicodeString &geographic_description_,
				GPlatesModel::integer_plate_id_type plate_id_number_,
				double age_of_appearance_,
				double age_of_disappearance_,
				const UnicodeString &data_type_code_,
				unsigned int data_type_code_number_,
				const UnicodeString &data_type_code_number_additional_,
				GPlatesModel::integer_plate_id_type conjugate_plate_id_number_,
				unsigned int colour_code_,
				unsigned int number_of_points_):
			PropertyValue(),
			d_region_number(region_number_),
			d_reference_number(reference_number_),
			d_string_number(string_number_),
			d_geographic_description(geographic_description_),
			d_plate_id_number(plate_id_number_),
			d_age_of_appearance(age_of_appearance_),
			d_age_of_disappearance(age_of_disappearance_),
			d_data_type_code(data_type_code_),
			d_data_type_code_number(data_type_code_number_),
			d_data_type_code_number_additional(data_type_code_number_additional_),
			d_conjugate_plate_id_number(conjugate_plate_id_number_),
			d_colour_code(colour_code_),
			d_number_of_points(number_of_points_)
		{ }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlOldPlatesHeader(
				const GpmlOldPlatesHeader &other) :
			PropertyValue(),
			d_region_number(other.d_region_number),
			d_reference_number(other.d_reference_number),
			d_string_number(other.d_string_number),
			d_geographic_description(other.d_geographic_description),
			d_plate_id_number(other.d_plate_id_number),
			d_age_of_appearance(other.d_age_of_appearance),
			d_age_of_disappearance(other.d_age_of_disappearance),
			d_data_type_code(other.d_data_type_code),
			d_data_type_code_number(other.d_data_type_code_number),
			d_data_type_code_number_additional(other.d_data_type_code_number_additional),
			d_conjugate_plate_id_number(other.d_conjugate_plate_id_number),
			d_colour_code(other.d_colour_code),
			d_number_of_points(other.d_number_of_points)
		{ }

	private:
		unsigned int d_region_number;
		unsigned int d_reference_number;
		unsigned int d_string_number;
		TextContent d_geographic_description;
		GPlatesModel::integer_plate_id_type d_plate_id_number;
		double d_age_of_appearance;
		double d_age_of_disappearance;
		TextContent d_data_type_code;
		unsigned int d_data_type_code_number;
		TextContent d_data_type_code_number_additional;
		GPlatesModel::integer_plate_id_type d_conjugate_plate_id_number;
		unsigned int d_colour_code;
		unsigned int d_number_of_points;
		
		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlOldPlatesHeader &
		operator=(
				const GpmlOldPlatesHeader &);
	};
}

#endif