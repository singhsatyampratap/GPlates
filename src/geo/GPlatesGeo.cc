/* $Id$ */

/**
 * \file 
 * This file contains the definitions of functions that have been
 * declared in the GPlatesGeo namespace.  These will primarily consist
 * of operators.
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
 */

#include "GeneralisedData.h"

/*
 * Defined in GeneralisedData.h.
 */
inline std::ostream&
GPlatesGeo::operator<<(std::ostream& os, const GeneralisedData& data)
{
	data.PrintOut(os); 
	return os;
}

/*
 * Defined in GeneralisedData.h.
 */
inline std::istream&
GPlatesGeo::operator>>(std::istream& is, GeneralisedData& data)
{
	data.ReadIn(is);
	return is;
}
