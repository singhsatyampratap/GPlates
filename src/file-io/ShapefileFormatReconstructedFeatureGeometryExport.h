/* $Id$ */

/**
 * \file Exports reconstructed feature geometries to a GMT format file.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_SHAPEFILEFORMATRECONSTRUCTEDFEATUREGEOMETRYEXPORT_H
#define GPLATES_FILEIO_SHAPEFILEFORMATRECONSTRUCTEDFEATUREGEOMETRYEXPORT_H

#include <QFileInfo>

#include "ReconstructedFeatureGeometryExport.h"

#include "model/types.h"


namespace GPlatesFileIO
{
	namespace ShapefileFormatReconstructedFeatureGeometryExport
	{
		/**
		 * Typedef for a sequence of @a FeatureGeometryGroup objects.
		 */
		typedef ReconstructedFeatureGeometryExport::feature_geometry_group_seq_type
			feature_geometry_group_seq_type;

		/**
		 * Typedef for a sequence of files that reference the geometries.
		 */
		typedef ReconstructedFeatureGeometryExport::referenced_files_collection_type
			referenced_files_collection_type;


		/**
		* Exports @a ReconstructedFeatureGeometry objects to ESRI Shapefile format.
		*/
		void
		export_geometries(
				const feature_geometry_group_seq_type &feature_geometry_group_seq,
				const QFileInfo& file_info,
				const referenced_files_collection_type &referenced_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time);
	}
}

#endif // GPLATES_FILEIO_SHAPEFILEFORMATRECONSTRUCTEDFEATUREGEOMETRYEXPORT_H
