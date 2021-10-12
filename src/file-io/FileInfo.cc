/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2007, 2008, 2009 The University of Sydney, Australia
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

#include "FileInfo.h"


const QString
GPlatesFileIO::FileInfo::get_display_name(
		bool use_absolute_path_name) const
{
	if (use_absolute_path_name) {
		return d_file_info.absoluteFilePath();
	} else {
		return d_file_info.fileName();
	}
}


GPlatesFileIO::FileInfo
GPlatesFileIO::create_copy_with_new_filename(
		const QString &filename,
		const FileInfo &other_file_info)
{
	FileInfo file_info(filename);

	file_info.set_model_to_shapefile_map(
			other_file_info.get_model_to_shapefile_map());

	return file_info;
}


bool
GPlatesFileIO::is_writable(
		const QFileInfo &file_info)
{
	// NOTE: This is known to not work correctly on Windows Vista and above when
	// saving to the Desktop and certain other directories (in particular, those
	// that the user can write to, but does not explicitly own). The test on
	// Windows should probably be on the fileName() not the path(). However, if
	// you are writing to a file, just try and open the file for writing, and if
	// it fails, then throw an exception (instead of explicitly checking whether
	// the file can be written to).
	
	QFileInfo dir(file_info.path());
	return dir.permission(QFile::WriteUser);
}
