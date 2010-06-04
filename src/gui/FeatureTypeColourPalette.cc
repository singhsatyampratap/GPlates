/* $Id$ */

/**
 * @file 
 * Contains the implementation of the FeatureTypeColourPalette class.
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

#include <string>
#include <boost/foreach.hpp>

#include "FeatureTypeColourPalette.h"
#include "HTMLColourNames.h"

#include "file-io/FeaturePropertiesMap.h"

#include "utils/UnicodeStringUtils.h"


namespace
{
	using namespace GPlatesGui;


	Colour
	map_to_colour(
			unsigned int number)
	{
		static const Colour COLOURS[] = {
			*(HTMLColourNames::instance().get_colour("saddlebrown")),
			Colour::get_yellow(),
			Colour::get_red(),
			Colour::get_blue(),
			Colour::get_green(),
			Colour::get_purple(),
			*(HTMLColourNames::instance().get_colour("orange")),
			*(HTMLColourNames::instance().get_colour("lightskyblue")),
			Colour::get_lime(),
			*(HTMLColourNames::instance().get_colour("lightsalmon"))
		};
		static const size_t NUM_COLOURS = sizeof(COLOURS) / sizeof(Colour);

		return COLOURS[number % NUM_COLOURS];
	}


	unsigned int
	hash(
			const GPlatesModel::FeatureType &feature_type)
	{
		// First convert to std::string.
		std::string str = GPlatesUtils::make_std_string_from_icu_string(
				feature_type.build_aliased_name());

		// Then xor all the individual chars together.
		unsigned int result = 0;
		BOOST_FOREACH(char c, str)
		{
			result ^= c;
		}

		return result;
	}


	/**
	 * Assign a colour to an unknown FeatureType.
	 */
	Colour
	create_colour(
			const GPlatesModel::FeatureType &feature_type)
	{
		return map_to_colour(hash(feature_type));
	}
}


GPlatesGui::FeatureTypeColourPalette::non_null_ptr_type
GPlatesGui::FeatureTypeColourPalette::create()
{
	return new FeatureTypeColourPalette();
}


GPlatesGui::FeatureTypeColourPalette::FeatureTypeColourPalette()
{
	// Populate the colours map with FeatureTypes that we know about.
	typedef GPlatesFileIO::FeaturePropertiesMap::const_iterator iterator_type;
	GPlatesFileIO::FeaturePropertiesMap &feature_properties_map =
		GPlatesFileIO::FeaturePropertiesMap::instance();
	unsigned int count = 0;
	for (iterator_type iter = feature_properties_map.begin();
			iter != feature_properties_map.end(); ++iter)
	{
		d_colours.insert(
				std::make_pair(
					iter->first,
					map_to_colour(count)));
		++count;
	}
}


boost::optional<GPlatesGui::Colour>
GPlatesGui::FeatureTypeColourPalette::get_colour(
		const GPlatesModel::FeatureType &feature_type) const
{
	std::map<GPlatesModel::FeatureType, Colour>::const_iterator colour =
		d_colours.find(feature_type);
	if (colour == d_colours.end())
	{
		Colour generated_colour = create_colour(feature_type);
		d_colours.insert(
				std::make_pair(
					feature_type,
					generated_colour));
		return generated_colour;
	}
	else
	{
		return colour->second;
	}
}
