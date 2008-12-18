/* $Id$ */

/**
 * \file 
 * Defines the interface for writing data in GMT xy format.
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_GMTFORMATWRITER_H
#define GPLATES_FILEIO_GMTFORMATWRITER_H

#include <iosfwd>
#include <vector>
#include <boost/optional.hpp>
#include <boost/scoped_ptr.hpp>
#include <QFile>
#include <QTextStream>
#include <QString>

#include "FileInfo.h"
#include "FeatureWriter.h"
#include "model/ConstFeatureVisitor.h"
#include "model/PropertyName.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/PointOnSphere.h"


namespace GPlatesFileIO
{
	class GMTHeaderFormatter;

	class GMTFormatWriter :
		public FeatureWriter,
		private GPlatesModel::ConstFeatureVisitor
	{
	public:

		enum HeaderFormat
		{
			// If feature has an old plates header then use that
			// otherwise print any elements of old plates header
			// (with defaults for missing elements).
			PLATES4_STYLE_HEADER,

			// Print verbose header containing feature's property values
			// printed on separate header lines.
			VERBOSE_HEADER,

			// If feature has an old plates header then use that
			// otherwise print verbose header.
			PREFER_PLATES4_STYLE_HEADER
		};

		/**
		* @pre is_writable(file_info) is true.
		* @param file_info file to write to.
		* @param header_format determines what information is printed in each feature header.
		*/
		explicit
			GMTFormatWriter(
			const FileInfo &file_info,
			HeaderFormat header_format = PLATES4_STYLE_HEADER);

		virtual
			~GMTFormatWriter();

		/**
		* Writes a feature in GMT 'xy' format.
		*
		* @param feature_handle feature to write
		*/
		virtual
			void
			write_feature(
			const GPlatesModel::FeatureHandle& feature_handle);

	private:
		virtual
			void
			visit_feature_handle(
			const GPlatesModel::FeatureHandle &feature_handle);

		virtual
			void
			visit_inline_property_container(
			const GPlatesModel::InlinePropertyContainer &inline_property_container);

		virtual
			void
			visit_gml_line_string(
			const GPlatesPropertyValues::GmlLineString &gml_line_string);

		virtual
			void
			visit_gml_multi_point(
			const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point);

		virtual
			void
			visit_gml_orientable_curve(
			const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve);

		virtual
			void
			visit_gml_point(
			const GPlatesPropertyValues::GmlPoint &gml_point);

		virtual
			void
			visit_gml_polygon(
			const GPlatesPropertyValues::GmlPolygon &gml_polygon);

		virtual
			void
			visit_gpml_constant_value(
			const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);


		//! Accumulates feature geometry(s) when visiting a feature.
		class FeatureAccumulator
		{
		public:
			typedef std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometries_type;
			typedef geometries_type::const_iterator geometries_const_iterator_type;

			void add_geometry(GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry)
			{
				d_feature_geometries.push_back(geometry);
			}

			bool have_geometry() const
			{
				return !d_feature_geometries.empty();
			}

			//@{
			/**
			 * Begin/end of geometry sequence.
			 * Dereference to get 'GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type'.
			 */
			geometries_const_iterator_type geometries_begin() const
			{
				return d_feature_geometries.begin();
			}
			geometries_const_iterator_type geometries_end() const
			{
				return d_feature_geometries.end();
			}
			//@}

			//! Clear accumulation when starting on a new feature.
			void clear()
			{
				d_feature_geometries.clear();
			}

		private:
			//! Stores geometries encountered while traversing a feature.
			std::vector<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_feature_geometries;
		};

		//! Prints lines of header and keeps track of writing starting '>' character.
		class HeaderPrinter
		{
		public:
			HeaderPrinter()
				: d_is_first_header_in_file(true)
			{ }

			void
				print_header_lines(
				QTextStream& output_stream,
				std::vector<QString>& header_lines);

		private:
			//! Is the next feature to be written the first one ?
			bool d_is_first_header_in_file;
		};

		boost::scoped_ptr<QFile> d_output_file;
		boost::scoped_ptr<QTextStream> d_output_stream;
		boost::scoped_ptr<GMTHeaderFormatter> d_feature_header;
		FeatureAccumulator d_feature_accumulator;
		HeaderPrinter d_header_printer;
	};
}

#endif // GPLATES_FILEIO_GMTFORMATWRITER_H