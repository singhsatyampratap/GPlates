/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_PLATESCOLOURTABLE_H
#define GPLATES_GUI_PLATESCOLOURTABLE_H

#include <boost/scoped_ptr.hpp>
#include <vector>

#include "ColourTable.h"
#include "model/types.h"


namespace GPlatesGui 
{
	class PlatesColourTable:
			public ColourTable
	{
		public:
			static
			PlatesColourTable *
			Instance();

			virtual
			~PlatesColourTable()
			{  }

			ColourTable::const_iterator
			end() const
			{
				return NULL;
			}

			virtual
			ColourTable::const_iterator
			lookup(
					const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const;

			ColourTable::const_iterator
			lookup_by_plate_id(
					GPlatesModel::integer_plate_id_type id) const;

		protected:
			/**
			 * Private constructor to enforce singleton design.
			 */
			PlatesColourTable();

		private:
			/**
			 * The singleton instance.
			 */
			static boost::scoped_ptr<PlatesColourTable> d_instance;

			/**
			 * Typedef for sequence of @a Colour objects.
			 */
			typedef std::vector<Colour> colour_seq_type;

			/**
			 * Typedef for sequence of @a Colour pointers.
			 */
			typedef std::vector<const Colour *> colour_ptr_seq_type;

			/**
			 * A mapping of Plate ID (aka "rid_t", for "rotation
			 * ID type") to a Colour object representing the colour
			 * for that ID, or NULL if there is no colour defined
			 * for that ID.
			 *
			 * Implemented as an array of pointers-to-Colour,
			 * intended to be indexed by an ID.  Each
			 * pointer-to-Colour should point to a Colour object in
			 * @a _colours, or be NULL when there is no colour
			 * defined for that ID.
			 *
			 * Since @a _highest_known_rid contains the highest
			 * known ID, the length of this array will be @a
			 * _highest_known_rid + 1 (since, for example, an index
			 * of 3 requires an array of at least 3 + 1 == 4).
			 */
			colour_ptr_seq_type d_id_table;

			/**
			 * The highest Plate ID (aka "rid_t", for "rotation
			 * ID type") entered in the ID table.
			 */
			GPlatesModel::integer_plate_id_type d_highest_known_rid;

			/**
			 * An array of Colour objects.  Each of these Colour
			 * objects represents the defined colour corresponding
			 * to some Plate ID (aka "rid_t", for "rotation ID
			 * type"), but this array doesn't worry about this
			 * ID -> Colour mapping (that's what @a _id_table is
			 * for); it just holds the Colour objects.
			 */
			colour_seq_type d_colours;

			struct MappingPair
			{
				GPlatesModel::integer_plate_id_type id;
				Colour colour;
			};

			void
			populate(
					const MappingPair array[],
					size_t array_len);

			static
			GPlatesModel::integer_plate_id_type
			getHighestID(
					const MappingPair array[],
					size_t array_len);
	};
}

#endif  /* GPLATES_GUI_PLATESCOLOURTABLE_H */
