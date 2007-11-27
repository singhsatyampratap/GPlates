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

#ifndef GPLATES_PROPERTYVALUES_GMLTIMEINSTANT_H
#define GPLATES_PROPERTYVALUES_GMLTIMEINSTANT_H

#include <map>
#include "model/PropertyValue.h"
#include "GeoTimeInstant.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"


namespace GPlatesPropertyValues {

	class GmlTimeInstant :
			public GPlatesModel::PropertyValue {

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlTimeInstant>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlTimeInstant> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GmlTimeInstant>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlTimeInstant>
				non_null_ptr_to_const_type;

		virtual
		~GmlTimeInstant() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				const GeoTimeInstant &time_position_,
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
						time_position_xml_attributes_) {
			non_null_ptr_type ptr(
					*(new GmlTimeInstant(time_position_,
							time_position_xml_attributes_)));
			return ptr;
		}

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const {
			GPlatesModel::PropertyValue::non_null_ptr_type dup(*(new GmlTimeInstant(*this)));
			return dup;
		}

		const GeoTimeInstant &
		time_position() const {
			return d_time_position;
		}

		void
		set_time_position(
				const GeoTimeInstant &tp) {
			d_time_position = tp;
		}

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map?  (For consistency with the non-const
		// overload...)
		const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
		time_position_xml_attributes() const {
			return d_time_position_xml_attributes;
		}

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map, as well as per-index assignment (setter) and
		// removal operations?  This would ensure that revisioning is correctly handled...
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
		time_position_xml_attributes() {
			return d_time_position_xml_attributes;
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
			visitor.visit_gml_time_instant(*this);
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
			visitor.visit_gml_time_instant(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlTimeInstant(
				const GeoTimeInstant &time_position_,
				const std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> &
						time_position_xml_attributes_):
			PropertyValue(),
			d_time_position(time_position_),
			d_time_position_xml_attributes(time_position_xml_attributes_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlTimeInstant(
				const GmlTimeInstant &other) :
			PropertyValue(),
			d_time_position(other.d_time_position),
			d_time_position_xml_attributes(other.d_time_position_xml_attributes)
		{  }

	private:

		GeoTimeInstant d_time_position;
		std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
				d_time_position_xml_attributes;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlTimeInstant &
		operator=(const GmlTimeInstant &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLTIMEINSTANT_H