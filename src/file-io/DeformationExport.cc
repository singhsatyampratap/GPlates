/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2016 The University of Sydney, Australia
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

#include <QLocale>
#include <QRegExp>
#include <QTextStream>

#include "DeformationExport.h"

#include "FileFormatNotSupportedException.h"
#include "GMTFormatDeformationExport.h"
#include "GpmlFormatDeformationExport.h"
#include "ReconstructionGeometryExportImpl.h"

#include "app-logic/TopologyReconstructedFeatureGeometry.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/LogException.h"


using namespace GPlatesFileIO::ReconstructionGeometryExportImpl;


namespace GPlatesFileIO
{
	namespace DeformationExport
	{
		namespace
		{
			//! Typedef for a sequence of @a TopologyReconstructedFeatureGeometry objects.
			typedef std::list< FeatureGeometryGroup<GPlatesAppLogic::TopologyReconstructedFeatureGeometry> >
					deformed_feature_geometry_seq_type;

			//! Typedef for a sequence of @a FeatureCollectionFeatureGroup objects.
			typedef std::list< FeatureCollectionFeatureGroup<GPlatesAppLogic::TopologyReconstructedFeatureGeometry> >
					grouped_features_seq_type;
		}
	}
}


void
GPlatesFileIO::DeformationExport::export_deformation_to_gpml_format(
		const QString &filename,
		const std::vector<const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *> &deformed_feature_geometry_seq,
		GPlatesModel::ModelInterface &model,
		const std::vector<const File::Reference *> &active_files,
		bool include_dilatation_rate,
		bool include_dilatation,
		bool export_single_output_file,
		bool export_per_input_file,
		bool export_separate_output_directory_per_input_file)
{
	// Get the list of active scalar coverage feature collection files that contain
	// the features referenced by the TopologyReconstructedFeatureGeometry objects.
	feature_handle_to_collection_map_type feature_to_collection_map;
	std::vector<const File::Reference *> referenced_files;
	get_files_referenced_by_geometries(
			referenced_files,
			deformed_feature_geometry_seq,
			active_files,
			feature_to_collection_map);

	// Group the TopologyReconstructedFeatureGeometry objects by their feature.
	deformed_feature_geometry_seq_type grouped_deformed_feature_geometry_seq;
	group_reconstruction_geometries_with_their_feature(
			grouped_deformed_feature_geometry_seq,
			deformed_feature_geometry_seq,
			feature_to_collection_map);

	if (export_single_output_file)
	{
		GpmlFormatDeformationExport::export_deformation(
				grouped_deformed_feature_geometry_seq,
				filename,
				model,
				include_dilatation_rate,
				include_dilatation);
	}

	if (export_per_input_file)
	{
		// Group the feature-groups with their collections. 
		grouped_features_seq_type grouped_features_seq;
		group_feature_geom_groups_with_their_collection(
				feature_to_collection_map,
				grouped_features_seq,
				grouped_deformed_feature_geometry_seq);

		std::vector<QString> output_filenames;
		get_output_filenames(
				output_filenames,
				filename,
				grouped_features_seq,
				export_separate_output_directory_per_input_file);

		grouped_features_seq_type::const_iterator grouped_features_iter = grouped_features_seq.begin();
		grouped_features_seq_type::const_iterator grouped_features_end = grouped_features_seq.end();
		for (std::vector<QString>::const_iterator output_filename_iter = output_filenames.begin();
			grouped_features_iter != grouped_features_end;
			++grouped_features_iter, ++output_filename_iter)
		{
			GpmlFormatDeformationExport::export_deformation(
					grouped_features_iter->feature_geometry_groups,
					*output_filename_iter,
					model,
					include_dilatation_rate,
					include_dilatation);
		}
	}
}


void
GPlatesFileIO::DeformationExport::export_deformation_to_gmt_format(
		const QString &filename,
		const std::vector<const GPlatesAppLogic::TopologyReconstructedFeatureGeometry *> &deformed_feature_geometry_seq,
		const std::vector<const File::Reference *> &active_files,
		const GPlatesModel::integer_plate_id_type &reconstruction_anchor_plate_id,
		const double &reconstruction_time,
		bool domain_point_lon_lat_format,
		bool include_dilatation_rate,
		bool include_dilatation,
		bool export_single_output_file,
		bool export_per_input_file,
		bool export_separate_output_directory_per_input_file)
{
	// Get the list of active scalar coverage feature collection files that contain
	// the features referenced by the TopologyReconstructedFeatureGeometry objects.
	feature_handle_to_collection_map_type feature_to_collection_map;
	std::vector<const File::Reference *> referenced_files;
	get_files_referenced_by_geometries(
			referenced_files,
			deformed_feature_geometry_seq,
			active_files,
			feature_to_collection_map);

	// Group the TopologyReconstructedFeatureGeometry objects by their feature.
	deformed_feature_geometry_seq_type grouped_deformed_feature_geometry_seq;
	group_reconstruction_geometries_with_their_feature(
			grouped_deformed_feature_geometry_seq,
			deformed_feature_geometry_seq,
			feature_to_collection_map);

	if (export_single_output_file)
	{
		GMTFormatDeformationExport::export_deformation(
				grouped_deformed_feature_geometry_seq,
				filename,
				referenced_files,
				reconstruction_anchor_plate_id,
				reconstruction_time,
				domain_point_lon_lat_format,
				include_dilatation_rate,
				include_dilatation);
	}

	if (export_per_input_file)
	{
		// Group the feature-groups with their collections. 
		grouped_features_seq_type grouped_features_seq;
		group_feature_geom_groups_with_their_collection(
				feature_to_collection_map,
				grouped_features_seq,
				grouped_deformed_feature_geometry_seq);

		std::vector<QString> output_filenames;
		get_output_filenames(
				output_filenames,
				filename,
				grouped_features_seq,
				export_separate_output_directory_per_input_file);

		grouped_features_seq_type::const_iterator grouped_features_iter = grouped_features_seq.begin();
		grouped_features_seq_type::const_iterator grouped_features_end = grouped_features_seq.end();
		for (std::vector<QString>::const_iterator output_filename_iter = output_filenames.begin();
			grouped_features_iter != grouped_features_end;
			++grouped_features_iter, ++output_filename_iter)
		{
			GMTFormatDeformationExport::export_deformation(
					grouped_features_iter->feature_geometry_groups,
					*output_filename_iter,
					referenced_files,
					reconstruction_anchor_plate_id,
					reconstruction_time,
					domain_point_lon_lat_format,
					include_dilatation_rate,
					include_dilatation);
		}
	}
}