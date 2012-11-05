/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date $
 * 
 * Copyright (C) 2007, 2008, 2009, 2010 The University of Sydney, Australia
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

#include "GpmlOutputVisitor.h"

#include <utility>
#include <vector>
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QProcess>
#include <QtCore/QUuid>
#include <QtGlobal> 

#include "ErrorOpeningFileForWritingException.h"
#include "ErrorOpeningPipeToGzipException.h"
#include "ExternalProgram.h"

#include "model/FeatureHandle.h"
#include "model/FeatureRevision.h"
#include "model/Gpgim.h"
#include "model/TopLevelPropertyInline.h"

#include "property-values/Enumeration.h"
#include "property-values/GmlDataBlock.h"
#include "property-values/GmlFile.h"
#include "property-values/GmlGridEnvelope.h"
#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlRectifiedGrid.h"
#include "property-values/GpmlPolarityChronId.h"
#include "property-values/GpmlPropertyDelegate.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlArray.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlFeatureReference.h"
#include "property-values/GpmlFeatureSnapshotReference.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlFiniteRotationSlerp.h"
#include "property-values/GpmlHotSpotTrailMark.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"
#include "property-values/GpmlMeasure.h"
#include "property-values/GpmlPiecewiseAggregation.h"
#include "property-values/GpmlPlateId.h"
#include "property-values/GpmlRasterBandNames.h"
#include "property-values/GpmlRevisionId.h"
#include "property-values/GpmlScalarField3DFile.h"
#include "property-values/GpmlStringList.h"
#include "property-values/GpmlTimeSample.h"
#include "property-values/GpmlTopologicalLine.h"
#include "property-values/GpmlTopologicalLineSection.h"
#include "property-values/GpmlTopologicalNetwork.h"
#include "property-values/GpmlTopologicalPoint.h"
#include "property-values/GpmlTopologicalPolygon.h"
#include "property-values/GpmlTopologicalSection.h"
#include "property-values/GpmlOldPlatesHeader.h"
#include "property-values/OldVersionPropertyValue.h"
#include "property-values/UninterpretedPropertyValue.h"
#include "property-values/StructuralType.h"
#include "property-values/XsString.h"
#include "property-values/XsBoolean.h"
#include "property-values/XsDouble.h"
#include "property-values/XsInteger.h"

#include "maths/PolylineOnSphere.h"
#include "maths/MultiPointOnSphere.h"
#include "maths/LatLonPoint.h"

#include "utils/Singleton.h"
#include "utils/UnicodeStringUtils.h"
#include "utils/XmlNamespaces.h"


namespace
{
	/**
	 * Replace the q_file_ptr's filename with a unique temporary .gpml filename.
	 */
	void
	set_temporary_filename(
		boost::shared_ptr<QFile> q_file_ptr)
	{
		if (!q_file_ptr)
		{
			return;
		}
		// I'm using a uuid to generate a unique name... is this overkill?
		QUuid uuid = QUuid::createUuid();
		QString uuid_string = uuid.toString();

		// The uuid string has braces around it, so remove them. 
		uuid_string.remove(0,1);
		uuid_string.remove(uuid_string.length()-1,1);

		QFileInfo file_info(*q_file_ptr);

		// And don't forget to put ".gpml" at the end. 
		q_file_ptr->setFileName(file_info.absolutePath() + QDir::separator() + uuid_string + ".gpml");
	}

	typedef std::pair<
		GPlatesModel::XmlAttributeName, 
		GPlatesModel::XmlAttributeValue> 
			XmlAttribute;

	template< typename QualifiedNameType >
	void
	writeTemplateTypeParameterType(
			GPlatesFileIO::XmlWriter &writer,
			const QualifiedNameType &value_type)
	{
		boost::optional<const GPlatesUtils::UnicodeString &> alias = 
			value_type.get_namespace_alias();

		GPlatesUtils::UnicodeString prefix;

		if (alias) 
		{
			// XXX:  This namespace declaration is a hack to get around the
			// fact that we can't access the current namespace declarations
			// from QXmlStreamWriter.  It ensures that the namespace of the 
			// QualifiedNameType about to be written has been
			// declared.
			writer.writeNamespace(
					GPlatesUtils::make_qstring_from_icu_string(value_type.get_namespace()),
					GPlatesUtils::make_qstring_from_icu_string(*alias));

			prefix = *alias;
		}
		else 
		{
			prefix = writer.getAliasForNamespace(value_type.get_namespace_iterator());
		}

		writer.writeText(prefix + ":" + value_type.get_name());
	}
	
	
	/**
	 * Convenience function to help write GmlPolygon's exterior and interior rings.
	 */
	void
	write_gml_linear_ring(
			GPlatesFileIO::XmlWriter &xml_output,
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_ptr)
	{
		xml_output.writeStartGmlElement("LinearRing");
	
		static std::vector<std::pair<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> > 
			pos_list_xml_attrs;
	
		if (pos_list_xml_attrs.empty()) 
		{
			GPlatesModel::XmlAttributeName attr_name =
				GPlatesModel::XmlAttributeName::create_gml("dimension");
			GPlatesModel::XmlAttributeValue attr_value("2");
			pos_list_xml_attrs.push_back(std::make_pair(attr_name, attr_value));
		}
		// FIXME: srsName?
		xml_output.writeStartGmlElement("posList");
		xml_output.writeAttributes(
				pos_list_xml_attrs.begin(),
				pos_list_xml_attrs.end());
	
		// It would be slightly "nicer" (ie, avoiding the allocation of a temporary buffer) if we
		// were to create an iterator which performed the following transformation for us
		// automatically, but (i) that's probably not the most efficient use of our time right now;
		// (ii) it's file I/O, it's slow anyway; and (iii) we can cut it down to a single memory
		// allocation if we reserve the size of the vector in advance.
		std::vector<double> pos_list;
		// Reserve enough space for the coordinates, to avoid the need to reallocate.
		//
		// number of coords = 
		//   (one for each segment start-point, plus one for the final end-point
		//   (all other end-points are the start-point of the next segment, so are not counted)),
		//   times two, since each point is a (lat, lon) duple.
		pos_list.reserve((polygon_ptr->number_of_segments() + 1) * 2);
	
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator begin = polygon_ptr->vertex_begin();
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator iter = begin;
		GPlatesMaths::PolygonOnSphere::vertex_const_iterator end = polygon_ptr->vertex_end();
		for ( ; iter != end; ++iter) 
		{
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
	
			// NOTE: We are assuming GPML is using (lat,lon) ordering.
			// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
			pos_list.push_back(llp.latitude());
			pos_list.push_back(llp.longitude());
		}
		// When writing gml:Polygons, the last point must be identical to the first point,
		// because the format wasn't verbose enough.
		GPlatesMaths::LatLonPoint begin_llp = GPlatesMaths::make_lat_lon_point(*begin);
		pos_list.push_back(begin_llp.latitude());
		pos_list.push_back(begin_llp.longitude());
		
		// Now that we have assembled the coordinates, write them into the XML.
		xml_output.writeNumericalSequence(pos_list.begin(), pos_list.end());
	
		// Don't forget to clear the vector when we're done with it!
		pos_list.clear();
	
		xml_output.writeEndElement(); // </gml:posList>
		xml_output.writeEndElement(); // </gml:LinearRing>
	}


	/**
	 * Convenience function to help write GmlPoint and GmlMultiPoint.
	 */
	void
	write_gml_point(
			GPlatesFileIO::XmlWriter &xml_output,
			const GPlatesMaths::PointOnSphere &point,
			GPlatesPropertyValues::GmlPoint::GmlProperty gml_property)
	{
		xml_output.writeStartGmlElement("Point");
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point);
			if (gml_property == GPlatesPropertyValues::GmlPoint::POS)
			{
				xml_output.writeStartGmlElement("pos");

					// NOTE: We are assuming GPML is using (lat,lon) ordering.
					// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
					xml_output.writeDecimalPair(llp.latitude(), llp.longitude());

				xml_output.writeEndElement();  // </gml:pos>
			}
			else // GmlPoint::COORDINATES
			{
				xml_output.writeStartGmlElement("coordinates");

					// NOTE: We are assuming GPML is using (lat,lon) ordering.
					// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
					xml_output.writeCommaSeparatedDecimalPair(llp.latitude(), llp.longitude());

				xml_output.writeEndElement();  // </gml:coordinates>
			}
		xml_output.writeEndElement();  // </gml:Point>
	}


	/**
	 * Similar to write_gml_point() but retrieves the lat-lon version of the point
	 * using GmlPoint::point_in_lat_lon().
	 *
	 * See the comments above GmlPoint::point_in_lat_lon for the rationale behind
	 * this special case.
	 */
	void
	write_gml_point_in_lat_lon(
			GPlatesFileIO::XmlWriter &xml_output,
			const GPlatesPropertyValues::GmlPoint &gml_point)
	{
		xml_output.writeStartGmlElement("Point");
			GPlatesMaths::LatLonPoint llp = gml_point.point_in_lat_lon();
			if (gml_point.gml_property() == GPlatesPropertyValues::GmlPoint::POS)
			{
				xml_output.writeStartGmlElement("pos");

					// NOTE: We are assuming GPML is using (lat,lon) ordering.
					// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
					xml_output.writeDecimalPair(llp.latitude(), llp.longitude());

				xml_output.writeEndElement();  // </gml:pos>
			}
			else
			{
				xml_output.writeStartGmlElement("coordinates");

					// NOTE: We are assuming GPML is using (lat,lon) ordering.
					// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
					xml_output.writeCommaSeparatedDecimalPair(llp.latitude(), llp.longitude());

				xml_output.writeEndElement();  // </gml:coordinates>
			}
		xml_output.writeEndElement();  // </gml:Point>
	}


	/**
	 * Convenience function to help write the value-object templates in the value-component
	 * properties in the composite-value in GmlDataBlock.
	 */
	void
	write_gml_data_block_value_component_value_object_template(
			GPlatesFileIO::XmlWriter &xml_output,
			GPlatesPropertyValues::GmlDataBlockCoordinateList::non_null_ptr_to_const_type
					coordinate_list)
	{
		xml_output.writeStartGmlElement("valueComponent");

		// Write a template of the value-object.

		// To understand what's happening in the next line, observe that
		// 'XmlWriter::writeStartElement' is a template function which has a template
		// parameter type 'SingletonType', which is the 'SingletonType' template type
		// parameter of QualifiedXmlName.  Thus, the template function overloads for
		// different template instantiations of QualifiedXmlName.
		xml_output.writeStartElement(coordinate_list->value_object_type());

		// Now follow up with the attributes for the element.  Note that to write XML
		// element attributes using QXmlStreamWriter, you follow an invocation of
		// 'QXmlStreamWriter::writeStartElement' immediately by an invocation of
		// 'QXmlStreamWriter::writeAttribute' before any content is written.
		// ( http://doc.trolltech.com/4.3/qxmlstreamwriter.html#writeAttribute )
		xml_output.writeAttributes(
				coordinate_list->value_object_xml_attributes().begin(),
				coordinate_list->value_object_xml_attributes().end());

		static const QString t("template");
		xml_output.writeText(t);

		// Now close the XML element tag of the value-object.
		xml_output.writeEndElement();

		xml_output.writeEndElement(); // </gml:valueComponent>
	}


	/**
	 * Convenience function to help write the tuple-list in GmlDataBlock.
	 *
	 * It's assumed that this function won't be called with an empty tuple list (ie, it's
	 * assumed that @a tuple_list_begin will never equal @a tuple_list_end).  It's also assumed
	 * that the vector passed into the function as @a coordinates_iterator_ranges is empty when
	 * it's passed into the function.  It's also assumed that the template parameter type
	 * @a TupleListIter is a forward iterator.
	 */
	template<typename CoordinatesIter, typename TupleListIter>
	void
	populate_coordinates_iterator_ranges(
			std::vector< std::pair<CoordinatesIter, CoordinatesIter> >
					&coordinates_iterator_ranges,
			TupleListIter tuple_list_begin,
			TupleListIter tuple_list_end)
	{
		coordinates_iterator_ranges.reserve(std::distance(tuple_list_begin, tuple_list_end));

		TupleListIter tuple_list_iter = tuple_list_begin;
		for ( ; tuple_list_iter != tuple_list_end; ++tuple_list_iter) {
			coordinates_iterator_ranges.push_back(std::make_pair(
						(*tuple_list_iter)->coordinates_begin(),
						(*tuple_list_iter)->coordinates_end()));
		}
	}


	/**
	 * Convenience function to help write the tuple-list in GmlDataBlock.
	 *
	 * It's assumed that this function won't be called with an empty tuple list (ie, it's
	 * assumed that @a ranges_begin will never equal @a ranges_end).  It's also assumed that
	 * the template parameter type @a RangesIter is a forward iterator which dereferences to a
	 * std::pair of iterators representing a half-open iterator range (i.e., [begin, end)).
	 */
	template<typename RangesIter>
	void
	write_tuple_list_from_coordinates_iterator_ranges(
			GPlatesFileIO::XmlWriter &xml_output,
			RangesIter ranges_begin,
			RangesIter ranges_end)
	{
		static const QString comma(",");
		static const QString space(" ");

		// Assume that when you dereference the iterator, you get a std::pair of iterators
		// representing a half-open iterator range (i.e., [begin, end)).
		typedef typename std::iterator_traits<RangesIter>::value_type CoordinatesIteratorRange;

		// Loop until we reach the end of any of the coordinate iterator ranges.
		for ( ; ; ) {
			RangesIter ranges_iter = ranges_begin;

			// We need to put a comma between adjacent coordinates in the tuple but a
			// space after the last coordinate in the tuple.  Hence, let's output the
			// first coordinate outside the loop, then within the loop each iteration
			// will be "write comma; write coordinate".
			{
				if (ranges_iter == ranges_end) {
					// Something strange has happened: The tuple-list is empty!
					// But we should already have handled this situation in the
					// invoking function.
					// FIXME:  Complain.
					return;
				}
				CoordinatesIteratorRange &range = *ranges_iter;
				if (range.first == range.second) {
					// We've reached the end of this range.
					return;
				}
				xml_output.writeDecimal(*(range.first++));
			}

			// Write the remaining coordinates in the tuple, preceded by commas.
			for (++ranges_iter; ranges_iter != ranges_end; ++ranges_iter) {
				CoordinatesIteratorRange &range = *ranges_iter;
				if (range.first == range.second) {
					// We've reached the end of this range.
					// But why didn't we reach the end of the range for the
					// first coordinate in the tuple?  This range must be
					// shorter than the range for the first coordinate...?
					// FIXME:  Complain.
					return;
				}
				xml_output.writeText(comma);
				xml_output.writeDecimal(*(range.first++));
			}

			// Now follow the coordinate tuple with a space.
			xml_output.writeText(space);
		}
	}


	/**
	 * Convenience function to help write the tuple-list in GmlDataBlock.
	 *
	 * It's OK if this function is called with an empty tuple list (ie, if @a tuple_list_begin
	 * equals @a tuple_list_end).  That situation will be handled gracefully within this
	 * function (by returning immediately).
	 */
	void
	write_gml_data_block_tuple_list(
			GPlatesFileIO::XmlWriter &xml_output,
			GPlatesPropertyValues::GmlDataBlock::tuple_list_type::const_iterator tuple_list_begin,
			GPlatesPropertyValues::GmlDataBlock::tuple_list_type::const_iterator tuple_list_end)
	{
		typedef GPlatesPropertyValues::GmlDataBlockCoordinateList::coordinate_list_type::const_iterator
				coordinates_iterator;
		typedef std::pair<coordinates_iterator, coordinates_iterator> coordinates_iterator_range;

		// Handle the situation when the tuple-list is empty.
		if (tuple_list_begin == tuple_list_end) {
			// Nothing to output.
			return;
		}

		std::vector<coordinates_iterator_range> coordinates_iterator_ranges;
		populate_coordinates_iterator_ranges(coordinates_iterator_ranges,
				tuple_list_begin, tuple_list_end);
		write_tuple_list_from_coordinates_iterator_ranges(xml_output,
				coordinates_iterator_ranges.begin(),
				coordinates_iterator_ranges.end());
	}


	template <typename T/*unused*/>
	class GzipCreationPolicy
	{
	public:
		static
		GPlatesFileIO::ExternalProgram *
		create_instance()
		{
			return new GPlatesFileIO::ExternalProgram("gzip", "gzip --version");
		}

		static
		void
		destroy_instance(
				GPlatesFileIO::ExternalProgram *external_program)
		{
			boost::checked_delete(external_program);
		}
	};
}


const GPlatesFileIO::ExternalProgram &
GPlatesFileIO::GpmlOutputVisitor::gzip_program()
{
	return GPlatesUtils::Singleton<ExternalProgram, GzipCreationPolicy>::instance();
}


GPlatesFileIO::GpmlOutputVisitor::GpmlOutputVisitor(
		const FileInfo &file_info,
		const GPlatesModel::Gpgim &gpgim,
		bool use_gzip):
	d_qfile_ptr(new QFile(file_info.get_qfileinfo().filePath())),
	d_qprocess_ptr(new QProcess),
	d_gzip_afterwards(false),
	d_output_filename(file_info.get_qfileinfo().filePath())
{

// If we're on a Windows system, we have to treat the whole gzip thing differently. 
//	The approach under Windows is:
//	1. Produce uncompressed output.
//	2. Gzip it.
//
//  To prevent us from overwriting any existing uncompressed file which the user may want preserved,
//  we generate a temporary gpml file name. The desired output file name is stored in the member variable
//  "d_output_filename". 
//
//	To implement this we do the following:
//	1. If we're on Windows, AND compressed output is requested, then
//		a. Set the "d_gzip_afterwards" flag to true. (We need to check the status of this flag in the destructor
//			where we'll actually do the gzipping).
//		b. Set "use_gzip" to false, so that uncompressed output will be produced.
//		c. Replace the d_qfile_ptr's filename with a temporary .gpml filename. 
//
//	2. In the destructor, (i.e. after the uncompressed file has been written), check if the 
//		"d_gzip_afterwards" flag is set. If so, gzip the uncompressed file, and rename it to 
//		"d_output_filename". If not, do the normal wait-for-process-to-end stuff. 
//																	
#ifdef Q_WS_WIN
	if (use_gzip) {
		d_gzip_afterwards = true;
		use_gzip = false;
		set_temporary_filename(d_qfile_ptr);
	}
#endif

	if ( ! d_qfile_ptr->open(QIODevice::WriteOnly | QIODevice::Text)) {
		throw ErrorOpeningFileForWritingException(GPLATES_EXCEPTION_SOURCE,
				file_info.get_qfileinfo().filePath());
	}

	if (use_gzip) {
		// Using gzip; We already opened the file using d_qfile_ptr, but that's okay,
		// since it verifies that we can actually write to that location.
		// Now we've verified that, we can close the file.
		d_qfile_ptr->close();
		
		// Set up the gzip process. Just like the QFile output, we need to keep it
		// as a shared_ptr belonging to this class.
		d_qprocess_ptr->setStandardOutputFile(file_info.get_qfileinfo().filePath());
		// FIXME: Assuming gzip is in a standard place on the path. Not true on MS/Win32. Not true at all.
		// In fact, it may need to be a user preference.
		d_qprocess_ptr->start(gzip_program().command());
		if ( ! d_qprocess_ptr->waitForStarted()) {
			throw ErrorOpeningPipeToGzipException(GPLATES_EXCEPTION_SOURCE,
					gzip_program().command(), file_info.get_qfileinfo().filePath());
		}
		// Use the newly-launched process as the device the XML writer writes to.
		d_output.setDevice(d_qprocess_ptr.get());
		
	} else {
		// Not using gzip, just write to the file as normal.
		d_output.setDevice(d_qfile_ptr.get());
		
	}
	
	start_writing_document(d_output, gpgim);
}


GPlatesFileIO::GpmlOutputVisitor::GpmlOutputVisitor(
		QIODevice *target,
		const GPlatesModel::Gpgim &gpgim):
	d_qfile_ptr(),
	d_qprocess_ptr(),
	d_output(target),
	d_gzip_afterwards(false)
{
	start_writing_document(d_output, gpgim);
}


void
GPlatesFileIO::GpmlOutputVisitor::start_writing_document(
		XmlWriter &writer,
		const GPlatesModel::Gpgim &gpgim)
{
	writer.writeStartDocument();

	writer.writeNamespace(
			GPlatesUtils::XmlNamespaces::GPML_NAMESPACE_QSTRING,
			GPlatesUtils::XmlNamespaces::GPML_STANDARD_ALIAS_QSTRING);
	writer.writeNamespace(
			GPlatesUtils::XmlNamespaces::GML_NAMESPACE_QSTRING,
			GPlatesUtils::XmlNamespaces::GML_STANDARD_ALIAS_QSTRING);
	writer.writeNamespace(
			GPlatesUtils::XmlNamespaces::XSI_NAMESPACE_QSTRING,
			GPlatesUtils::XmlNamespaces::XSI_STANDARD_ALIAS_QSTRING);

	writer.writeStartGpmlElement("FeatureCollection");

	writer.writeGpmlAttribute("version", gpgim.get_version().version_string());
	writer.writeAttribute(
			GPlatesUtils::XmlNamespaces::XSI_NAMESPACE_QSTRING,
			"schemaLocation",
			"http://www.gplates.org/gplates ../xsd/gpml.xsd "\
			"http://www.opengis.net/gml ../../../gml/current/base");
}


GPlatesFileIO::GpmlOutputVisitor::~GpmlOutputVisitor()
{
	// Wrap the entire body of the function in a 'try {  } catch(...)' block so that no
	// exceptions can escape from the destructor.
	try {
		d_output.writeEndElement(); // </gpml:FeatureCollection>
		d_output.writeEndDocument();


		// d_gzip_aftewards should only be true if we're on Windows, and compressed output was requested. 
		if (d_gzip_afterwards && d_qfile_ptr)
		{
			// Do the zipping now, but close the file first.
			d_output.device()->close();

			QStringList args;

			args << d_qfile_ptr->fileName();

			// The temporary gpml filename, and hence the corresponding .gpml.gz name, should be unique, 
			// so we can just go ahead and compress the file. 
			QProcess::execute(gzip_program().command(), args);
			
			// The requested output file may exist. If that is the case, at this stage the user has already
			// confirmed that the file be overwritten. We can't rename a file if a file with the new name
			// already exists, so remove it. 
			if (QFile::exists(d_output_filename))
			{
				QFile::remove(d_output_filename);
			}
			QString gz_filename = d_qfile_ptr->fileName() + ".gz";
			QFile::rename(gz_filename,d_output_filename);
		}
		else
		{
			// If we were using gzip compression, we must wait for the process to finish.
			if (d_qprocess_ptr.get() != NULL) {
				// in fact, we need to forcibly close the input to gzip, because
				// if we wait for it to go out of scope to clean itself up, there seems
				// to be a bit of data left in a buffer somewhere - either ours, or gzip's.
				d_qprocess_ptr->closeWriteChannel();
				d_qprocess_ptr->waitForFinished();
			}
		}
	}
	catch (...) {
		// Nothing we can really do in here, I think... unless we want to log that we
		// smothered an exception.  However, if we DO want to log that we smothered an
		// exception, we need to wrap THAT code in a try-catch block also, to ensure that
		// THAT code can't throw an exception which escapes the destructor.
	}
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_feature_handle(
		const GPlatesModel::FeatureHandle &feature_handle)
{
	// Every feature must be wrapped in a "gml:featureMember" element.
	d_output.writeStartGmlElement("featureMember");
		bool pop = d_output.writeStartElement(feature_handle.feature_type());

			d_output.writeStartGpmlElement("identity");
				d_output.writeText(feature_handle.feature_id().get());
			d_output.writeEndElement();

			d_output.writeStartGpmlElement("revision");
				d_output.writeText(feature_handle.revision_id().get());
			d_output.writeEndElement();

			// Now visit each of the properties in turn.
			visit_feature_properties(feature_handle);

		d_output.writeEndElement(pop); // </gpml:SomeFeature>
	d_output.writeEndElement(); // </gml:featureMember>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_top_level_property_inline(
		const GPlatesModel::TopLevelPropertyInline &top_level_property_inline)
{
	bool pop = d_output.writeStartElement(top_level_property_inline.property_name());

	// Top-level properties which also contain xml attributes
	// may be having their attributes written twice (at both the property
	// level, and here). To attempt to get round this, do not write
	// xml attributes at the top level.
	//
	// If this turns out to cause problems with other property types
	// we will have to find another solution.
	//
	// Similar modifications have been made in the GpmlReader
	// (or one of the classes it uses to read feature properties).
#if 0
		d_output.writeAttributes(
			top_level_property_inline.xml_attributes().begin(),
			top_level_property_inline.xml_attributes().end());
#endif

		visit_property_values(top_level_property_inline);
	d_output.writeEndElement(pop);
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_enumeration(
		const GPlatesPropertyValues::Enumeration &enumeration)
{
	d_output.writeText(enumeration.value().get());
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gml_data_block(
		const GPlatesPropertyValues::GmlDataBlock &gml_data_block)
{
	using namespace GPlatesPropertyValues;

	d_output.writeStartGmlElement("DataBlock");

	// First, output the <gml:CompositeValue> in the <gml:rangeParameters> (to mimic the
	// example on p.251 of the GML book).
		d_output.writeStartGmlElement("rangeParameters");
			d_output.writeStartGmlElement("CompositeValue");

			// Output each value-component in the composite-value.
			// If the tuple-list is empty, the body of the for-loop will never be entered, so the
			// <gml:CompositeValue> will be empty.
			GmlDataBlock::tuple_list_type::const_iterator iter = gml_data_block.tuple_list_begin();
			GmlDataBlock::tuple_list_type::const_iterator end = gml_data_block.tuple_list_end();
			for ( ; iter != end; ++iter) {
				write_gml_data_block_value_component_value_object_template(d_output, *iter);
			}

			d_output.writeEndElement(); // </gml:CompositeValue>
		d_output.writeEndElement(); // </gml:rangeParameters>

	// Now output the <gml:tupleList>.
	d_output.writeStartGmlElement("tupleList");
	write_gml_data_block_tuple_list(d_output, gml_data_block.tuple_list_begin(),
			gml_data_block.tuple_list_end());
	d_output.writeEndElement(); // </gml:tupleList>

	d_output.writeEndElement(); // </gml:DataBlock>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gml_file(
		const GPlatesPropertyValues::GmlFile &gml_file)
{
	using namespace GPlatesPropertyValues;

	d_output.writeStartGmlElement("File");

	// First, output the <gml:CompositeValue> in the <gml:rangeParameters> (to mimic the
	// example on p.252 of the GML book).
		d_output.writeStartGmlElement("rangeParameters");
			d_output.writeStartGmlElement("CompositeValue");

				// Output each value-component in the composite-value with its attributes.
				// The following code is based on write_gml_data_block_value_component_value_object_template
				// in the anonymous namespace above; see the comments there for an explanation.
				const GmlFile::composite_value_type &range_parameters = gml_file.range_parameters();
				BOOST_FOREACH(const GmlFile::value_component_type &value_component, range_parameters)
				{
					d_output.writeStartGmlElement("valueComponent");
						d_output.writeStartElement(value_component.first);
						d_output.writeAttributes(
								value_component.second.begin(),
								value_component.second.end());

							static const QString t("template");
							d_output.writeText(t);

						d_output.writeEndElement(); // close XML element tag of value-object.
					d_output.writeEndElement(); // </gml:valueComponent>
				}

			d_output.writeEndElement();
		d_output.writeEndElement(); // </gml:rangeParameters>

		d_output.writeStartGmlElement("fileName");
			d_output.writeRelativeFilePath(gml_file.file_name()->value().get());
		d_output.writeEndElement(); // </gml:fileName>

		d_output.writeStartGmlElement("fileStructure");
			visit_xs_string(*gml_file.file_structure());
		d_output.writeEndElement(); // </gml:fileStructure>

		// The next two are optional.
		const boost::optional<XsString::non_null_ptr_to_const_type> &mime_type = gml_file.mime_type();
		if (mime_type)
		{
			d_output.writeStartGmlElement("mimeType");
				visit_xs_string(**mime_type);
			d_output.writeEndElement(); // </gml:mimeType>
		}

		const boost::optional<XsString::non_null_ptr_to_const_type> &compression = gml_file.compression();
		if (compression)
		{
			d_output.writeStartGmlElement("compression");
				visit_xs_string(**compression);
			d_output.writeEndElement(); // </gml:compression>
		}

	d_output.writeEndElement(); // </gml:File>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gml_grid_envelope(
		const GPlatesPropertyValues::GmlGridEnvelope &gml_grid_envelope)
{
	d_output.writeStartGmlElement("GridEnvelope");

	const GPlatesPropertyValues::GmlGridEnvelope::integer_list_type &low = gml_grid_envelope.low();
	const GPlatesPropertyValues::GmlGridEnvelope::integer_list_type &high = gml_grid_envelope.high();

	d_output.writeStartGmlElement("low");
	d_output.writeNumericalSequence(low.begin(), low.end());
	d_output.writeEndElement(); // </gml:low>

	d_output.writeStartGmlElement("high");
	d_output.writeNumericalSequence(high.begin(), high.end());
	d_output.writeEndElement(); // </gml:high>

	d_output.writeEndElement(); // </gml:GridEnvelope>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gml_line_string(
		const GPlatesPropertyValues::GmlLineString &gml_line_string)
{
	d_output.writeStartGmlElement("LineString");

	static std::vector<std::pair<GPlatesModel::XmlAttributeName, GPlatesModel::XmlAttributeValue> > 
		pos_list_xml_attrs;

	if (pos_list_xml_attrs.empty()) 
	{
		GPlatesModel::XmlAttributeName attr_name =
			GPlatesModel::XmlAttributeName::create_gml("dimension");
		GPlatesModel::XmlAttributeValue attr_value("2");
		pos_list_xml_attrs.push_back(std::make_pair(attr_name, attr_value));
	}
	d_output.writeStartGmlElement("posList");
	d_output.writeAttributes(
			pos_list_xml_attrs.begin(),
			pos_list_xml_attrs.end());

	// It would be slightly "nicer" (ie, avoiding the allocation of a temporary buffer) if we
	// were to create an iterator which performed the following transformation for us
	// automatically, but (i) that's probably not the most efficient use of our time right now;
	// (ii) it's file I/O, it's slow anyway; and (iii) we can cut it down to a single memory
	// allocation if we reserve the size of the vector in advance.
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr =
			gml_line_string.polyline();
	std::vector<double> pos_list;
	// Reserve enough space for the coordinates, to avoid the need to reallocate.
	//
	// number of coords = 
	//   (one for each segment start-point, plus one for the final end-point
	//   (all other end-points are the start-point of the next segment, so are not counted)),
	//   times two, since each point is a (lat, lon) duple.
	pos_list.reserve((polyline_ptr->number_of_segments() + 1) * 2);

	GPlatesMaths::PolylineOnSphere::vertex_const_iterator iter = polyline_ptr->vertex_begin();
	GPlatesMaths::PolylineOnSphere::vertex_const_iterator end = polyline_ptr->vertex_end();
	for ( ; iter != end; ++iter) 
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);

		// NOTE: We are assuming GPML is using (lat,lon) ordering.
		// See http://trac.gplates.org/wiki/CoordinateReferenceSystem for details.
		pos_list.push_back(llp.latitude());
		pos_list.push_back(llp.longitude());
	}
	d_output.writeNumericalSequence(pos_list.begin(), pos_list.end());

	// Don't forget to clear the vector when we're done with it!
	pos_list.clear();

	d_output.writeEndElement(); // </gml:posList>
	d_output.writeEndElement(); // </gml:LineString>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gml_multi_point(
		const GPlatesPropertyValues::GmlMultiPoint &gml_multi_point) 
{
	d_output.writeStartGmlElement("MultiPoint");

	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multipoint_ptr =
			gml_multi_point.multipoint();
	typedef std::vector<GPlatesPropertyValues::GmlPoint::GmlProperty> gml_properties_type;
	const gml_properties_type &gml_properties = gml_multi_point.gml_properties();

	GPlatesMaths::MultiPointOnSphere::const_iterator iter = multipoint_ptr->begin();
	GPlatesMaths::MultiPointOnSphere::const_iterator end = multipoint_ptr->end();

	// gml_properties should have the same length as the multipoint.
	gml_properties_type::const_iterator gml_properties_iter = gml_properties.begin();

	for ( ; iter != end; ++iter, ++gml_properties_iter) 
	{
		d_output.writeStartGmlElement("pointMember");
		write_gml_point(d_output, *iter, *gml_properties_iter);
		d_output.writeEndElement(); // </gml:pointMember>
	}

	d_output.writeEndElement(); // </gml:MultiPoint>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gml_orientable_curve(
		const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve) 
{
	d_output.writeStartGmlElement("OrientableCurve");
		d_output.writeAttributes(
				gml_orientable_curve.xml_attributes().begin(),
				gml_orientable_curve.xml_attributes().end());

		d_output.writeStartGmlElement("baseCurve");
			gml_orientable_curve.base_curve()->accept_visitor(*this);
		d_output.writeEndElement();  // </gml:base_curve>

	d_output.writeEndElement();  // </gml:OrientableCurve>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gml_point(
		const GPlatesPropertyValues::GmlPoint &gml_point) 
{
	write_gml_point_in_lat_lon(d_output, gml_point);
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gml_polygon(
		const GPlatesPropertyValues::GmlPolygon &gml_polygon)
{
	d_output.writeStartGmlElement("Polygon");
	
	// GmlPolygon has exactly one exterior ring.
	d_output.writeStartGmlElement("exterior");
	write_gml_linear_ring(d_output, gml_polygon.exterior());
	d_output.writeEndElement(); // </gml:exterior>
	
	// GmlPolygon has zero or more interior rings.
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator it = gml_polygon.interiors_begin();
	GPlatesPropertyValues::GmlPolygon::ring_const_iterator end = gml_polygon.interiors_end();
	for ( ; it != end; ++it) {
		d_output.writeStartGmlElement("interior");
		write_gml_linear_ring(d_output, *it);
		d_output.writeEndElement(); // </gml:interior>
	}

	d_output.writeEndElement(); // </gml:Polygon>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gml_rectified_grid(
		const GPlatesPropertyValues::GmlRectifiedGrid &gml_rectified_grid)
{
	using namespace GPlatesPropertyValues;

	d_output.writeStartGmlElement("RectifiedGrid");
	const GmlRectifiedGrid::xml_attributes_type &xml_attributes = gml_rectified_grid.xml_attributes();
	d_output.writeAttributes(xml_attributes.begin(), xml_attributes.end());

		d_output.writeStartGmlElement("limits");
			visit_gml_grid_envelope(*gml_rectified_grid.limits());
		d_output.writeEndElement(); // </gml:limits>

		const GmlRectifiedGrid::axes_list_type &axes = gml_rectified_grid.axes();
		BOOST_FOREACH(const XsString::non_null_ptr_to_const_type &axis, axes)
		{
			d_output.writeStartGmlElement("axisName");
				visit_xs_string(*axis);
			d_output.writeEndElement(); // </gml:axisName>
		}

		d_output.writeStartGmlElement("origin");
			visit_gml_point(*gml_rectified_grid.origin());
		d_output.writeEndElement(); // </gml:origin>

		const GmlRectifiedGrid::offset_vector_list_type offset_vectors =
			gml_rectified_grid.offset_vectors();
		BOOST_FOREACH(const GmlRectifiedGrid::offset_vector_type &offset_vector, offset_vectors)
		{
			d_output.writeStartGmlElement("offsetVector");
				d_output.writeNumericalSequence(offset_vector.begin(), offset_vector.end());
			d_output.writeEndElement(); // </gml:offsetVector>
		}

	d_output.writeEndElement(); // </gml:RectifiedGrid>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gml_time_instant(
		const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant) 
{
	d_output.writeStartGmlElement("TimeInstant");
	d_output.writeStartGmlElement("timePosition");
		d_output.writeAttributes(
				gml_time_instant.time_position_xml_attributes().begin(),
				gml_time_instant.time_position_xml_attributes().end());

		const GPlatesPropertyValues::GeoTimeInstant &time_position = 
			gml_time_instant.time_position();
		if (time_position.is_real()) {
			d_output.writeDecimal(time_position.value());
		} else if (time_position.is_distant_past()) {
			d_output.writeText(QString("http://gplates.org/times/distantPast"));
		} else if (time_position.is_distant_future()) {
			d_output.writeText(QString("http://gplates.org/times/distantFuture"));
		}

	d_output.writeEndElement();  // </gml:timePosition>
	d_output.writeEndElement();  // </gml:TimeInstant>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gml_time_period(
		const GPlatesPropertyValues::GmlTimePeriod &gml_time_period) 
{
	d_output.writeStartGmlElement("TimePeriod");
		d_output.writeStartGmlElement("begin");
			gml_time_period.begin()->accept_visitor(*this);
		d_output.writeEndElement();

		d_output.writeStartGmlElement("end");
			gml_time_period.end()->accept_visitor(*this);
		d_output.writeEndElement();
	d_output.writeEndElement(); // </gml:TimePeriod>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_array(
		const GPlatesPropertyValues::GpmlArray &gpml_array)
{

	d_output.writeStartGpmlElement("Array");
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_array.type());
		d_output.writeEndElement();
	
	//d_output.writeStartGpmlElement("members");
		std::vector<GPlatesModel::PropertyValue::non_null_ptr_type>::const_iterator 
			iter = gpml_array.members().begin(),
			end = gpml_array.members().end();
		for ( ; iter != end; ++iter) {
			d_output.writeStartGpmlElement("member");
				(*iter)->accept_visitor(*this);
			d_output.writeEndElement();
		}
	d_output.writeEndElement();

}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_polarity_chron_id(
		const GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id) 
{
	d_output.writeStartGpmlElement("PolarityChronId");
	if (gpml_polarity_chron_id.get_era()) {
		d_output.writeStartGpmlElement("era");
		d_output.writeText(*gpml_polarity_chron_id.get_era());
		d_output.writeEndElement();
	}
	if (gpml_polarity_chron_id.get_major_region()) {
		d_output.writeStartGpmlElement("major");
		d_output.writeInteger(*gpml_polarity_chron_id.get_major_region());
		d_output.writeEndElement();
	}
	if (gpml_polarity_chron_id.get_minor_region()) {
		d_output.writeStartGpmlElement("minor");
		d_output.writeText(*gpml_polarity_chron_id.get_minor_region());
		d_output.writeEndElement();
	}
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_constant_value(
		const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
{
	d_output.writeStartGpmlElement("ConstantValue");
		d_output.writeStartGpmlElement("value");
			gpml_constant_value.value()->accept_visitor(*this);
		d_output.writeEndElement();


		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_constant_value.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_feature_reference(
		const GPlatesPropertyValues::GpmlFeatureReference &gpml_feature_reference)
{
	d_output.writeStartGpmlElement("FeatureReference");
		d_output.writeStartGpmlElement("targetFeature");
			d_output.writeText(gpml_feature_reference.feature_id().get());
		d_output.writeEndElement();


		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_feature_reference.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_feature_snapshot_reference(
		const GPlatesPropertyValues::GpmlFeatureSnapshotReference &gpml_feature_snapshot_reference)
{
	d_output.writeStartGpmlElement("FeatureSnapshotReference");
		d_output.writeStartGpmlElement("targetFeature");
			d_output.writeText(gpml_feature_snapshot_reference.feature_id().get());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("targetRevision");
			d_output.writeText(gpml_feature_snapshot_reference.revision_id().get());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_feature_snapshot_reference.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_property_delegate(
		const GPlatesPropertyValues::GpmlPropertyDelegate &gpml_property_delegate)
{
	d_output.writeStartGpmlElement("PropertyDelegate");
		d_output.writeStartGpmlElement("targetFeature");
			d_output.writeText(gpml_property_delegate.feature_id().get());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("targetProperty");
			writeTemplateTypeParameterType(d_output, gpml_property_delegate.target_property());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_property_delegate.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_finite_rotation(
		const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation) 
{
	if (gpml_finite_rotation.is_zero_rotation()) {
		d_output.writeEmptyGpmlElement("ZeroFiniteRotation");
	} else {
		d_output.writeStartGpmlElement("AxisAngleFiniteRotation");

			d_output.writeStartGpmlElement("eulerPole");
				GPlatesPropertyValues::GmlPoint::non_null_ptr_type gml_point =
						GPlatesPropertyValues::calculate_euler_pole(gpml_finite_rotation);
				visit_gml_point(*gml_point);
			d_output.writeEndElement();

			d_output.writeStartGpmlElement("angle");
				GPlatesMaths::real_t angle_in_degrees =
						GPlatesPropertyValues::calculate_angle(gpml_finite_rotation);
				d_output.writeDecimal(angle_in_degrees.dval());
			d_output.writeEndElement();

		d_output.writeEndElement();  // </gpml:AxisAngleFiniteRotation>
	}
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_finite_rotation_slerp(
		const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp)
{
	d_output.writeStartGpmlElement("FiniteRotationSlerp");
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_finite_rotation_slerp.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();
}

void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_key_value_dictionary(
		const GPlatesPropertyValues::GpmlKeyValueDictionary &gpml_key_value_dictionary)
{
	d_output.writeStartGpmlElement("KeyValueDictionary");
		//d_output.writeStartGpmlElement("elements");
			std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement>::const_iterator 
				iter = gpml_key_value_dictionary.elements().begin(),
				end = gpml_key_value_dictionary.elements().end();
			for ( ; iter != end; ++iter) {
				d_output.writeStartGpmlElement("element");
				write_gpml_key_value_dictionary_element(*iter);
				d_output.writeEndElement();
			}
		//d_output.writeEndElement();
	d_output.writeEndElement();
}

void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_piecewise_aggregation(
		const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation)
{
	d_output.writeStartGpmlElement("PiecewiseAggregation");
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_piecewise_aggregation.value_type());
		d_output.writeEndElement();

		std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator iter =
				gpml_piecewise_aggregation.time_windows().begin();
		std::vector<GPlatesPropertyValues::GpmlTimeWindow>::const_iterator end =
				gpml_piecewise_aggregation.time_windows().end();
		for ( ; iter != end; ++iter) 
		{
			d_output.writeStartGpmlElement("timeWindow");
				write_gpml_time_window(*iter);
			d_output.writeEndElement();
		}
	d_output.writeEndElement();  // </gpml:IrregularSampling>
}

void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_topological_network(
		const GPlatesPropertyValues::GpmlTopologicalNetwork &gpml_toplogical_network)
{
	d_output.writeStartGpmlElement("TopologicalNetwork");

		// Write the network boundary.
		d_output.writeStartGpmlElement("boundary");
			d_output.writeStartGpmlElement("TopologicalSections");
				// Write the boundary topological sections.
				GPlatesPropertyValues::GpmlTopologicalNetwork::boundary_sections_const_iterator
						boundary_sections_iter = gpml_toplogical_network.boundary_sections_begin();
				GPlatesPropertyValues::GpmlTopologicalNetwork::boundary_sections_const_iterator
						boundary_sections_end = gpml_toplogical_network.boundary_sections_end();
				for ( ; boundary_sections_iter != boundary_sections_end; ++boundary_sections_iter) 
				{
					d_output.writeStartGpmlElement("section");
					(*boundary_sections_iter)->accept_visitor(*this);
					d_output.writeEndElement();
				}
			d_output.writeEndElement();  // </gpml:TopologicalSections>
		d_output.writeEndElement();  // </gpml:boundary>

		// Write the network interior geometries.
		GPlatesPropertyValues::GpmlTopologicalNetwork::interior_geometries_const_iterator
				interior_geometries_iter = gpml_toplogical_network.interior_geometries_begin();
		GPlatesPropertyValues::GpmlTopologicalNetwork::interior_geometries_const_iterator
				interior_geometries_end = gpml_toplogical_network.interior_geometries_end();
		for ( ; interior_geometries_iter != interior_geometries_end; ++interior_geometries_iter) 
		{
			d_output.writeStartGpmlElement("interior");
			write_gpml_topological_network_interior(*interior_geometries_iter);
			d_output.writeEndElement();
		}

	d_output.writeEndElement();  // </gpml:TopologicalNetwork>
}

void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_topological_polygon(
	const GPlatesPropertyValues::GpmlTopologicalPolygon &gpml_toplogical_polygon)
{
	d_output.writeStartGpmlElement("TopologicalPolygon");

		// Write the exterior topological sections.
		d_output.writeStartGpmlElement("exterior");
			d_output.writeStartGpmlElement("TopologicalSections");
				GPlatesPropertyValues::GpmlTopologicalPolygon::sections_const_iterator iter =
						gpml_toplogical_polygon.exterior_sections_begin();
				GPlatesPropertyValues::GpmlTopologicalPolygon::sections_const_iterator end =
						gpml_toplogical_polygon.exterior_sections_end();
				for ( ; iter != end; ++iter) 
				{
					d_output.writeStartGpmlElement("section");
					(*iter)->accept_visitor(*this);
					d_output.writeEndElement();
				}
			d_output.writeEndElement();  // </gpml:TopologicalSections>
		d_output.writeEndElement();  // </gpml:exterior>

		// TODO: Write the topological interiors (interior hole regions).

	d_output.writeEndElement();  // </gpml:TopologicalPolygon>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_topological_line(
	const GPlatesPropertyValues::GpmlTopologicalLine &gpml_toplogical_line)
{
	d_output.writeStartGpmlElement("TopologicalLine");

	GPlatesPropertyValues::GpmlTopologicalPolygon::sections_const_iterator iter =
			gpml_toplogical_line.sections_begin();
	GPlatesPropertyValues::GpmlTopologicalPolygon::sections_const_iterator end =
			gpml_toplogical_line.sections_end();
	for ( ; iter != end; ++iter) 
	{
		d_output.writeStartGpmlElement("section");
		(*iter)->accept_visitor(*this);
		d_output.writeEndElement();
	}

	d_output.writeEndElement();  // </gpml:TopologicalLine>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_topological_line_section(
	const GPlatesPropertyValues::GpmlTopologicalLineSection &gpml_toplogical_line_section)
{  
	d_output.writeStartGpmlElement("TopologicalLineSection");

		d_output.writeStartGpmlElement("sourceGeometry");
			// visit the delgate 
			( gpml_toplogical_line_section.get_source_geometry() )->accept_visitor(*this); 
		d_output.writeEndElement();
		
		d_output.writeStartGpmlElement("reverseOrder");
			d_output.writeBoolean( gpml_toplogical_line_section.get_reverse_order() );
		d_output.writeEndElement();

	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_topological_point(
	const GPlatesPropertyValues::GpmlTopologicalPoint &gpml_toplogical_point)
{  
	d_output.writeStartGpmlElement("TopologicalPoint");
		d_output.writeStartGpmlElement("sourceGeometry");
			// visit the delegate
			( gpml_toplogical_point.get_source_geometry() )->accept_visitor(*this); 
		d_output.writeEndElement();
	d_output.writeEndElement();  
}

void
GPlatesFileIO::GpmlOutputVisitor::visit_hot_spot_trail_mark(
		const GPlatesPropertyValues::GpmlHotSpotTrailMark &gpml_hot_spot_trail_mark)
{
	d_output.writeStartGpmlElement("HotSpotTrailMark");
		d_output.writeStartGpmlElement("position");
			gpml_hot_spot_trail_mark.position()->accept_visitor(*this);
		d_output.writeEndElement();

		if (gpml_hot_spot_trail_mark.trail_width()) {
			d_output.writeStartGpmlElement("trailWidth");
				(*gpml_hot_spot_trail_mark.trail_width())->accept_visitor(*this);
			d_output.writeEndElement();
		}
		if (gpml_hot_spot_trail_mark.measured_age()) {
			d_output.writeStartGpmlElement("measuredAge");
				(*gpml_hot_spot_trail_mark.measured_age())->accept_visitor(*this);
			d_output.writeEndElement();
		}
		if (gpml_hot_spot_trail_mark.measured_age_range()) {
			d_output.writeStartGpmlElement("measuredAgeRange");
				(*gpml_hot_spot_trail_mark.measured_age_range())->accept_visitor(*this);
			d_output.writeEndElement();
		}
	d_output.writeEndElement();  // </gpml:HotSpotTrailMark>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_measure(
		const GPlatesPropertyValues::GpmlMeasure &gpml_measure)
{
	d_output.writeAttributes(
			gpml_measure.quantity_xml_attributes().begin(),
			gpml_measure.quantity_xml_attributes().end());
	d_output.writeDecimal(gpml_measure.quantity());
}


void
GPlatesFileIO::GpmlOutputVisitor::write_gpml_time_window(
		const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window)
{
	d_output.writeStartGpmlElement("TimeWindow");
		d_output.writeStartGpmlElement("timeDependentPropertyValue");
			gpml_time_window.time_dependent_value()->accept_visitor(*this);
		d_output.writeEndElement();
		d_output.writeStartGpmlElement("validTime");
			gpml_time_window.valid_time()->accept_visitor(*this);
		d_output.writeEndElement();
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_time_window.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement(); // </gpml:TimeWindow>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_irregular_sampling(
		const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling)
{
	d_output.writeStartGpmlElement("IrregularSampling");
		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator iter =
				gpml_irregular_sampling.time_samples().begin();
		std::vector<GPlatesPropertyValues::GpmlTimeSample>::const_iterator end =
				gpml_irregular_sampling.time_samples().end();
		for ( ; iter != end; ++iter) 
		{
			d_output.writeStartGpmlElement("timeSample");
				write_gpml_time_sample(*iter);
			d_output.writeEndElement();
		}

		// The interpolation function is optional.
		if (gpml_irregular_sampling.interpolation_function() != NULL)
		{
			d_output.writeStartGpmlElement("interpolationFunction");
				gpml_irregular_sampling.interpolation_function()->accept_visitor(*this);
			d_output.writeEndElement();
		}

		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_irregular_sampling.value_type());
		d_output.writeEndElement();
	d_output.writeEndElement();  // </gpml:IrregularSampling>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_plate_id(
		const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id)
{
	d_output.writeInteger(gpml_plate_id.value());
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_raster_band_names(
		const GPlatesPropertyValues::GpmlRasterBandNames &gpml_raster_band_names)
{
	d_output.writeStartGpmlElement("RasterBandNames");

		const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &band_names =
			gpml_raster_band_names.band_names();
		BOOST_FOREACH(const GPlatesPropertyValues::XsString::non_null_ptr_to_const_type &band_name, band_names)
		{
			d_output.writeStartGpmlElement("bandName");
				visit_xs_string(*band_name);
			d_output.writeEndElement(); // <gpml:bandName>
		}

	d_output.writeEndElement(); // </gpml:RasterBandNames>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_revision_id(
		const GPlatesPropertyValues::GpmlRevisionId &gpml_revision_id)
{
	d_output.writeText(gpml_revision_id.value().get());
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_scalar_field_3d_file(
		const GPlatesPropertyValues::GpmlScalarField3DFile &gpml_scalar_field_3d_file)
{
	d_output.writeStartGpmlElement("ScalarField3DFile");

		d_output.writeStartGpmlElement("fileName");
			d_output.writeRelativeFilePath(gpml_scalar_field_3d_file.file_name()->value().get());
		d_output.writeEndElement(); // <gpml:fileName>

	d_output.writeEndElement(); // </gpml:ScalarField3DFile>
}


void
GPlatesFileIO::GpmlOutputVisitor::write_gpml_time_sample(
		const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample) 
{
	d_output.writeStartGpmlElement("TimeSample");
		d_output.writeStartGpmlElement("value");
			gpml_time_sample.value()->accept_visitor(*this);
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("validTime");
			gpml_time_sample.valid_time()->accept_visitor(*this);
		d_output.writeEndElement();

		// The description is optional.
		if (gpml_time_sample.description() != NULL) 
		{
			d_output.writeStartGmlElement("description");
				gpml_time_sample.description()->accept_visitor(*this);
			d_output.writeEndElement();
		}

		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(d_output, gpml_time_sample.value_type());
		d_output.writeEndElement();

	d_output.writeEndElement();  // </gpml:TimeSample>
}

void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_old_plates_header(
		const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header)
{
	d_output.writeStartGpmlElement("OldPlatesHeader");

		d_output.writeStartGpmlElement("regionNumber");
 		d_output.writeInteger(gpml_old_plates_header.region_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("referenceNumber");
 		d_output.writeInteger(gpml_old_plates_header.reference_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("stringNumber");
 		d_output.writeInteger(gpml_old_plates_header.string_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("geographicDescription");
 		d_output.writeText(gpml_old_plates_header.geographic_description());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("plateIdNumber");
 		d_output.writeInteger(gpml_old_plates_header.plate_id_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("ageOfAppearance");
 		d_output.writeDecimal(gpml_old_plates_header.age_of_appearance());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("ageOfDisappearance");
 		d_output.writeDecimal(gpml_old_plates_header.age_of_disappearance());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("dataTypeCode");
 		d_output.writeText(gpml_old_plates_header.data_type_code());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("dataTypeCodeNumber");
 		d_output.writeInteger(gpml_old_plates_header.data_type_code_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("dataTypeCodeNumberAdditional");
 		d_output.writeText(gpml_old_plates_header.data_type_code_number_additional());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("conjugatePlateIdNumber");
 		d_output.writeInteger(gpml_old_plates_header.conjugate_plate_id_number());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("colourCode");
 		d_output.writeInteger(gpml_old_plates_header.colour_code());
		d_output.writeEndElement();

		d_output.writeStartGpmlElement("numberOfPoints");
 		d_output.writeInteger(gpml_old_plates_header.number_of_points());
		d_output.writeEndElement();

	d_output.writeEndElement();  // </gpml:OldPlatesHeader>
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_gpml_string_list(
			const GPlatesPropertyValues::GpmlStringList &gpml_string_list)
{
	d_output.writeStartGpmlElement("StringList");

		BOOST_FOREACH(const GPlatesPropertyValues::TextContent &text_content, gpml_string_list)
		{
			d_output.writeStartGpmlElement("element");
				d_output.writeText(text_content.get());
			d_output.writeEndElement();
		}
	d_output.writeEndElement();
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_xs_string(
		const GPlatesPropertyValues::XsString &xs_string)
{
	d_output.writeText(xs_string.value().get());
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_old_version_property_value(
		const GPlatesPropertyValues::OldVersionPropertyValue &old_version_prop_val)
{
	// NOTE: We really shouldn't get an 'OldVersionPropertyValue' in a feature because it's only used
	// during import when converting an old version property value to the latest version.
	// In other words they should never be added to a feature.

	// Log a warning for now.
	qWarning() << "Internal error: Encountered an 'OldVersionPropertyValue' property when writing GPML file '"
		<< d_output_filename
		<< "' - not writing property to file.";
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_uninterpreted_property_value(
		const GPlatesPropertyValues::UninterpretedPropertyValue &uninterpreted_prop_val)
{
	// XXX: Uncomment to indicate which property values weren't interpreted.
	//d_output.get_writer().writeEmptyElement("Uninterpreted");
	const GPlatesModel::XmlElementNode::non_null_ptr_to_const_type elem = 
		uninterpreted_prop_val.value();

	std::for_each(elem->children_begin(), elem->children_end(),
			boost::bind(&GPlatesModel::XmlNode::write_to, _1, 
				boost::ref(d_output.get_writer())));
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_xs_boolean(
		const GPlatesPropertyValues::XsBoolean &xs_boolean)
{
	d_output.writeBoolean(xs_boolean.value());
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_xs_double(
		const GPlatesPropertyValues::XsDouble &xs_double)
{
	d_output.writeDecimal(xs_double.value());
}


void
GPlatesFileIO::GpmlOutputVisitor::visit_xs_integer(
		const GPlatesPropertyValues::XsInteger &xs_integer)
{
	d_output.writeInteger(xs_integer.value());
}


void
GPlatesFileIO::GpmlOutputVisitor::write_gpml_key_value_dictionary_element(
			const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element)
{
	d_output.writeStartGpmlElement("KeyValueDictionaryElement");
		d_output.writeStartGpmlElement("key");
			element.key()->accept_visitor(*this);
		d_output.writeEndElement();
		d_output.writeStartGpmlElement("valueType");
			writeTemplateTypeParameterType(
				d_output,
				element.value_type());
		d_output.writeEndElement();
		d_output.writeStartGpmlElement("value");
			element.value()->accept_visitor(*this);
		d_output.writeEndElement();
	d_output.writeEndElement();
}

void
GPlatesFileIO::GpmlOutputVisitor::write_gpml_topological_network_interior(
		const GPlatesPropertyValues::GpmlTopologicalNetwork::Interior &gpml_toplogical_network_interior)
{
	d_output.writeStartGpmlElement("TopologicalNetworkInterior");
		d_output.writeStartGpmlElement("sourceGeometry");
			// visit the delegate 
			gpml_toplogical_network_interior.get_source_geometry()->accept_visitor(*this); 
		d_output.writeEndElement();
	d_output.writeEndElement();  // </gpml:TopologicalNetworkInterior>
}