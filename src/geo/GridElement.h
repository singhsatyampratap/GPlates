/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   Hamish Law <hlaw@es.usyd.edu.au>
 *   Dave Symonds <ds@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_GEO_GRIDELEMENT_H_
#define _GPLATES_GEO_GRIDELEMENT_H_

#include "GridData.h"

namespace GPlatesGeo
{
	/**
	 * An element of gridded data.
	 */
	class GridElement
	{
		public:
			GridElement(const Attributes_t& attrs) 
						: _attributes(attrs) { }

		private:
			Attributes_t _attributes;
	};
}

#endif  // _GPLATES_GEO_GRIDELEMENT_H_
