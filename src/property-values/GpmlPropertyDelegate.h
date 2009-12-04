/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#ifndef GPLATES_PROPERTYVALUES_GPMLPROPERTYDELEGATE_H
#define GPLATES_PROPERTYVALUES_GPMLPROPERTYDELEGATE_H

#include "TemplateTypeParameterType.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "model/PropertyValue.h"
#include "model/FeatureId.h"
#include "model/PropertyName.h"


// Enable GPlatesFeatureVisitors::getPropertyValue() to work with this property value.
// First parameter is the namespace qualified property value class.
// Second parameter is the name of the feature visitor method that visits the property value.
DECLARE_PROPERTY_VALUE_FINDER(GPlatesPropertyValues::GpmlPropertyDelegate, visit_gpml_property_delegate)

namespace GPlatesPropertyValues {

	class GpmlPropertyDelegate:
			public GPlatesModel::PropertyValue {

	public:
		/**
		 * A convenience typedef for boost::intrusive_ptr<GpmlPropertyDelegate>.
		 */
		typedef boost::intrusive_ptr<GpmlPropertyDelegate>
				maybe_null_ptr_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const GpmlPropertyDelegate>.
		 */
		typedef boost::intrusive_ptr<const GpmlPropertyDelegate>
				maybe_null_ptr_to_const_type;

		/**
		 * A convenience typedef for 
		 * GPlatesUtils::non_null_intrusive_ptr<GpmlPropertyDelegate,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GpmlPropertyDelegate,
				GPlatesUtils::NullIntrusivePointerHandler> 
				non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GpmlPropertyDelegate,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GpmlPropertyDelegate,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;


		virtual
		~GpmlPropertyDelegate() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		const non_null_ptr_type
		create(
				const GPlatesModel::FeatureId &feature_,
				const GPlatesModel::PropertyName &property_,
				const TemplateTypeParameterType &value_type_) {
			non_null_ptr_type ptr(
					new GpmlPropertyDelegate(feature_, property_, value_type_),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const {
			GPlatesModel::PropertyValue::non_null_ptr_type dup(
					new GpmlPropertyDelegate(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		const GPlatesModel::FeatureId
		feature_id() const {
			return d_feature;
		}

		const GPlatesModel::PropertyName
		target_property() const {
			return d_property;
		}

		// Note that no "setter" is provided:  The value type of a GpmlPropertyDelegate
		// instance should never be changed.
		const TemplateTypeParameterType &
		value_type() const {
			return d_value_type;
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
			visitor.visit_gpml_property_delegate(*this);
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
			visitor.visit_gpml_property_delegate(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GpmlPropertyDelegate(
				const GPlatesModel::FeatureId &feature_,
				const GPlatesModel::PropertyName &property_,
				const TemplateTypeParameterType &value_type_):
			PropertyValue(),
			d_feature(feature_),
			d_property(property_),
			d_value_type(value_type_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlPropertyDelegate(
				const GpmlPropertyDelegate &other) :
			PropertyValue(),
			d_feature(other.d_feature),
			d_property(other.d_property),
			d_value_type(other.d_value_type)
		{  }

	private:

		GPlatesModel::FeatureId d_feature;
		GPlatesModel::PropertyName d_property;
		TemplateTypeParameterType d_value_type;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlPropertyDelegate &
		operator=(const GpmlPropertyDelegate &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GPMLPROPERTYDELEGATE_H
