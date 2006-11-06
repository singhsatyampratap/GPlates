/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef GPLATES_MODEL_PROPERTYCONTAINER_H
#define GPLATES_MODEL_PROPERTYCONTAINER_H

#include <unicode/unistr.h>
#include <boost/intrusive_ptr.hpp>
#include "PropertyName.h"


namespace GPlatesModel {

	class PropertyContainer {

	public:

		typedef long ref_count_type;

		virtual
		~PropertyContainer()
		{ }

		/**
		 * Construct a PropertyContainer instance with the given property name.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		PropertyContainer(
				const PropertyName &property_name_) :
			d_ref_count(0),
			d_property_name(property_name_)
		{ }

		/**
		 * Construct a PropertyContainer instance which is a copy of @a other.
		 *
		 * Since this class is an abstract class, this constructor can never be invoked
		 * other than explicitly in the initialiser lists of derived classes. 
		 * Nevertheless, the initialiser lists of derived classes @em do need to invoke it
		 * explicitly, since this class contains members which need to be initialised.
		 */
		PropertyContainer(
				const PropertyContainer &other) :
			d_ref_count(other.d_ref_count),
			d_property_name(other.d_property_name)
		{ }

		virtual
		boost::intrusive_ptr<PropertyContainer>
		clone() const = 0;

		void
		increment_ref_count() {
			++d_ref_count;
		}

		ref_count_type
		decrement_ref_count() {
			return --d_ref_count;
		}

		const PropertyName &
		property_name() const {
			return d_property_name;
		}

		// FIXME: visitor accept method

	private:

		ref_count_type d_ref_count;
		PropertyName d_property_name;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		PropertyContainer &
		operator=(
				const PropertyContainer &);

	};


	inline
	void
	intrusive_ptr_add_ref(
			PropertyContainer *p) {
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			PropertyContainer *p) {
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_PROPERTYCONTAINER_H