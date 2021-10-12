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

#ifndef GPLATES_PROPERTYVALUES_XSDOUBLE_H
#define GPLATES_PROPERTYVALUES_XSDOUBLE_H

#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::XsDouble, visit_xs_double)

namespace GPlatesPropertyValues {

	class XsDouble :
			public GPlatesModel::PropertyValue {

	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<XsIntger,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<XsDouble,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const XsDouble,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const XsDouble,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		virtual
		~XsDouble() {  }

		static
		const non_null_ptr_type
		create(
				double value) {
			XsDouble::non_null_ptr_type ptr(new XsDouble(value),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const {
			GPlatesModel::PropertyValue::non_null_ptr_type dup(new XsDouble(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		/**
		 * Accesses the double contained within this XsDouble.
		 */
		double
		value() const {
			return d_value;
		}
		
		/**
		 * Set the double value contained within this XsDouble to @a d.
		 *
		 * FIXME: when we have undo/redo, this act should cause
		 * a new revision to be propagated up to the Feature which
		 * contains this PropertyValue.
		 */
		void
		set_value(
				const double &d) {
			d_value = d;
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
			visitor.visit_xs_double(*this);
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
			visitor.visit_xs_double(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		XsDouble(
				double value_) :
			PropertyValue(),
			d_value(value_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		XsDouble(
				const XsDouble &other) :
			PropertyValue(other),
			d_value(other.d_value)
		{  }

	private:

		double d_value;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		XsDouble &
		operator=(const XsDouble &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_XSDouble_H
