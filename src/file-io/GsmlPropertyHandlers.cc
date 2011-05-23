/* $Id$ */

/**
 * @file 
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
#include <boost/optional.hpp>
#include <QDebug>
#include <QXmlQuery>
#include <QXmlSerializer>
#include <QXmlStreamReader>
#include <QXmlResultItems>

#include "ArbitraryXmlReader.h"
#include "GsmlConst.h"
#include "GsmlPropertyHandlers.h"
#include "GsmlNodeProcessorFactory.h"
#include "PropertyCreationUtils.h"

#include "global/LogException.h"
#include "model/XmlNode.h"
#include "model/PropertyValue.h"
#include "model/ModelUtils.h"
#include "property-values/GmlLineString.h"
#include "property-values/TextContent.h"
#include "utils/SpatialReferenceSystem.h"
#include "utils/XQueryUtils.h"

using namespace GPlatesFileIO::GsmlConst;
using namespace GPlatesPropertyValues;
using namespace GPlatesModel;
using namespace GPlatesUtils;

namespace
{
	const QString
	get_element_text(
			QBuffer& xml_data)
	{
		QXmlStreamReader reader(&xml_data);
		if(XQuery::next_start_element(reader))
		{
			return reader.readElementText();
		}
		return QString();
	}
	
	/*
	* Since EPSG:4326 coordinates system put longitude before latitude, which is opposite in gpml.
	* This function changes the order of lat-lon.
	* The output will replace the input buffer.
	*/
	void
	normalize_geometry_coord(
			QByteArray& buf)
	{
		static const QString posList_begin = "<gml:posList";
		static const QString posList_end = "</gml:posList>";
		
		int idx = 0, idx_begin = 0, idx_end = 0;

		idx = buf.indexOf(posList_begin.toUtf8());
		while(idx != -1)
		{
			idx_begin = buf.indexOf(">", idx)+1;
			idx_end = buf.indexOf(posList_end.toUtf8(), idx_begin);

			if(idx_end == -1 || idx_begin == -1)
			{
				qWarning() << "The input data is not well-formed.";
				break;
			}

			QByteArray tail = buf.right(buf.size()-idx_end);
			QList<QByteArray> list = buf.mid(idx_begin, idx_end-idx_begin).simplified().split(' ');
			buf.chop(buf.size()-idx_begin);
			
			//swap the lat-lon pair.
			for(int i=0; i<list.size();i+=2)
			{
				buf.append(" ");
				buf.append(list[i+1]);
				buf.append(" ");
				buf.append(list[i]);
			}
			//append the rest of data.
			idx_end = buf.length();
			buf.append(tail);
			
			//move to next "<gml:posList>  </gml:posList>" block.
			idx_end = buf.indexOf(posList_end,idx_end) + posList_end.length();
			idx = buf.indexOf(posList_begin.toUtf8(),idx_end);
		}
	}

	/*
	* Get the Spatial Reference System name from xml data.
	*/
	const QString
	get_srs_name(
			QByteArray& array_buf)
	{
		std::vector<QVariant> results = 
			GPlatesUtils::XQuery::evaluate_attribute(array_buf,"srsName");
		std::size_t s = results.size();
		if( s >= 1)
		{
			if(s > 1)
			{
				qWarning() << "More than one srsName attributes have been found.";
				qWarning() << "Only the first one will be returned.";
			}
			return results[0].toString();
		}
		return QString();
	}

	/*
	* Check the input name and determine if it is EPSG_4326.
	*/
	inline
	bool
	is_epsg_4326(
			QString& name)
	{
		return (name.contains("EPSG",Qt::CaseInsensitive) && name.contains("4326"));
	}

	using GPlatesUtils::SpatialReferenceSystem::Dimension;
	
	/*
	* Find the dimension of Spatial Reference System from xml data buffer.
	*/
	const Dimension
	find_srs_dimension(
			const QByteArray& buf)
	{
		QXmlStreamReader reader(buf);
		QXmlStreamAttributes attrs;
		while(XQuery::next_start_element(reader))
		{
			if(reader.name() == "posList")
			{
				attrs = reader.attributes();
				break;
			}
		}

		BOOST_FOREACH(QXmlStreamAttribute& attr, attrs)
		{
			if(
				attr.name().toString() == "srsDimension" && 
				attr.value().toString() == "3")
				{
					return Dimension::threeD();
				}
		}
		return Dimension::twoD();
	}

	/*
	* Transform data into EPSG 4326.
	* Output data will replace input buffer.
	*/
	void
	convert_to_epsg_4326(
			QByteArray& buf)
	{
		using namespace GPlatesUtils::SpatialReferenceSystem;

		QString srs_name = get_srs_name(buf);
		
		if(srs_name.size() == 0)
		{
			qWarning() << "No Spatial Reference System name found. Use default EPSG:4326.";
			srs_name = "EPSG:4326";
		}
		else if(is_epsg_4326(srs_name))
		{
			//if it is already EPSG:4326, do nothing.
			qDebug() << "The Spatial Reference System is already EPSG:4326. Do nothing and return";
			return;
		}

		static const QString posList_begin = "<gml:posList";
		static const QString posList_end = "</gml:posList>";
		
		int idx = 0, idx_begin = 0, idx_end = 0;
		Dimension srs_dimension = Dimension::twoD(); //by default, 2D

		idx = buf.indexOf(posList_begin.toUtf8());
		while(idx != -1)
		{
			idx_begin = buf.indexOf(">", idx) + 1;
			srs_dimension = find_srs_dimension(buf.mid(idx, idx_begin-idx));
			idx_end = buf.indexOf(posList_end.toUtf8(), idx_begin);

			if(idx_end == -1)
			{
				qWarning() << "The XML data is not well-formedin convert_to_EPSG_4326().";
				break;
			}

			QByteArray tail = buf.right(buf.size()-idx_end);
			QByteArray head = buf.left(idx_begin);
			QList<QByteArray> list = buf.mid(idx_begin, idx_end-idx_begin).simplified().split(' ');
			buf.clear();buf.append(head);

			std::vector<Coordinates> coordinates;
			QList<QByteArray>::const_iterator it = list.begin();
			QList<QByteArray>::const_iterator it_end = list.end();
			for(;it != it_end;)
			{
				//TODO:
				//validate the QList
				//check the result flag of toDouble()
				//bool f; toDouble(&f)
				std::vector<double> tmp;
				tmp.push_back((*it).toDouble()); ++it;
				tmp.push_back((*it).toDouble()); ++it;
				if(srs_dimension == Dimension::threeD())
				{
					tmp.push_back((*it).toDouble()); ++it;
				}
				coordinates.push_back(
						Coordinates(
								tmp,
								CoordinateReferenceSystem::create_by_name(
										srs_name,
										srs_dimension)));
			}

			transform(
					CoordinateReferenceSystem::create_by_name(srs_name,srs_dimension),
					CoordinateReferenceSystem::espg_4326(),
					coordinates);

			list.clear();
			std::vector<Coordinates>::const_iterator c_it = coordinates.begin();
			std::vector<Coordinates>::const_iterator c_it_end = coordinates.end();
			for(;c_it != c_it_end; ++c_it)
			{
				buf.append(" ");
				buf.append(QByteArray().setNum(c_it->x()));
				buf.append(" ");
				buf.append(QByteArray().setNum(c_it->y()));
			}

			idx_end = buf.length();
			buf.append(tail);
		
			idx_end = buf.indexOf(posList_end,idx_end) + posList_end.length();
			idx = buf.indexOf(posList_begin.toUtf8(),idx_end);
		}
	}

	/*
	* Create XmlElementNode from QBuffer
	*/
	GPlatesModel::XmlElementNode::non_null_ptr_type
	create_xml_node(
			QBuffer& buf)
	{
		QXmlStreamReader reader(&buf);
		XQuery::next_start_element(reader);

		boost::shared_ptr<XmlElementNode::AliasToNamespaceMap> alias_map(
			new XmlElementNode::AliasToNamespaceMap());

		return XmlElementNode::create(reader,alias_map);
	}

	/*
	* Create XmlElementNode from QByteArray
	*/
	GPlatesModel::XmlElementNode::non_null_ptr_type
	create_xml_node(
			QByteArray& array)
	{
		QBuffer buffer(&array);
		buffer.open(QIODevice::ReadOnly | QIODevice::Text);
		if(!buffer.isOpen())
		{
			throw GPlatesGlobal::LogException(
					GPLATES_EXCEPTION_SOURCE,	
					"Unable to open buffer.");
		}
		return create_xml_node(buffer);
	}
}


GPlatesFileIO::GsmlPropertyHandlers::GsmlPropertyHandlers(
		GPlatesModel::FeatureHandle::weak_ref fh):
	d_feature(fh)
{	
	d_read_errors = ArbitraryXmlReader::instance()->get_read_error_accumulation();
}


void
GPlatesFileIO::GsmlPropertyHandlers::process_geometries(
		QBuffer& xml_data,
		const QString& query_str)
{
	QByteArray buf_array = xml_data.data();
	std::vector<QByteArray> results = 
		XQuery::evaluate(
				buf_array,
				query_str,
				boost::bind(&XQuery::is_empty,_1));
	
	BOOST_FOREACH(QByteArray& array, results)
	{
		XQuery::wrap_xml_data(array,"gml:baseCurve");
		convert_to_epsg_4326(array);
		normalize_geometry_coord(array);
			
		//gplates doesn't support gml:outerBoundaryIs and gml:innerBoundaryIs
		//replace them with gml:exterior and gml:interior
		array.replace("outerBoundaryIs", "exterior");
		array.replace("innerBoundaryIs", "interior");

		XmlElementNode::non_null_ptr_type xml_node = 
			create_xml_node(array);

		boost::optional<PropertyValue::non_null_ptr_type> geometry_property = boost::none;

		if(query_str.indexOf("LineString") != -1)
		{
			geometry_property = 
					PropertyCreationUtils::create_line_string(
							xml_node,
							*d_read_errors);
		}
		else if(query_str.indexOf("Polygon") != -1)
		{
			geometry_property = 
					PropertyCreationUtils::create_gml_polygon(
							xml_node,
							*d_read_errors);
		}
		else if(query_str.indexOf("Point") != -1)
		{
			geometry_property = 
				PropertyCreationUtils::create_point(
							xml_node,
							*d_read_errors);
		}

		if(!geometry_property)
		{
			qWarning() << "Failed to create geometry property.";
		}
		else
		{
			ModelUtils::add_property(
					d_feature,
					PropertyName::create_gpml("GsmlGeometry"),
					*geometry_property);
		}
	}
}


void
GPlatesFileIO::GsmlPropertyHandlers::handle_geometry_property(
		QBuffer& xml_data)
{
	//keep the query strings locally for better readability.
	process_geometries(xml_data, "/gsml:shape/gml:LineString");
	process_geometries(xml_data, "/gsml:shape/gml:point");
	process_geometries(xml_data, "/gsml:shape/gml:Polygon");
}


void
GPlatesFileIO::GsmlPropertyHandlers::handle_observation_method(
		QBuffer& xml_data)
{
	ModelUtils::add_property<UninterpretedPropertyValue>(
			d_feature,
			PropertyName::create_gpml("ObservationMethod"),
			create_xml_node(xml_data));
}


void
GPlatesFileIO::GsmlPropertyHandlers::handle_gml_name(
		QBuffer& xml_data)
{
	ModelUtils::add_property<XsString>(
			d_feature,
			PropertyName::create_gml("name"),
			UnicodeString(get_element_text(xml_data)));
}


void
GPlatesFileIO::GsmlPropertyHandlers::handle_gml_desc(
		QBuffer& xml_data)
{
	ModelUtils::add_property<XsString>(
			d_feature,
			PropertyName::create_gml("description"),
			UnicodeString(get_element_text(xml_data)));
}


void
GPlatesFileIO::GsmlPropertyHandlers::handle_occurrence_property(
		QBuffer& xml_data)
{
	std::vector<QByteArray> results = 
		XQuery::evaluate(
				xml_data,
				"/gsml:occurrence/gsml:MappedFeature/gsml:shape",
				boost::bind(&XQuery::is_empty,_1));
	BOOST_FOREACH(QByteArray& array, results)
	{
		QBuffer buffer(&array);
		buffer.open(QIODevice::ReadOnly | QIODevice::Text);
		if(!buffer.isOpen())
		{
			throw GPlatesGlobal::LogException(
				GPLATES_EXCEPTION_SOURCE,	
				"Unable to open buffer.");
		}
		handle_geometry_property(buffer);
	}
}

















