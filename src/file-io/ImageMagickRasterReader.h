/* $Id$ */

/**
 * \file 
 * File specific comments.
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

#ifndef GPLATES_FILEIO_IMAGEMAGICKRASTERREADER_H
#define GPLATES_FILEIO_IMAGEMAGICKRASTERREADER_H

#include <string>
#include <utility>
#include <boost/function.hpp>
#include <boost/optional.hpp>
#include <QSize>

#include "global/CompilerWarnings.h"

PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING( 4251 )
#include <Magick++.h>
POP_MSVC_WARNINGS

#include "RasterReader.h"
#include "RasterBandReaderHandle.h"


namespace GPlatesFileIO
{
	/**
	 * Reads rasters using ImageMagick.
	 */
	class ImageMagickRasterReader :
			public RasterReaderImpl
	{
	public:

		ImageMagickRasterReader(
				const QString &filename,
				const boost::function<RasterBandReaderHandle (unsigned int)> &proxy_handle_function,
				ReadErrorAccumulation *read_errors);

		virtual
		bool
		can_read();

		virtual
		unsigned int
		get_number_of_bands(
				ReadErrorAccumulation *read_errors);

		virtual
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
		get_proxied_raw_raster(
				unsigned int band_number,
				ReadErrorAccumulation *read_errors);

		virtual
		boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type>
		get_raw_raster(
				unsigned int band_number,
				const QRect &region,
				ReadErrorAccumulation *read_errors);

		virtual
		GPlatesPropertyValues::RasterType::Type
		get_type(
				unsigned int band_number,
				ReadErrorAccumulation *read_errors);

		virtual
		void *
		get_data(
				unsigned int band_number,
				const QRect &region,
				ReadErrorAccumulation *read_errors);

	private:

		boost::optional<std::pair<Magick::Image, QSize> >
		get_region_as_image(
				const QRect &region,
				ReadErrorAccumulation *read_errors);

		void
		report_recoverable_error(
				ReadErrorAccumulation *read_errors,
				ReadErrors::Description description);

		void
		report_failure_to_begin(
				ReadErrorAccumulation *read_errors,
				ReadErrors::Description description);

		QString d_filename;
		std::string d_filename_as_std_string;
		boost::function<RasterBandReaderHandle (unsigned int)> d_proxy_handle_function;
		bool d_can_read;
	};
}

#endif  // GPLATES_FILEIO_QIMAGERASTERREADER_H