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

#ifndef GPLATES_MODEL_GPMLPLATEID_H
#define GPLATES_MODEL_GPMLPLATEID_H

#include "PropertyValue.h"


namespace GPlatesModel {

	class GpmlPlateId :
			public PropertyValue {

	public:

		virtual
		~GpmlPlateId() {  }

		// This creation function is here purely for the simple, hard-coded construction of
		// features.  It may not be necessary or appropriate later on when we're doing
		// everything properly, so don't look at this function and think "Uh oh, this
		// function doesn't look like it should be here, but I'm sure it's here for a
		// reason..."
		static
		boost::intrusive_ptr<GpmlPlateId>
		create(
				const unsigned long &value_) {
			boost::intrusive_ptr<GpmlPlateId> ptr(new GpmlPlateId(value_));
			return ptr;
		}

		virtual
		boost::intrusive_ptr<PropertyValue>
		clone() const {
			boost::intrusive_ptr<PropertyValue> dup(new GpmlPlateId(*this));
			return dup;
		}

		const unsigned long &
		value() const {
			return d_value;
		}

		unsigned long &
		value() {
			return d_value;
		}

		virtual
		void
		accept_visitor(
				ConstFeatureVisitor &visitor) const {
			visitor.visit_gpml_plate_id(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		GpmlPlateId(
				const unsigned long &value_):
			PropertyValue(),
			d_value(value_)
		{  }

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GpmlPlateId(
				const GpmlPlateId &other) :
			PropertyValue(),
			d_value(other.d_value)
		{  }

	private:

		unsigned long d_value;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GpmlPlateId &
		operator=(const GpmlPlateId &);

	};

}

#endif  // GPLATES_MODEL_GPMLPLATEID_H
