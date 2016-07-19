/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTATION_REMAPPEDCOLOURPALETTEPARAMETERS_H
#define GPLATES_PRESENTATION_REMAPPEDCOLOURPALETTEPARAMETERS_H

#include <utility>
#include <boost/optional.hpp>
#include <QString>

#include "file-io/ReadErrorAccumulation.h"

#include "gui/RasterColourPalette.h"

// Try to only include the heavyweight "Scribe.h" in '.cc' files where possible.
#include "scribe/Transcribe.h"


namespace GPlatesPresentation
{
	/**
	 * Manages a real-valued colour palette whose input range can be remapped.
	 *
	 * This class is mainly to support RemappedColourPaletteWidget and a place to keep
	 * visual layer parameters for any layers that use RemappedColourPaletteWidget.
	 */
	class RemappedColourPaletteParameters
	{
	public:

		static const double DEFAULT_DEVIATION_FROM_MEAN;

		/**
		 * Some pre-defined internal palette types are provided for convenience.
		 */
		enum ConvenientPaletteType
		{
			AGE_PALETTE

			// NOTE: Any new values should also be added to @a transcribe.
		};


		/**
		 * Constructor uses the specified default colour palette and deviation-from-mean parameter.
		 */
		explicit
		RemappedColourPaletteParameters(
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &default_colour_palette,
				const double &default_deviation_from_mean = DEFAULT_DEVIATION_FROM_MEAN);


		/**
		 * Returns the colour palette - this is the mapped palette if mapping is currently used.
		 */
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type
		get_colour_palette() const;

		/**
		 * Returns the palette range - this is the mapped range if mapping is currently used.
		 */
		const std::pair<double, double> &
		get_palette_range() const;

		/**
		 * Returns the filename of the CPT file from which the current colour palette was loaded,
		 * if it was loaded from a file (or an internal file resource if a @a ConvenientPaletteType,
		 * eg, ":/age.cpt").
		 *
		 * If the current colour palette is the auto-generated default palette then returns the empty string.
		 */
		QString
		get_colour_palette_filename() const
		{
			return d_colour_palette_filename;
		}

		/**
		 * Returns the convenient palette type (if current palette was loaded via @a load_convenient_colour_palette).
		 *
		 * Returns none if current palette is default colour palette or was loaded from a file.
		 */
		boost::optional<ConvenientPaletteType>
		get_convenient_palette_type() const;

		/**
		 * Causes the current colour palette to be the auto-generated default palette,
		 * and sets the filename field to be the empty string.
		 *
		 * If the previous palette is mapped then the new (default) palette will be mapped to the same range.
		 */
		void
		use_default_colour_palette();

		/**
		 * Sets the current colour palette to be one that has been loaded from a file.
		 * @a filename must not be the empty string.
		 *
		 * If the previous palette is mapped then the new palette will be mapped to the same range.
		 *
		 * Returns false if a mapping is applied but failed, in which case palette range is unmapped.
		 * See @a map_palette_range for more details.
		 */
		bool
		set_colour_palette(
				const QString &filename,
				const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette,
				const std::pair<double, double> &palette_range);

		/**
		 * Same as @a set_colour_palette but also loads the colour palette from the file @a filename.
		 *
		 * If the previous palette is mapped then the new palette will be mapped to the same range.
		 *
		 * Only allow loading an integer colour palette if the raster type is integer-valued and
		 * the colour palette is not being remapped (see @a is_palette_range_mapped).
		 *
		 * Returns false if failed to load colour palette file, or if a mapping is applied but failed
		 * in which case palette range is unmapped (see @a map_palette_range for more details).
		 */
		bool
		load_colour_palette(
				const QString &filename,
				GPlatesFileIO::ReadErrorAccumulation &read_errors,
				bool allow_integer_colour_palette = false);

		/**
		 * Same as @a load_colour_palette except loads an convenient internal palette type.
		 */
		bool
		load_convenient_colour_palette(
				ConvenientPaletteType convenient_palette_type,
				GPlatesFileIO::ReadErrorAccumulation &read_errors,
				bool allow_integer_colour_palette = false);

		/**
		 * Remaps the value range of the colour palette (the palette colours remain unchanged).
		 *
		 * Returns false if mapping failed, in which case palette range is unmapped.
		 * An integer (categorical) colour palette is not mappable.
		 */
		bool
		map_palette_range(
				const double &lower_bound,
				const double &upper_bound);

		/**
		 * Unmaps the current colour palette.
		 *
		 * The palette range will revert to the original range loaded from the palette file, or default palette.
		 */
		void
		unmap_palette_range()
		{
			d_is_currently_mapped = false;
		}

		/**
		 * Returns true if the palette range is currently mapped.
		 */
		bool
		is_palette_range_mapped() const
		{
			return d_is_currently_mapped;
		}

		/**
		 * Returns the currently mapped palette range (or most recently mapped if not currently mapped).
		 *
		 * NOTE: This is most recently mapped range if the palette range is not currently mapped.
		 * This is useful for restoring a previous mapping.
		 */
		const std::pair<double, double> &
		get_mapped_palette_range() const
		{
			return d_mapped_colour_palette_info.palette_range;
		}

		/**
		 * Sets the deviation-from-mean parameter (number of standard deviations).
		 *
		 * See RemappedColourPaletteWidget.
		 *
		 * Only used to keep track of the deviation parameter for when it's used to
		 * generate a mapped palette range.
		 *
		 * For colour-by-scalar this range is [mean - deviation, mean + deviation].
		 * For colour-by-gradient this range is [-mean - deviation, mean + deviation].
		 */
		void
		set_deviation_from_mean(
				const double &deviation_from_mean)
		{
			d_deviation_from_mean = deviation_from_mean;
		}

		/**
		 * Returns the deviation-from-mean parameter (number of standard deviations).
		 *
		 * See RemappedColourPaletteWidget.
		 */
		const double &
		get_deviation_from_mean() const
		{
			return d_deviation_from_mean;
		}

	private:

		struct ColourPaletteInfo
		{
			ColourPaletteInfo(
					const GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type &colour_palette_,
					const std::pair<double, double> &palette_range_) :
				colour_palette(colour_palette_),
				palette_range(palette_range_)
			{  }

			GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type colour_palette;
			std::pair<double, double> palette_range;
		};


		ColourPaletteInfo d_default_colour_palette_info;

		/**
		 * The filename the colour palette was loaded from.
		 *
		 * Is an empty string if the default palette is being used.
		 * Is a string beginning with ":/" (ie, internal filename) if a @a ConvenientPaletteType.
		 */
		QString d_colour_palette_filename;

		double d_deviation_from_mean;

		/**
		 * The unmapped palette loaded from the CPT file (or the default palette).
		 */
		ColourPaletteInfo d_unmapped_colour_palette_info;

		/**
		 * The mapped palette (a mapped version of @a d_unmapped_colour_palette_info).
		 *
		 * Is the same as the unmapped palette if not currently mapped.
		 */
		ColourPaletteInfo d_mapped_colour_palette_info;

		bool d_is_currently_mapped;

	};


	/**
	 * Transcribe for sessions/projects.
	 */
	GPlatesScribe::TranscribeResult
	transcribe(
			GPlatesScribe::Scribe &scribe,
			RemappedColourPaletteParameters::ConvenientPaletteType &convenient_palette_type,
			bool transcribed_construct_data);
}

#endif // GPLATES_PRESENTATION_REMAPPEDCOLOURPALETTEPARAMETERS_H