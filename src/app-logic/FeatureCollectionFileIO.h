/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_FEATURECOLLECTIONFILEIO_H
#define GPLATES_APP_LOGIC_FEATURECOLLECTIONFILEIO_H

#include <list>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <QObject>
#include <QString>
#include <QStringList>

#include "app-logic/FeatureCollectionFileState.h"

#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/File.h"
#include "file-io/FileInfo.h"

#include "model/ModelInterface.h"
#include "model/FeatureCollectionHandle.h"


namespace GPlatesFileIO
{
	class PropertyMapper;
	struct ReadErrorAccumulation;
}

namespace GPlatesAppLogic
{
	class FeatureCollectionFileIO :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT


	public:
		FeatureCollectionFileIO(
				GPlatesModel::ModelInterface &model,
				GPlatesAppLogic::FeatureCollectionFileState &file_state);


		/**
		 * Loads feature collections from multiple files named @a file_names and adds them
		 * to the application state.
		 *
		 * NOTE: if you are loading multiple files in one group then use this method
		 * instead of multiple calls to @a load_file so that the application state
		 * sends one notification instead of multiple notifications which is beneficial
		 * if some files in the group depend on each other.
		 */
		void
		load_files(
				const QStringList &file_names);


		/**
		 * Loads a feature collection from the file names @a file_name and adds it
		 * to the application state.
		 *
		 * NOTE: if you are loading multiple files in one group then use @a load_files
		 * instead so that the application state sends one notification instead of
		 * multiple notifications (for each @a load_file) which is beneficial
		 * if some files in the group depend on each other.
		 */
		void
		load_file(
				const QString &filename);


		/**
		 * Given a file_iterator, reloads the data for that file from disk,
		 * replacing the feature collection associated with that file_iterator in
		 * the application state.
		 */
		void
		reload_file(
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file);


		/**
		 * This method simply delegates to @a FeatureCollectionFileState and removes the file from it.
		 */
		void
		unload_file(
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file);


		/**
		 * Creates a fresh, empty, FeatureCollection. Associates a 'dummy'
		 * FileInfo for it, and registers it with FeatureCollectionFileState.
		 */
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator
		create_empty_file();


		/**
		 * Write the feature collection associated to a file described by @a file_info.
		 */
		static
		void
		save_file(
				const GPlatesFileIO::FileInfo &file_info,
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection,
				GPlatesFileIO::FeatureCollectionWriteFormat::Format =
					GPlatesFileIO::FeatureCollectionWriteFormat::USE_FILE_EXTENSION);


		/**
		 * Temporary method for initiating shapefile attribute remapping. 
		 */
		void
		remap_shapefile_attributes(
			GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_it);

	signals:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		handle_read_errors(
				GPlatesAppLogic::FeatureCollectionFileIO &,
				const GPlatesFileIO::ReadErrorAccumulation &read_errors);

		void
		remapped_shapefile_attributes(
				GPlatesAppLogic::FeatureCollectionFileIO &feature_collection_file_manager,
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file);

	private:
		//! Typedef for a sequence of file shared refs.
		typedef std::vector<GPlatesFileIO::File::shared_ref> file_seq_type;


		GPlatesModel::ModelInterface d_model;

		/**
		 * The loaded feature collection files.
		 */
		GPlatesAppLogic::FeatureCollectionFileState &d_file_state;


		file_seq_type
		read_feature_collections(
				const QStringList &filenames);


		const GPlatesFileIO::File::shared_ref
		read_feature_collection(
				const GPlatesFileIO::FileInfo &file_info);


		void
		emit_handle_read_errors_signal(
				const GPlatesFileIO::ReadErrorAccumulation &read_errors);
	};
}

#endif // GPLATES_APP_LOGIC_FEATURECOLLECTIONFILEIO_H