/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_FILEINFO_H
#define GPLATES_FILEIO_FILEINFO_H

#include <QString>
#include <QFileInfo>
#include <boost/optional.hpp>
#include "model/FeatureCollectionHandle.h"
#include "FileFormat.h"

namespace GPlatesFileIO {

	/**
	 * Holds information associated with a loaded file.
	 *
	 * FileInfo is used to keep track of information relating to a file that
	 * has been loaded into GPlates.  It keeps track of basic file information
	 * with a QFileInfo object, but it also keeps track of the file's data format, 
	 * it's line-ending style, and the FeatureCollectionHandle containing the
	 * features that were loaded from the file.
	 */
	class FileInfo 
	{
	public:
		// XXX: these belongs elsewhere
		enum LineEndingStyle
		{
			NL, CRNL, CR 
		};


		explicit
		FileInfo(
				QString file_name):
			d_file_info(file_name)
		{ }


		const QFileInfo &
		get_qfileinfo() const
		{
			return d_file_info;
		}


		boost::optional<GPlatesModel::FeatureCollectionHandle::weak_ref>
		get_feature_collection() 
		{ 
			return d_feature_collection; 
		}


		void
		set_feature_collection(
				GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection)
		{
			d_feature_collection = feature_collection;
		}


	private:
#if 0
		FileInfo(
				QFileInfo file_info,
				FileFormat file_format,
				LineEndingStyle line_ending,
				GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection) :
			d_file_info(file_info), d_file_format(file_format),
			d_line_ending(line_ending), d_feature_collection(feature_collection) { }
#endif

		QFileInfo d_file_info;
		//FileFormat d_file_format;
		//LineEndingStyle d_line_ending;
		boost::optional<GPlatesModel::FeatureCollectionHandle::weak_ref> d_feature_collection;
	};
}

#endif // GPLATES_FILEIO_FILEINFO_H
