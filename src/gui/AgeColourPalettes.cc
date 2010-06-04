/* $Id$ */

/**
 * @file 
 * Contains the implementation of the DefaultAgeColourPalette class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include "AgeColourPalettes.h"
#include "ColourSpectrum.h"

#include "maths/Real.h"


const double
GPlatesGui::DefaultAgeColourPalette::DEFAULT_UPPER_BOUND = 450.0; // Ma


const double
GPlatesGui::DefaultAgeColourPalette::DEFAULT_LOWER_BOUND = 0.0;


GPlatesGui::DefaultAgeColourPalette::non_null_ptr_type
GPlatesGui::DefaultAgeColourPalette::create()
{
	return new DefaultAgeColourPalette();
}


GPlatesGui::DefaultAgeColourPalette::DefaultAgeColourPalette() :
	AgeColourPalette(DEFAULT_UPPER_BOUND, DEFAULT_LOWER_BOUND)
{
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::DefaultAgeColourPalette::get_colour(
		const GPlatesMaths::Real &age) const
{
	double dval;
	if (age.is_positive_infinity())
	{
		// Distant past.
		dval = d_upper_bound;
	}
	else if (age.is_negative_infinity())
	{
		// Distant future.
		dval = d_lower_bound;
	}
	else
	{
		dval = age.dval();
	}

	double position = (dval - d_lower_bound) / (d_upper_bound - d_lower_bound);
	return ColourSpectrum::get_colour_at(position);
}


const double
GPlatesGui::MonochromeAgeColourPalette::DEFAULT_UPPER_BOUND = 450.0; // Ma


const double
GPlatesGui::MonochromeAgeColourPalette::DEFAULT_LOWER_BOUND = 0.0;


const GPlatesGui::Colour
GPlatesGui::MonochromeAgeColourPalette::UPPER_COLOUR = Colour::get_black();


const GPlatesGui::Colour
GPlatesGui::MonochromeAgeColourPalette::LOWER_COLOUR = Colour::get_white();


GPlatesGui::MonochromeAgeColourPalette::MonochromeAgeColourPalette() :
	AgeColourPalette(DEFAULT_UPPER_BOUND, DEFAULT_LOWER_BOUND)
{
}


GPlatesGui::MonochromeAgeColourPalette::non_null_ptr_type
GPlatesGui::MonochromeAgeColourPalette::create()
{
	return new MonochromeAgeColourPalette();
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::MonochromeAgeColourPalette::get_colour(
		const GPlatesMaths::Real &age) const
{
	if (age >= d_upper_bound)
	{
		return UPPER_COLOUR;
	}
	else if (age <= d_lower_bound)
	{
		return LOWER_COLOUR;
	}
	else
	{
		double position = (age.dval() - d_lower_bound) / (d_upper_bound - d_lower_bound);
		return Colour::linearly_interpolate(
				LOWER_COLOUR,
				UPPER_COLOUR,
				position);
	}
}


