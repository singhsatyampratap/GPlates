
/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_PROPERTYVALUES_GMLDATABLOCK_H
#define GPLATES_PROPERTYVALUES_GMLDATABLOCK_H

#include <vector>

#include "GmlDataBlockCoordinateList.h"
#include "model/PropertyValue.h"


namespace GPlatesPropertyValues
{
	/**
	 * This class implements the PropertyValue which corresponds to "gml:DataBlock".
	 */
	class GmlDataBlock:
			public GPlatesModel::PropertyValue
	{
	public:

		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<GmlDataBlock,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlDataBlock,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const GmlDataBlock,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const GmlDataBlock,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;

		/**
		 * The type of the sequence of coordinates.
		 */
		typedef GmlDataBlockCoordinateList::coordinate_list_type coordinate_list_type;

		/**
		 * The type of the sequence of GmlDataBlockCoordinateList instances.
		 */
		typedef std::vector<GmlDataBlockCoordinateList::non_null_ptr_type> tuple_list_type;

		virtual
		~GmlDataBlock()
		{  }

		static
		const non_null_ptr_type
		create(
				std::list< std::pair< ValueObjectType,
						GmlDataBlockCoordinateList::xml_attributes_type > >
								&range_parameters,
				coordinate_list_type::size_type tuple_list_len)
		{
			non_null_ptr_type ptr(new GmlDataBlock(range_parameters, tuple_list_len),
					GPlatesUtils::NullIntrusivePointerHandler());
			return ptr;
		}

		virtual
		const GPlatesModel::PropertyValue::non_null_ptr_type
		clone() const
		{
			GPlatesModel::PropertyValue::non_null_ptr_type dup(new GmlDataBlock(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
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
			visitor.visit_gml_data_block(*this);
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
			visitor.visit_gml_data_block(*this);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlDataBlock(
				std::list< std::pair< ValueObjectType,
						GmlDataBlockCoordinateList::xml_attributes_type > >
								&range_parameters,
				coordinate_list_type::size_type tuple_list_len):
		{
			// FIXME:  Set up the tuple-list in here from the range-parameters and the
			// tuple-list len.  The function body should probably move into a .cc file.
		}

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except it should not be public.
		GmlDataBlock(
				const GmlDataBlock &other):
			// FIXME:  Since the vector of doubles in GmlDataBlockCoordinateList is
			// currently non-const, its contents can be modified, which means we don't
			// want these GmlDataBlock instances to be sharing tuple-lists, which means
			// we need to perform a deep copy.  I need to resolve this situation
			// somehow.
			d_tuple_list(other.d_tuple_list)
		{  }

	private:

		tuple_list_type d_tuple_list;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlDataBlock &
		operator=(const GmlDataBlock &);

	};


}

#endif  // GPLATES_PROPERTYVALUES_GMLDATABLOCK_H
