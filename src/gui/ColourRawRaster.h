/* $Id$ */

/**
 * @file 
 * Contains classes and functions related to applying colour palettes to rasters.
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

#ifndef GPLATES_GUI_COLOURRAWRASTER_H
#define GPLATES_GUI_COLOURRAWRASTER_H

#include <algorithm>
#include <boost/optional.hpp>

#include "ColourPalette.h"
#include "ColourPaletteAdapter.h"

#include "maths/Real.h"

#include "property-values/RawRaster.h"

#include "utils/TypeTraits.h"

namespace GPlatesGui
{
	namespace ColourRawRaster
	{
		namespace ColourRawRasterInternals
		{
			const rgba8_t TRANSPARENT_COLOUR(0, 0, 0, 0);

			/**
			 * This helper functor wraps around a ColourPalette. The function operator
			 * takes a value as argument, checks whether it is the no-data value,
			 * and if it isn't, converts the value to a Colour using the ColourPalette
			 * and then returns the rgba8_t equivalent colour.
			 */
			template<class RawRasterType>
			class ColourPaletteFunctor
			{
			public:

				typedef typename RawRasterType::element_type raster_element_type;

				ColourPaletteFunctor(
						const RawRasterType &raster,
						const ColourPalette<raster_element_type> &colour_palette) :
					d_raster(raster),
					d_colour_palette(colour_palette)
				{  }

				rgba8_t
				operator()(
						raster_element_type value) const
				{
					// Check whether it is the no-data value.
					if (d_raster.is_no_data_value(value))
					{
						return TRANSPARENT_COLOUR;
					}

					boost::optional<Colour> result = d_colour_palette.get_colour(value);
					if (!result)
					{
						return TRANSPARENT_COLOUR;
					}

					return Colour::to_rgba8(*result);
				}

			private:

				const RawRasterType &d_raster;
				const ColourPalette<raster_element_type> &d_colour_palette;
			};
		}

		/**
		 * Colours a RawRaster of type RawRasterType with the given @a colour_palette,
		 * which must be of the correct type for the raster that you want to colour.
		 */
		template<class RawRasterType>
		GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type
		colour_raw_raster(
				RawRasterType &source,
				typename ColourPalette<typename RawRasterType::element_type>::non_null_ptr_type colour_palette)
		{
			// Creates an uninitialised RGBA8 raster with the same width and height (in
			// pixels) as the original raster.
			const unsigned int width = source.width();
			const unsigned int height = source.height();
			GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type rgba_raster =
				GPlatesPropertyValues::Rgba8RawRaster::create(width, height);

			// Get pointers to the source and destination buffers.
			// It's important to hold a shared_array while operating on these buffers
			// in case they get swept out from underneath us.
			typedef typename RawRasterType::element_type source_element_type;
			boost::shared_array<source_element_type> source_array = source.data();
			source_element_type *source_buf = source_array.get();
			const unsigned int source_size = width * height;

			boost::shared_array<rgba8_t> dest_array = rgba_raster->data();
			rgba8_t *dest_buf = dest_array.get();

			std::transform(source_buf, source_buf + source_size, dest_buf,
					ColourRawRasterInternals::ColourPaletteFunctor<RawRasterType>(
						source, *colour_palette));

			return rgba_raster;
		}

		template<>
		GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type
		colour_raw_raster<GPlatesPropertyValues::Rgba8RawRaster>(
				GPlatesPropertyValues::Rgba8RawRaster &source,
				ColourPalette<rgba8_t>::non_null_ptr_type palette);
			// This is intentionally not defined anywhere, because you can't colour a
			// raster already in RGBA format.


		namespace ColourRawRasterInternals
		{
			using namespace GPlatesPropertyValues;

			/**
			 * A visitor that attempts to colour a raster of unknown type.
			 *
			 * The template parameter T is the type of the colour palette.
			 */
			template<typename T>
			class ColourRawRasterVisitor :
					public GPlatesPropertyValues::RawRasterVisitor
			{
			public:

				using GPlatesPropertyValues::RawRasterVisitor::visit;

				ColourRawRasterVisitor(
						typename ColourPalette<T>::non_null_ptr_type colour_palette) :
					d_colour_palette(colour_palette)
				{  }

				void
				visit(
						Int8RawRaster &raster)
				{
					do_visit(raster);
				}

				void
				visit(
						UInt8RawRaster &raster)
				{
					do_visit(raster);
				}

				void
				visit(
						Int16RawRaster &raster)
				{
					do_visit(raster);
				}

				void
				visit(
						UInt16RawRaster &raster)
				{
					do_visit(raster);
				}

				void
				visit(
						Int32RawRaster &raster)
				{
					do_visit(raster);
				}

				void
				visit(
						UInt32RawRaster &raster)
				{
					do_visit(raster);
				}

				void
				visit(
						FloatRawRaster &raster)
				{
					// We can only colour real-valued rasters with real-valued palettes.
					if (GPlatesUtils::TypeTraits<T>::is_floating_point)
					{
						do_visit(raster);
					}
				}

				void
				visit(
						DoubleRawRaster &raster)
				{
					// We can only colour real-valued rasters with real-valued palettes.
					if (GPlatesUtils::TypeTraits<T>::is_floating_point)
					{
						do_visit(raster);
					}
				}

				boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type>
				coloured_raster() const
				{
					return d_coloured_raster;
				}

			private:

				template<class RawRasterType>
				void
				do_visit(
						RawRasterType &source)
				{
					d_coloured_raster = colour_raw_raster(source);
				}

				template<class RawRasterType>
				GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type
				colour_raw_raster(
						RawRasterType &source)
				{
					// Convert the colour palette to take the right type.
					typedef typename RawRasterType::element_type element_type;
					typename ColourPalette<element_type>::non_null_ptr_type colour_palette =
						GPlatesGui::convert_colour_palette<T, element_type>(d_colour_palette);

					return GPlatesGui::ColourRawRaster::colour_raw_raster(source, colour_palette);
				}

				typename ColourPalette<T>::non_null_ptr_type d_colour_palette;
				boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type> d_coloured_raster;
			};
		}


		/**
		 * Colours a RawRaster, without knowing which specific derivation it is.
		 *
		 * The template parameter is the type of the colour palette.
		 *
		 * If the colour palette is integer-valued, it will only colour rasters that
		 * are themselves integer-valued.
		 *
		 * If the colour palette is real-valued, it will colour rasters that are either
		 * integer or real-valued, but of course not RGBA rasters.
		 */
		template<typename T>
		boost::optional<GPlatesPropertyValues::Rgba8RawRaster::non_null_ptr_type>
		colour_raw_raster(
				GPlatesPropertyValues::RawRaster &source,
				typename ColourPalette<T>::non_null_ptr_type colour_palette)
		{
			ColourRawRasterInternals::ColourRawRasterVisitor<T> visitor(colour_palette);
			source.accept_visitor(visitor);

			return visitor.coloured_raster();
		}
	}
}

#endif  /* GPLATES_GUI_COLOURRAWRASTER_H */