/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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


#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QVariant>

#include "OgrWriter.h"
#include "OgrException.h"
#include "feature-visitors/ToQvariantConverter.h"
#include "maths/LatLonPointConversions.h"
#include "property-values/GpmlKeyValueDictionary.h"
#include "property-values/GpmlKeyValueDictionaryElement.h"

#if 1
// The only driver we're interested in for now is the Shapefile one. 
static const QString OGR_DRIVER_NAME("ESRI Shapefile");

#else
// But we'll try the KML driver for kicks....seems to work OK :) 
static const QString OGR_DRIVER_NAME("KML");
#endif

namespace{

	typedef std::vector<GPlatesPropertyValues::GpmlKeyValueDictionaryElement> element_type;
	typedef element_type::const_iterator element_iterator_type;


	QVariant
	get_qvariant_from_element(
		const GPlatesPropertyValues::GpmlKeyValueDictionaryElement &element)
	{
		GPlatesFeatureVisitors::ToQvariantConverter converter;

		element.value()->accept_visitor(converter);

		if (converter.found_values_begin() != converter.found_values_end())
		{
			return *converter.found_values_begin();
		}
		else
		{
			return QVariant();
		}
	}

	QString
	get_type_qstring_from_qvariant(
		QVariant &variant)
	{
		switch (variant.type())
		{
		case QVariant::Int:
			return QString("integer");
			break;
		case QVariant::Double:
			return QString("double");
			break;
		case QVariant::String:
			return QString("string");
			break;
		default:
			return QString();
		}
	}


	OGRFieldType
	get_ogr_field_type_from_qvariant(
		QVariant &variant)
	{
		switch (variant.type())
		{
		case QVariant::Int:
			return OFTInteger;
			break;
		case QVariant::Double:
			return OFTReal;
			break;
		case QVariant::String:
			return OFTString;
			break;
		default:
			return OFTString;
		}		
	}

	/**
	 * Sets the Ogr attribute field names and types from the key_value_dictionary elements.
	 */
	void
	set_layer_field_names(
		OGRLayer *ogr_layer,
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type &key_value_dictionary)
	{
		element_type elements = key_value_dictionary->elements();
		if (elements.empty())
		{
			qDebug() << "No elements in dictionary...";
			return;
		}

		if (ogr_layer != NULL)
		{
			element_iterator_type 
				iter = elements.begin(),
				end = elements.end();

			for ( ; iter != end ; ++iter)
			{
				// FIXME: Handle long field names....or prevent creation of long field names
				// at the appropriate point in the model-to-shapefile-attribute mapping process.
				// 
				// (Shapefile attribute field names are restricted to 10 characters in length. 
				// If the field name came from a shapefile, it'll already be of appropriate length.
				// But if the field name was generated by the user, it may not be...)

				QString key_string = GPlatesUtils::make_qstring_from_icu_string(iter->key()->value().get());

				QVariant value_variant = get_qvariant_from_element(*iter);
				QString type_string = get_type_qstring_from_qvariant(value_variant);

				//qDebug() << "Field name: " << key_string << ", type: " << type_string;

				OGRFieldType ogr_field_type = get_ogr_field_type_from_qvariant(value_variant);

				OGRFieldDefn ogr_field(key_string.toStdString().c_str(),ogr_field_type);

				if (ogr_layer->CreateField(&ogr_field) != OGRERR_NONE)
				{
					qDebug() << "Error creating ogr field. Name: " << key_string << ", type: " << type_string;
					throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating ogr field.");
				}

			}
		}
	}

	/**
	 * Set the Ogr attribute field values from the key_value_dictionary. 
	 */
	void
	set_feature_field_values(
		OGRLayer *ogr_layer,
		OGRFeature *ogr_feature,
		GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type key_value_dictionary)
	{
		if ((ogr_layer != NULL) && (ogr_feature != NULL))
		{
			int num_attributes_in_layer = ogr_layer->GetLayerDefn()->GetFieldCount();			
			int num_attributes_in_dictionary = key_value_dictionary->num_elements();

			if (num_attributes_in_layer != num_attributes_in_dictionary)
			{
				qDebug() << "Mismatch in number of fields.";
			}

			element_iterator_type 
				iter = key_value_dictionary->elements().begin(),
				end = key_value_dictionary->elements().end();

			for (int count = 0; (count < num_attributes_in_layer) && (iter != end) ; count++, ++iter)
			{
				QString model_string = GPlatesUtils::make_qstring_from_icu_string(iter->key()->value().get());
				QString layer_string = QString::fromStdString(
					ogr_layer->GetLayerDefn()->GetFieldDefn(count)->GetNameRef());

				if (QString::compare(model_string,layer_string) != 0)
				{
					// FIXME: Think of something suitable to do here. 
					qDebug() << "Mismatch in field names.";
				}

				QVariant value_variant = get_qvariant_from_element(*iter);
				QString type_string = get_type_qstring_from_qvariant(value_variant);

				OGRFieldType layer_type = ogr_layer->GetLayerDefn()->GetFieldDefn(count)->GetType();	
				OGRFieldType model_type  = get_ogr_field_type_from_qvariant(value_variant);

				if (layer_type != model_type)
				{
					// FIXME: Think of something suitable to do here. 
					qDebug() << "Mismatch in field types.";
				}

				// FIXME: Check that it's possible to represent the variants in the required forms.
				switch(layer_type)
				{
				case OFTInteger:
					ogr_feature->SetField(count,value_variant.toInt());
					break;
				case OFTReal:
					ogr_feature->SetField(count,value_variant.toDouble());
					break;
				case OFTString:
					ogr_feature->SetField(count,value_variant.toString().toStdString().c_str());
					break;
				default:
					// Handle anything else as a string. 
					ogr_feature->SetField(count,value_variant.toString().toStdString().c_str());
					
				}

			}

		}
	}

	/**
	 * Creates an OGRLayer of type wkb_type and adds it to the OGRDataSource.
	 * Adds any attribute field names provided in key_value_dictionary. 
	 */
	void
	setup_layer(
		OGRDataSource *ogr_data_source_ptr,
		boost::optional<OGRLayer*>& ogr_layer,
		OGRwkbGeometryType wkb_type,
		const QString &layer_name,
		boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> key_value_dictionary)
	{
		if (!ogr_data_source_ptr)
		{
			return;
		}
		if (!ogr_layer)
		{
			OGRSpatialReference spatial_reference; 
			spatial_reference.SetWellKnownGeogCS("WGS84");

			ogr_layer.reset(ogr_data_source_ptr->CreateLayer(
				layer_name.toStdString().c_str(),&spatial_reference,wkb_type,0));
			if (*ogr_layer == NULL)
			{
				qDebug() << "Error creating shapefile layer.";
				throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,"Error creating shapefile layer.");
			}
			if (key_value_dictionary && !((*key_value_dictionary)->is_empty()))
			{
				set_layer_field_names(*ogr_layer,*key_value_dictionary);
			}
		}
	}
		
	void
	remove_shapefile_layers(
		OGRSFDriver *ogr_driver,
		const QString &filename)
	{

		OGRDataSource *ogr_data_source_ptr = ogr_driver->Open(filename.toStdString().c_str(),true /* true to allow updates */);

		int number_of_layers = ogr_data_source_ptr->GetLayerCount();

		for (int count = 0; count < number_of_layers; ++count)
		{
			ogr_data_source_ptr->DeleteLayer(count);
		}

	}

}

GPlatesFileIO::OgrWriter::OgrWriter(
	QString filename,
	bool multiple_layers):
	d_filename(filename),
	d_root_filename(QString()),
	d_ogr_data_source_ptr(0),
	d_multiple_layers(multiple_layers)
{
	OGRRegisterAll();

	OGRSFDriver *ogr_driver = OGRSFDriverRegistrar::GetRegistrar()->GetDriverByName(OGR_DRIVER_NAME.toStdString().c_str());
	if (ogr_driver == NULL)
	{
		qDebug() << "Driver not available.";
		throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,"Ogr driver not available.");
	}

	QFileInfo q_file_info_original(d_filename);

	if (!multiple_layers)
	{
		// If there's no .shp extension, add one. 
		if (q_file_info_original.suffix().isEmpty())
		{
			d_filename.append(".shp");
		}
	}
	else
	{
		// If there is a .shp extension (or any other extension(s)), strip it/them.
		if (!q_file_info_original.completeSuffix().isEmpty())
		{
			d_filename = q_file_info_original.absolutePath() + QDir::separator() + 
				q_file_info_original.baseName();
		}
	}

	// FIXME: Consider saving the file to a temporary name, and rename it once export is complete. 

	QFileInfo q_file_info_modified(d_filename);
	if (q_file_info_modified.exists())
	{
		remove_shapefile_layers(ogr_driver,d_filename);
	}

	d_ogr_data_source_ptr = ogr_driver->CreateDataSource(d_filename.toStdString().c_str(), NULL );

	d_root_filename = q_file_info_original.baseName();

#if 0
	if (!q_file_info.exists())
	{
		// Create new file / folder
		d_ogr_data_source_ptr = ogr_driver->CreateDataSource(d_filename.toStdString().c_str(), NULL );
	}
	else
	{
		// File already exists; check if the user wants to overwrite.

		d_ogr_data_source_ptr = ogr_driver->Open(d_filename.toStdString().c_str(),true /* true to allow updates */);
	}
#endif

#if 0
	// test exception handling. 
	d_ogr_data_source_ptr = 0;
#endif

	if (d_ogr_data_source_ptr == NULL)
	{
		qDebug() << "Creation of data source failed.";
		throw GPlatesFileIO::OgrException(GPLATES_EXCEPTION_SOURCE,"Ogr data source creation failed.");
	}
}

GPlatesFileIO::OgrWriter::~OgrWriter()
{
	try {
		if(d_ogr_data_source_ptr){
			OGRDataSource::DestroyDataSource(d_ogr_data_source_ptr);
		}
	}
	catch (...) {
	}
}

void
GPlatesFileIO::OgrWriter::write_point_feature(
	GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere,
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
{

	// Check that we have a valid data_source. It shouldn't be possible to get here without a valid data_source pointer.
	if (d_ogr_data_source_ptr == NULL)
	{
		qDebug() << "NULL data source in write_point_feature";
		return;
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(d_ogr_data_source_ptr,d_ogr_point_layer,wkbPoint,QString(d_root_filename + "_point"),key_value_dictionary);

	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_point_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		qDebug() << "OGR error creating shapefile feature.";
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"OGR error creating shapefile feature.");
	}

	if (key_value_dictionary && !((*key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_point_layer,ogr_feature,*key_value_dictionary);
	}

	// Create the point feature from the point_on_sphere
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*point_on_sphere);

	OGRPoint ogr_point;
	ogr_point.setX(llp.longitude());
	ogr_point.setY(llp.latitude());

	ogr_feature->SetGeometry(&ogr_point);

	// Add the new feature to the layer.
	if ((*d_ogr_point_layer)->CreateFeature(ogr_feature) != OGRERR_NONE)
	{
		qDebug() << "Failed to create point feature.";
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Failed to create point feature.");
	}

	OGRFeature::DestroyFeature(ogr_feature);
}

void
GPlatesFileIO::OgrWriter::write_multi_point_feature(
	GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
{

	// Check that we have a valid data_source.
	if (d_ogr_data_source_ptr == NULL)
	{
		qDebug() << "NULL data source in write_multi_point_feature";
		return;
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(d_ogr_data_source_ptr,d_ogr_multi_point_layer,wkbMultiPoint,
		QString(d_root_filename + "_multi_point"),key_value_dictionary);

	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_multi_point_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		qDebug() << "Error creating shapefile feature.";
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"OGR error creating shapefile feature.");
	}

	if (key_value_dictionary && !((*key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_multi_point_layer,ogr_feature,*key_value_dictionary);
	}

	OGRMultiPoint ogr_multi_point;

	GPlatesMaths::MultiPointOnSphere::const_iterator 
		iter = multi_point_on_sphere->begin(),
		end = multi_point_on_sphere->end();

	for (; iter != end ; ++iter)
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
		OGRPoint ogr_point;
		ogr_point.setX(llp.longitude());
		ogr_point.setY(llp.latitude());
		ogr_multi_point.addGeometry(&ogr_point);
	}

	ogr_feature->SetGeometry(&ogr_multi_point);


	// Add the new feature to the layer.
	if ((*d_ogr_multi_point_layer)->CreateFeature(ogr_feature) != OGRERR_NONE)
	{
		qDebug() << "Failed to create multi-point feature.";
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Failed to create multi-point feature.");
	}

	OGRFeature::DestroyFeature(ogr_feature);
}

void
GPlatesFileIO::OgrWriter::write_polyline_feature(
	GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
{	

	// Check that we have a valid data_source.
	if (d_ogr_data_source_ptr == NULL)
	{
		qDebug() << "NULL data source in write_polyline_feature";
		return;
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(d_ogr_data_source_ptr,d_ogr_polyline_layer,wkbLineString,
		QString(d_root_filename + "_polyline"),key_value_dictionary);

	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_polyline_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		qDebug() << "Error creating shapefile feature.";
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"OGR error creating shapefile feature.");
	}

	if (key_value_dictionary && !((*key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_polyline_layer,ogr_feature,*key_value_dictionary);
	}

	OGRLineString ogr_line_string;

	GPlatesMaths::PolylineOnSphere::vertex_const_iterator 
		iter = polyline_on_sphere->vertex_begin(),
		end = polyline_on_sphere->vertex_end();

	for (; iter != end ; ++iter)
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
		OGRPoint ogr_point;
		ogr_point.setX(llp.longitude());
		ogr_point.setY(llp.latitude());
		ogr_line_string.addPoint(&ogr_point);
	}

	ogr_feature->SetGeometry(&ogr_line_string);


	// Add the new feature to the layer.
	if ((*d_ogr_polyline_layer)->CreateFeature(ogr_feature) != OGRERR_NONE)
	{
		qDebug() << "Failed to create polyline feature.";
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Failed to create polyline feature.");
	}

	OGRFeature::DestroyFeature(ogr_feature);
}


void
GPlatesFileIO::OgrWriter::write_polyline_feature(
	const std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type> &polylines, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
{
	
	if (polylines.empty())
	{
		return;
	}


	// Check that we have a valid data_source.
	if (d_ogr_data_source_ptr == NULL)
	{
		qDebug() << "NULL data source in write_polyline_feature";
		return;
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(d_ogr_data_source_ptr,d_ogr_polyline_layer,wkbMultiLineString,
		QString(d_root_filename + "_polyline"),key_value_dictionary);

	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_polyline_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		qDebug() << "Error creating shapefile feature.";
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"OGR error creating shapefile feature.");
	}

	if (key_value_dictionary && !((*key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_polyline_layer,ogr_feature,*key_value_dictionary);
	}

	OGRMultiLineString ogr_multi_line_string;

	std::vector<GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type>::const_iterator 
		it = polylines.begin(),
		end = polylines.end();
		
	for (; it != end ; ++it)
	{
	
		OGRLineString ogr_line_string;

		GPlatesMaths::PolylineOnSphere::vertex_const_iterator 
			line_iter = (*it)->vertex_begin(),
			line_end = (*it)->vertex_end();

		for (; line_iter != line_end ; ++line_iter)
		{
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*line_iter);
			OGRPoint ogr_point;
			ogr_point.setX(llp.longitude());
			ogr_point.setY(llp.latitude());
			ogr_line_string.addPoint(&ogr_point);
		}

		ogr_multi_line_string.addGeometry(&ogr_line_string);
	
	}

	ogr_feature->SetGeometry(&ogr_multi_line_string);


	// Add the new feature to the layer.
	if ((*d_ogr_polyline_layer)->CreateFeature(ogr_feature) != OGRERR_NONE)
	{
		qDebug() << "Failed to create multi polyline feature.";
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Failed to create polyline feature.");
	}

	OGRFeature::DestroyFeature(ogr_feature);
}


void
GPlatesFileIO::OgrWriter::write_polygon_feature(
	GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
{	

	// Check that we have a valid data_source.
	if (d_ogr_data_source_ptr == NULL)
	{
		qDebug() << "NULL data source in write_polygon_feature";
		return;
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(d_ogr_data_source_ptr,d_ogr_polygon_layer,wkbPolygon,
		QString(d_root_filename + "_polygon"),key_value_dictionary);


	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_polygon_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		qDebug() << "Error creating shapefile feature.";
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"OGR error creating shapefile feature.");
	}

	if (key_value_dictionary && !((*key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_polygon_layer,ogr_feature,*key_value_dictionary);
	}

	OGRPolygon ogr_polygon;
	OGRLinearRing ogr_linear_ring;

	GPlatesMaths::PolygonOnSphere::vertex_const_iterator 
		iter = polygon_on_sphere->vertex_begin(),
		end = polygon_on_sphere->vertex_end();

	for (; iter != end ; ++iter)
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*iter);
		OGRPoint ogr_point;
		ogr_point.setX(llp.longitude());
		ogr_point.setY(llp.latitude());
		ogr_linear_ring.addPoint(&ogr_point);
	}

	// Close the ring. GPlates stores polygons such that first-point != last-point; the 
	// ESRI shapefile specification says that polygon rings must be closed (first-point == last-point). 
	ogr_linear_ring.closeRings();

	// This will be the external ring. 
	ogr_polygon.addRing(&ogr_linear_ring);

	ogr_feature->SetGeometry(&ogr_polygon);

	// Add the new feature to the layer.
	if ((*d_ogr_polygon_layer)->CreateFeature(ogr_feature) != OGRERR_NONE)
	{
		qDebug() << "Failed to create polygon feature.";
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Failed to create polygon feature.");
	}

	OGRFeature::DestroyFeature(ogr_feature);
}

void
GPlatesFileIO::OgrWriter::write_polygon_feature(
	const std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type> &polygons, 
	const boost::optional<GPlatesPropertyValues::GpmlKeyValueDictionary::non_null_ptr_to_const_type> &key_value_dictionary)
{

	if (polygons.empty())
	{
		return;
	}

	// Check that we have a valid data_source.
	if (d_ogr_data_source_ptr == NULL)
	{
		qDebug() << "NULL data source in write_polygon_feature";
		return;
	}

	// Create the layer, if it doesn't already exist, and add any attribute names.
	setup_layer(d_ogr_data_source_ptr,d_ogr_polygon_layer,wkbMultiPolygon,
		QString(d_root_filename + "_polygon"),key_value_dictionary);


	OGRFeature *ogr_feature = OGRFeature::CreateFeature((*d_ogr_polygon_layer)->GetLayerDefn());

	if (ogr_feature == NULL)
	{
		qDebug() << "Error creating shapefile feature.";
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"OGR error creating shapefile feature.");
	}

	if (key_value_dictionary && !((*key_value_dictionary)->is_empty()))
	{
		set_feature_field_values(*d_ogr_polygon_layer,ogr_feature,*key_value_dictionary);
	}

	OGRMultiPolygon ogr_multi_polygon;

	std::vector<GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type>::const_iterator 
		it = polygons.begin(),
		end = polygons.end();

	for (; it != end ; ++it)
	{


		GPlatesMaths::PolygonOnSphere::vertex_const_iterator 
			polygon_iter = (*it)->vertex_begin(),
			polygon_end = (*it)->vertex_end();

		OGRPolygon ogr_polygon;
		OGRLinearRing ogr_linear_ring;


		for (; polygon_iter != polygon_end ; ++polygon_iter)
		{
			GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(*polygon_iter);
			OGRPoint ogr_point;
			ogr_point.setX(llp.longitude());
			ogr_point.setY(llp.latitude());
			ogr_linear_ring.addPoint(&ogr_point);
		}

		// Close the ring. GPlates stores polygons such that first-point != last-point; the 
		// ESRI shapefile specification says that polygon rings must be closed (first-point == last-point). 
		ogr_linear_ring.closeRings();

		// This will be the external ring. 
		ogr_polygon.addRing(&ogr_linear_ring);

		ogr_multi_polygon.addGeometry(&ogr_polygon);
	}
	
	ogr_feature->SetGeometry(&ogr_multi_polygon);

	// Add the new feature to the layer.
	if ((*d_ogr_polygon_layer)->CreateFeature(ogr_feature) != OGRERR_NONE)
	{
		qDebug() << "Failed to create polygon feature.";
		throw OgrException(GPLATES_EXCEPTION_SOURCE,"Failed to create multi-polygon feature.");
	}

	OGRFeature::DestroyFeature(ogr_feature);
}