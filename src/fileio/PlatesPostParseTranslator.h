/* $Id$ */

/**
 * @file 
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
 *   Hamish Ivey-Law <hlaw@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_FILEIO_PLATESPOSTPARSETRANSLATOR_
#define _GPLATES_FILEIO_PLATESPOSTPARSETRANSLATOR_

#include "geo/DataGroup.h"
#include "PlatesBoundaryParser.h"

namespace GPlatesFileIO
{
	/**
	 * Contains the function that translates from the data structure
	 * output by the PLATES parser into the internal GPlates data
	 * structure.
	 */
	namespace PlatesPostParseTranslator
	{
		GPlatesGeo::DataGroup*
		GetDataGroupFromPlatesDataMap(
			const PlatesParser::PlatesDataMap& map);
	}
}

#endif  /* _GPLATES_FILEIO_PLATESPOSTPARSETRANSLATOR_ */
