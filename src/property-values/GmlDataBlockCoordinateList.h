
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

#ifndef GPLATES_PROPERTYVALUES_GMLDATABLOCKCOORDINATELIST_H
#define GPLATES_PROPERTYVALUES_GMLDATABLOCKCOORDINATELIST_H

#include <map>
#include <vector>

#include "ValueObjectType.h"
#include "model/XmlAttributeName.h"
#include "model/XmlAttributeValue.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"


namespace GPlatesPropertyValues
{
	/**
	 * This associates a ValueObjectType instance with a sequence of "i-th" coordinates from a
	 * @c <gml:tupleList> property in a "gml:DataBlock".
	 *
	 * For info about "gml:DataBlock", see p.251-2 of Lake et al (2004).
	 *
	 * To understand what this class contains and how it fits into GmlDataBlock, consider that
	 * the @a <gml:tupleList> property stores a sequence of coordinate tuples: x1,y1 x2,y2
	 * x3,y3 x4,y4 ... (ie, a "record interleaved" encoding).
	 *
	 * Each coordinate xn in the tuple xn,yn is described by a ValueObject X in a
	 * @a <gml:valueComponent> property in a @a <gml:CompositeValue> element.
	 *
	 * Class GmlDataBlockCoordinateList effectively "de-interleaves" the records, storing the
	 * ValueObject X along with the coordinates it describes x1 x2 x3 x4 ...; a GmlDataBlock
	 * instance contains a sequence of GmlDataBlockCoordinateList instances (one instance for
	 * each coordinate in the coordinate tuple).
	 *
	 * When the GmlDataBlock is output in GPML, it will be necessary to "re-interleave" the
	 * records.
	 */
	class GmlDataBlockCoordinateList:
			public GPlatesUtils::ReferenceCount<GmlDataBlockCoordinateList>
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<GmlDataBlockCoordinateList,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<GmlDataBlockCoordinateList,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

		/**
		 * The type which contains XML attribute names and values.
		 */
		typedef std::map<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue>
				xml_attributes_type;

		/**
		 * The type of the sequence of coordinates.
		 */
		typedef std::vector<double> coordinate_list_type;

		~GmlDataBlockCoordinateList()
		{  }

		static
		const non_null_ptr_type
		create(
				const ValueObjectType &value_object_type_,
				const xml_attributes_type &value_object_xml_attributes_,
				coordinate_list_type::size_type list_len)
		{
			non_null_ptr_type ptr(
					new GmlDataBlockCoordinateList(
							value_object_type_,
							value_object_xml_attributes_,
							list_len));
			return ptr;
		}

		const non_null_ptr_type
		clone() const
		{
			non_null_ptr_type dup(new GmlDataBlockCoordinateList(this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}

		const ValueObjectType &
		value_object_type() const
		{
			return d_value_object_type;
		}

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map?  (For consistency with the non-const
		// overload...)
		const xml_attributes_type &
		value_object_xml_attributes() const
		{
			return d_value_object_xml_attributes;
		}

		// @b FIXME:  Should this function be replaced with per-index const-access to
		// elements of the XML attribute map, as well as per-index assignment (setter) and
		// removal operations?  This would ensure that revisioning is correctly handled...
		xml_attributes_type &
		value_object_xml_attributes()
		{
			return d_value_object_xml_attributes;
		}

		coordinate_list_type::const_iterator
		coordinates_begin() const
		{
			return d_coordinates.begin();
		}

		coordinate_list_type::const_iterator
		coordinates_end() const
		{
			return d_coordinates.end();
		}

		coordinate_list_type::iterator
		coordinates_begin()
		{
			return d_coordinates.begin();
		}

		coordinate_list_type::iterator
		coordinates_end()
		{
			return d_coordinates.end();
		}

		void
		coordinates_push_back(
				const double &coord)
		{
			d_coordinates.push_back(coord);
		}

	protected:

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		GmlDataBlockCoordinateList(
				const ValueObjectType &value_object_type_,
				const xml_attributes_type &value_object_xml_attributes_,
				coordinate_list_type::size_type list_len):
			d_value_object_type(value_object_type_),
			d_value_object_xml_attributes(value_object_xml_attributes_),
		{
			// Avoid re-allocating the vector buffer as we append coordinates.
			d_coordinates.reserve(list_len);
		}

		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		//
		// Note that this should act exactly the same as the default (auto-generated)
		// copy-constructor, except that it should initialise the ref-count to zero.
		GmlDataBlockCoordinateList(
				const GmlDataBlockCoordinateList &other):
			GPlatesUtils::ReferenceCount<GmlDataBlockCoordinateList>(),
			d_value_object_type(other.d_value_object_type),
			d_value_object_xml_attributes(other.d_value_object_xml_attributes),
			d_coordinates(other.d_coordinates)
		{  }

	private:

		ValueObjectType d_value_object_type;

		xml_attributes_type d_value_object_xml_attributes;

		coordinate_list_type d_coordinates;

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive_ptr to another.
		GmlDataBlockCoordinateList &
		operator=(const GmlDataBlockCoordinateList &);

	};

}

#endif  // GPLATES_PROPERTYVALUES_GMLDATABLOCKCOORDINATELIST_H
