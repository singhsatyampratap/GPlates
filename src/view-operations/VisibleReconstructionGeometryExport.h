/* $Id$ */

/**
 * \file Exports visible reconstructed feature geometries to a file.
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

#ifndef GPLATES_VIEWOPERATIONS_VISIBLERECONSTRUCTIONGEOMETRYEXPORT_H
#define GPLATES_VIEWOPERATIONS_VISIBLERECONSTRUCTIONGEOMETRYEXPORT_H

#include <vector>
#include <QString>

#include "file-io/File.h"

#include "model/types.h"


namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;

	namespace VisibleReconstructionGeometryExport
	{
		//! Typedef for sequence of feature collection files.
		typedef std::vector<const GPlatesFileIO::File::Reference *> files_collection_type;

		/**
		 * Collects visible @a ReconstructedFeatureGeometry objects that are displayed
		 * using @a rendered_geom_collection and exports to a file depending on the
		 * file extension of @a filename.
		 *
		 * @param active_files used to determine which files the RFGs came from.
		 * @param reconstruction_anchor_plate_id the anchor plate id used in the reconstruction.
		 * @param reconstruction_time time at which the reconstruction took place.
		 * @param export_single_output_file write all geometries to a single file.
		 * @param export_per_input_file write output files corresponding to input files.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_visible_reconstructed_feature_geometries(
				const QString &filename,
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const files_collection_type &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file);


		/**
		 * Collects visible @a ReconstructedFeatureGeometry objects that are displayed
		 * using @a rendered_geom_collection and exports to a file depending on the
		 * file extension of @a filename.
		 *
		 * @param active_files used to determine which files the RFGs came from.
		 * @param reconstruction_anchor_plate_id the anchor plate id used in the reconstruction.
		 * @param reconstruction_time time at which the reconstruction took place.
		 * @param export_single_output_file write all geometries to a single file.
		 * @param export_per_input_file write output files corresponding to input files.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_visible_reconstruced_flowlines(
				const QString &filename,
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const files_collection_type &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file);


		/**
		 * Collects visible @a ReconstructedMotionPath objects that are displayed
		 * using @a rendered_geom_collection and exports to a file depending on the
		 * file extension of @a filename.
		 *
		 * @param active_files used to determine which files the RFGs came from.
		 * @param reconstruction_anchor_plate_id the anchor plate id used in the reconstruction.
		 * @param reconstruction_time time at which the reconstruction took place.
		 * @param export_single_output_file write all geometries to a single file.
		 * @param export_per_input_file write output files corresponding to input files.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_visible_reconstruced_motion_paths(
				const QString &filename,
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const files_collection_type &active_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time,
				bool export_single_output_file,
				bool export_per_input_file);
	}
}

#endif // GPLATES_VIEWOPERATIONS_VISIBLERECONSTRUCTIONFEATUREGEOMETRYEXPORT_H
