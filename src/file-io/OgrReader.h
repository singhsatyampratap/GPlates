/* $Id: OgrReader.h 13647 2012-11-30 18:28:48Z rwatson $ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date: 2012-11-30 19:28:48 +0100 (Fri, 30 Nov 2012) $
 * 
 * Copyright (C) 2007, 2008, 2009 Geological Survey of Norway (under the name "ShapefileReader.h")
 * Copyright (C) 2010 The University of Sydney, Australia (under the name "ShapefileReader.h")
 * Copyright (C) 2012 Geological Survey of Norway
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

#ifndef GPLATES_FILEIO_OGRREADER_H
#define GPLATES_FILEIO_OGRREADER_H

#include <boost/shared_ptr.hpp>
#ifdef HAVE_CONFIG_H
// We're building on a UNIX-y system, and can thus expect "global/config.h".

// On some systems, it's <ogrsf_frmts.h>, on others, <gdal/ogrsf_frmts.h>.
// The "CMake" script should have determined which one to use.
#include "global/config.h"
#ifdef HAVE_GDAL_OGRSF_FRMTS_H
#include <gdal/ogrsf_frmts.h>
#else
#include <ogrsf_frmts.h>
#endif

#else  // We're not building on a UNIX-y system.  We'll have to assume it's <ogrsf_frmts.h>.
#include <ogrsf_frmts.h>
#endif  // HAVE_CONFIG_H

#include "File.h"
#include "FileInfo.h"
#include "PropertyMapper.h"
#include "ReadErrorAccumulation.h"

#include "model/ModelInterface.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelUtils.h"


namespace GPlatesModel
{
	class Gpgim;
}

namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		class OGRConfiguration;
	}

	const double SHAPE_NO_DATA = -1e38; 

	class OgrReader
	{

	public:

		/**
		 * Reads file specified by the filename in @a file_ref and stores into feature collection
		 * in @a file_ref.
		 *
		 * @a default_file_configuration should be the current default shapefile file configuration
		 * as determined by 'FeatureCollectionFileFormat::Registry'.
		 *
		 * @throws ErrorOpeningFileForReadingException if unable to open file for reading.
		 */
		static
		void
		read_file(
				GPlatesFileIO::File::Reference &file_ref,
				const boost::shared_ptr<const FeatureCollectionFileFormat::OGRConfiguration> &default_file_configuration,
				GPlatesModel::ModelInterface &model,
				const GPlatesModel::Gpgim &gpgim,
				ReadErrorAccumulation &read_errors);

		static
		void
		set_property_mapper(
			boost::shared_ptr< PropertyMapper > property_mapper);

		/**
		 * Reads only the field names from the file @a file_ref.
		 *
		 * @throws ErrorOpeningFileForReadingException if unable to open file for reading.
		 */
		static
		QStringList
		read_field_names(
				GPlatesFileIO::File::Reference &file_ref,
				GPlatesModel::ModelInterface &model,
				ReadErrorAccumulation &read_errors);

		/**
		 * Remaps the attributes stored in the file configuration of @a file to the
		 * mapped feature properties of the features in the feature collection in @a file.
		 *
		 * NOTE: This does not pop-up a remapper dialog anymore. That must already have been done.
		 * Instead, @a file *must* have a valid file configuration - this is now the source of the
		 * model-to-attribute mapping.
		 */
		static
		void
		remap_shapefile_attributes(
			GPlatesFileIO::File::Reference &file,
			GPlatesModel::ModelInterface &model,
			ReadErrorAccumulation &read_errors);

	private:

		OgrReader();

		~OgrReader();

// Make copy constructor private
		OgrReader(
			const OgrReader &other);


// Make assignment private
		OgrReader &
		operator=(
				const OgrReader &other);


		/**
		 * Checks that the file represented by OgrReader::d_filename can
		 * be opened, contains at least one layer, and that this layer contains
		 * at least one feature with a valid geometry.
		 *
		 * @return true if the above conditions are met, otherwise false.
		 */
		bool
		check_file_format(
				ReadErrorAccumulation &read_errors);

		bool 
		open_file(
				const QString &filename);

		void 
		get_field_names(
			ReadErrorAccumulation &read_errors);

		void 
		get_attributes();

		void
		handle_point(
				GPlatesModel::ModelInterface &model,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
				ReadErrorAccumulation &read_errors,
				const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);

		void
		handle_multi_point(
				GPlatesModel::ModelInterface &model,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
				ReadErrorAccumulation &read_errors,
				const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);


		void
		handle_linestring(
				GPlatesModel::ModelInterface &model,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
				ReadErrorAccumulation &read_errors,
				const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location
				);

		void
		handle_multi_linestring(
				GPlatesModel::ModelInterface &model,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
				ReadErrorAccumulation &read_errors,
				const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);

		void
		handle_polygon(
				GPlatesModel::ModelInterface &model,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
				ReadErrorAccumulation &read_errors,
				const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);

		void
		handle_multi_polygon(
				GPlatesModel::ModelInterface &model,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
				ReadErrorAccumulation &read_errors,
				const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
				const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);


		void
		read_features(
				GPlatesModel::ModelInterface &model,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
				ReadErrorAccumulation &read_errors);

		const GPlatesModel::FeatureHandle::weak_ref
		create_polygon_feature_from_list(
			GPlatesModel::ModelInterface &model,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			std::list<GPlatesMaths::PointOnSphere> &list);

		const GPlatesModel::FeatureHandle::weak_ref
		create_line_feature_from_list(
				GPlatesModel::ModelInterface &model,
				const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
				std::list<GPlatesMaths::PointOnSphere> &list);

		const GPlatesModel::FeatureHandle::weak_ref
		create_point_feature_from_pair(
			GPlatesModel::ModelInterface &model,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			std::pair<double,double> &pair);

		const GPlatesModel::FeatureHandle::weak_ref
		create_point_feature_from_point_on_sphere(
			GPlatesModel::ModelInterface &model,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			GPlatesMaths::PointOnSphere &point);

		const GPlatesModel::FeatureHandle::weak_ref
		create_multi_point_feature_from_list(
			GPlatesModel::ModelInterface &model,
			const GPlatesModel::FeatureCollectionHandle::weak_ref &collection,
			std::list<GPlatesMaths::PointOnSphere> &list);

		void
		add_attributes_to_feature(
			const GPlatesModel::FeatureHandle::weak_ref &,
			GPlatesFileIO::ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);

		bool 
		is_valid_shape_data(
			double lat,
			double lon,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location
			);

		void
		display_feature_counts();

		OGRwkbGeometryType 
		get_OGR_type();

		void
		add_ring_to_points_list(
			OGRLinearRing *ring,
			std::list<GPlatesMaths::PointOnSphere> &list,
			ReadErrorAccumulation &read_errors,
			const boost::shared_ptr<GPlatesFileIO::DataSource> &source,
			const boost::shared_ptr<GPlatesFileIO::LocationInDataSource> &location);


		QString d_filename;

		int d_num_layers;

		OGRDataSource *d_data_source_ptr;

		OGRGeometry *d_geometry_ptr;

		OGRFeature *d_feature_ptr;

		OGRLayer *d_layer_ptr;

		/// The type of the current geometry (e.g. LineString, Polygon, MultiPolygon...)
		OGRwkbGeometryType d_type;

		/// The shapefile attribute field names. 
		QStringList d_field_names;

		/// The shapefile attributes for the current geometry.
		std::vector<QVariant> d_attributes;

		/// Map for associating a model property with a shapefile attribute.
//		std::map<int, int> d_model_to_attribute_map;
		QMap<QString,QString> d_model_to_attribute_map;

#if 0
		/// The feature type and the geometry type
		std::pair<QString,QString> d_feature_creation_pair;
#endif
		QString d_feature_type;
		
		boost::optional<GPlatesUtils::UnicodeString> d_feature_id;

		/// The total number of geometries, including those from multi-geometries, in the file.
		unsigned d_total_geometries;

		/// The total number of geometries successfully loaded.
		unsigned d_loaded_geometries;

		/// The total number of features in the file.
		unsigned d_total_features;

		static boost::shared_ptr< PropertyMapper > s_property_mapper;
	};


}

#endif  // GPLATES_FILEIO_OGRREADER_H