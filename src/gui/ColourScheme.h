/* $Id$ */

/**
 * @file 
 * Contains the definition of the ColourScheme class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_COLOURSCHEME_H
#define GPLATES_GUI_COLOURSCHEME_H

#include <boost/optional.hpp>

#include "Colour.h"

namespace GPlatesModel
{
	class ReconstructionGeometry;
}

namespace GPlatesGui
{
	/**
	 * This class assigns colours to ReconstructionGeometry instances.
	 */
	class ColourScheme
	{
	public:

		//! Destructor
		virtual
		~ColourScheme()
		{  }

		/**
		 * Returns a colour for a particular @a reconstruction_geometry, or
		 * boost::none if it does not have the necessary parameters or if the
		 * reconstruction geometry should not be drawn for some other reason
		 */
		virtual
		boost::optional<Colour>
		get_colour(
				const GPlatesModel::ReconstructionGeometry &reconstruction_geometry) const = 0;

	};
}

#endif  /* GPLATES_GUI_COLOURSCHEME_H */
