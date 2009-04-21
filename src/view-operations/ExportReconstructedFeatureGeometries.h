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

#ifndef GPLATES_VIEWOPERATIONS_EXPORTRECONSTRUCTEDFEATUREGEOMETRIES_H
#define GPLATES_VIEWOPERATIONS_EXPORTRECONSTRUCTEDFEATUREGEOMETRIES_H

#include <vector>
#include <QString>

#include "model/types.h"
#include "qt-widgets/ApplicationState.h"


namespace GPlatesModel
{
	class Reconstruction;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;

	namespace ExportReconstructedFeatureGeometries
	{
		//! Typedef for iterator into global list of loaded feature collection files.
		typedef GPlatesAppState::ApplicationState::file_info_iterator file_info_const_iterator_type;

		//! Typedef for sequence of feature collection file iterators.
		typedef std::vector<file_info_const_iterator_type> active_files_collection_type;

		/**
		 * Collects visible @a ReconstructedFeatureGeometry objects from
		 * @a reconstruction that are displayed using @a rendered_geom_collection
		 * and exports to a file depending on the file extension of @a filename.
		 * Only those @a ReconstructionFeatureGeometry objects that are visible in
		 * the RECONSTRUCTION_LAYER of @a rendered_geom_collection are exported.
		 *
		 * @param active_reconstructable_files used to determine which files the RFGs came from.
		 * @param reconstruction_anchor_plate_id the anchor plate id used in the reconstruction.
		 * @param reconstruction_time time at which the reconstruction took place.
		 *
		 * @throws ErrorOpeningFileForWritingException if file is not writable.
		 * @throws FileFormatNotSupportedException if file format not supported.
		 */
		void
		export_visible_geometries(
				const QString &filename,
				const GPlatesModel::Reconstruction &reconstruction,
				const GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const active_files_collection_type &active_reconstructable_files,
				const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
				const double &reconstruction_time);
	}
}

#endif // GPLATES_VIEWOPERATIONS_EXPORTRECONSTRUCTEDFEATUREGEOMETRIES_H
