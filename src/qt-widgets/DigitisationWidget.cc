/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#include <QtGlobal>
#include <QDebug>
#include <QHeaderView>
#include <QTreeWidget>
#include <QUndoStack>
#include <QToolButton>
#include <QMessageBox>
#include <boost/optional.hpp>
#include <boost/none.hpp>

#include "DigitisationWidget.h"
#include "DigitisationWidgetUndoCommands.h"

#include "ViewportWindow.h"
#include "ExportCoordinatesDialog.h"
#include "CreateFeatureDialog.h"
#include "maths/InvalidLatLonException.h"
#include "maths/InvalidLatLonCoordinateException.h"
#include "maths/LatLonPointConversions.h"
#include "maths/GeometryOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"


namespace
{
	/**
	 * Simple typedef for convenient calling of some of PolylineOnSphere's members.
	 */
	typedef GPlatesMaths::PolylineOnSphere polyline_type;
	
	/**
	 * Type of intrusive pointer to use for polylines.
	 */
	typedef GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_ptr_type;
	
	/**
	 * This typedef is used wherever geometry (of some unknown type) is expected.
	 * It is a boost::optional because creation of geometry may fail for various reasons.
	 */
	typedef boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> geometry_opt_ptr_type;
	
	
	
	
	/**
	 * Determines what fragment of geometry the top-level tree widget item
	 * would become, given the current configuration of the DigitisationWidget
	 * and the position and number of children in this top-level item.
	 */
	QString
	calculate_label_for_item(
			GPlatesQtWidgets::DigitisationWidget::GeometryType target_geom_type,
			int position,
			QTreeWidgetItem *item)
	{
		QString label;
		// Pick a sensible default.
		switch (target_geom_type)
		{
		default:
				label = QObject::tr("<Error: unknown GeometryType>");
				break;
		case GPlatesQtWidgets::DigitisationWidget::POLYLINE:
				label = "gml:LineString";
				break;
		case GPlatesQtWidgets::DigitisationWidget::MULTIPOINT:
				label = "gml:MultiPoint";
				break;
		case GPlatesQtWidgets::DigitisationWidget::POLYGON:
				label = "gml:Polygon";
				break;
		}
		
		// Override that default for particular edge cases.
		int children = item->childCount();
		if (children == 0) {
			label = "";
		} else if (children == 1) {
			label = "gml:Point";
		} else if (children == 2 && target_geom_type == GPlatesQtWidgets::DigitisationWidget::POLYGON) {
			label = "gml:LineString";
		}
		
		// Digitising a Polygon gives special meaning to the first entry.
		if (target_geom_type == GPlatesQtWidgets::DigitisationWidget::POLYGON) {
			if (position == 0) {
				label = QObject::tr("exterior: %1").arg(label);
			} else {
				label = QObject::tr("interior: %1").arg(label);
			}
		}
		
		return label;
	}


	/**
	 * Goes through the children of the QTreeWidgetItem geometry-item (i.e. the
	 * points in the table) and attempts to build a vector of PointOnSphere.
	 * 
	 * Invalid points in the table will be skipped over, although due to the nature
	 * of the DigitisationWidget, there really shouldn't be any invalid points to begin
	 * with, since we're getting them from a PointOnSphere in the first place.
	 */
	std::vector<GPlatesMaths::PointOnSphere>
	build_points_from_table_item(
			QTreeWidgetItem *geom_item)
	{
		std::vector<GPlatesMaths::PointOnSphere> points;
		int children = geom_item->childCount();
		points.reserve(children);
		
		// Build a vector of points that we can pass to PolylineOnSphere's validity test.
		for (int i = 0; i < children; ++i) {
			QTreeWidgetItem *child = geom_item->child(i);
			double lat = 0.0;
			double lon = 0.0;
			
			// Pull the lat,lon out of the QTreeWidgetItem that we stored inside it
			// using the Qt::EditRole. This avoids unnecessary parsing of text.
			QVariant lat_var = child->data(GPlatesQtWidgets::DigitisationWidget::COLUMN_LAT, Qt::EditRole);
			bool lat_ok = false;
			lat = lat_var.toDouble(&lat_ok);
			
			QVariant lon_var = child->data(GPlatesQtWidgets::DigitisationWidget::COLUMN_LON, Qt::EditRole);
			bool lon_ok = false;
			lon = lon_var.toDouble(&lon_ok);
			
			// (Attempt to) create a LatLonPoint for the coordinates.
			if (lat_ok && lon_ok) {
				// At this point we have a valid lat,lon - valid as far as doubles are concerned.
				try {
					points.push_back(GPlatesMaths::make_point_on_sphere(
							GPlatesMaths::LatLonPoint(lat,lon)));
				} catch (GPlatesMaths::InvalidLatLonException &) {
					// We really shouldn't be encountering invalid latlongs. How did the
					// user click them?
					throw;
				}
			} else {
				// If ! lat_ok || ! lon_ok, something is seriously wrong.
				// How did invalid data get in here? Throw an appropriate exception.
				if ( ! lat_ok ) {
					throw GPlatesMaths::InvalidLatLonCoordinateException(lat,
							GPlatesMaths::InvalidLatLonCoordinateException::LatitudeCoord, i);
				} else {
					throw GPlatesMaths::InvalidLatLonCoordinateException(lon,
							GPlatesMaths::InvalidLatLonCoordinateException::LongitudeCoord, i);
				}
			}
		}
		return points;
	}
	
	
	/**
	 * Enumerates all possible return values from GeometryOnSphere construction-parameter
	 * validation functions. This takes advantage of the fact that some invalid states
	 * (e.g. insufficient points) are common to several different GeometryOnSphere derivations.
	 * 
	 * The downside is that each create_*_on_sphere() function needs to map the type-specific
	 * validity states to this enumeration - but it should be checking the return value anyway.
	 */
	namespace GeometryConstruction
	{
		enum GeometryConstructionValidity
		{
			VALID,
			INVALID_INSUFFICIENT_POINTS,
			INVALID_ANTIPODAL_SEGMENT_ENDPOINTS
		};
	}


	/**
	 * Creates a single PointOnSphere (assuming >0 points are provided).
	 * If you supply more than one point, the others will get ignored.
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * GeometryOnSphere::non_null_ptr_to_const_type.
	 */
	geometry_opt_ptr_type
	create_point_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		if (points.size() > 0) {
			validity = GeometryConstruction::VALID;
			return points.front().clone_as_geometry();
		} else {
			validity = GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
			return boost::none;
		}
	}


	/**
	 * @a validity is a return-parameter. It will be set to
	 * GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * GeometryOnSphere::non_null_ptr_to_const_type.
	 */
	geometry_opt_ptr_type
	create_polyline_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		// Set up the return-parameter for the evaluate_construction_parameter_validity() function.
		std::pair<
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator, 
			std::vector<GPlatesMaths::PointOnSphere>::const_iterator>
				invalid_points;
		
		// Evaluate construction parameter validity, and create.
		polyline_type::ConstructionParameterValidity polygon_validity = 
				polyline_type::evaluate_construction_parameter_validity(points, invalid_points);

		// Create the polyline and return it - if we can.
		switch (polygon_validity)
		{
		case polyline_type::VALID:
			validity = GeometryConstruction::VALID;
			// Note that create_on_heap gives us a PolylineOnSphere::non_null_ptr_to_const_type,
			// while we are returning it as an optional GeometryOnSphere::non_null_ptr_to_const_type.
			return GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type(
					polyline_type::create_on_heap(points));
		
		case polyline_type::INVALID_INSUFFICIENT_DISTINCT_POINTS:
			validity = GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
			return boost::none;

		case polyline_type::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
			validity = GeometryConstruction::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS;
			return boost::none;
			
		default:
			// FIXME: Exception.
			qDebug() << "UNKNOWN/UNHANDLED polyline evaluate_construction_parameter_validity," << validity;
			return boost::none;
		}
	}


	/**
	 * FIXME: As we have no MultiPoint available, this one is rather simplistic;
	 * it just returns a single PointOnSphere (assuming >0 points are provided).
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * GeometryOnSphere::non_null_ptr_to_const_type.
	 */
	geometry_opt_ptr_type
	create_multipoint_on_sphere(
			const std::vector<GPlatesMaths::PointOnSphere> &points,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		return create_point_on_sphere(points, validity);
	}


	/**
	 * Does the work of examining QTreeWidgetItems and the user's intentions
	 * to call upon the appropriate anonymous namespace geometry creation function.
	 *
	 * @a validity is a return-parameter. It will be set to
	 * GeometryConstruction::VALID if everything went ok. In the event
	 * of construction problems occuring, it will indicate why construction
	 * failed.
	 *
	 * Note we are returning a possibly-none boost::optional of
	 * GeometryOnSphere::non_null_ptr_to_const_type.
	 */
	geometry_opt_ptr_type
	create_geometry_from_table_items(
			QTreeWidgetItem *geom_item,
			GPlatesQtWidgets::DigitisationWidget::GeometryType target_geom_type,
			GeometryConstruction::GeometryConstructionValidity &validity)
	{
		std::vector<GPlatesMaths::PointOnSphere> points;
		
		// FIXME: I think... we need some way to add data() to the 'header' QTWIs,
		// so that we can immediately discover which bits are supposed to be polygon exteriors etc.

		switch (target_geom_type)
		{
		default:
				// FIXME: Exception.
				qDebug() << "Unknown geometry type, not implemented yet!";
				return boost::none;
				
		case GPlatesQtWidgets::DigitisationWidget::POLYLINE:
				points = build_points_from_table_item(geom_item);

				// FIXME: I'd really like to wrap this up into a function pointer.
				if (points.size() == 0) {
					validity = GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
					return boost::none;
				} else if (points.size() == 1) {
					return create_point_on_sphere(points, validity);
				} else {
					return create_polyline_on_sphere(points, validity);
				}
				break;
				
		case GPlatesQtWidgets::DigitisationWidget::MULTIPOINT:
				points = build_points_from_table_item(geom_item);

				// FIXME: I'd really like to wrap this up into a function pointer,
				if (points.size() == 0) {
					validity = GeometryConstruction::INVALID_INSUFFICIENT_POINTS;
					return boost::none;
				} else if (points.size() == 1) {
					return create_point_on_sphere(points, validity);
				} else {
					return create_multipoint_on_sphere(points, validity);
				}
				break;
				
		case GPlatesQtWidgets::DigitisationWidget::POLYGON:
				qDebug() << "Polygon not implemented yet!";
				return boost::none;
		}
		// Should never reach here.
	}
	
}



GPlatesQtWidgets::DigitisationWidget::DigitisationWidget(
		GPlatesModel::ModelInterface &model_interface,
		ViewportWindow &view_state_,
		QWidget *parent_):
	QWidget(parent_),
	d_view_state_ptr(&view_state_),
	d_export_coordinates_dialog(new ExportCoordinatesDialog(this)),
	d_create_feature_dialog(new CreateFeatureDialog(model_interface, this)),
	d_geometry_type(GPlatesQtWidgets::DigitisationWidget::POLYLINE),
	d_geometry_opt_ptr(boost::none)
{
	setupUi(this);
	
	// Set up the header of the coordinates widget.
	treewidget_coordinates->header()->setResizeMode(QHeaderView::Stretch);
	
	// Clear button to clear points from table and start over.
	QObject::connect(button_clear_coordinates, SIGNAL(clicked()),
			this, SLOT(handle_clear()));
	
	// Export... button to open the Export Coordinates dialog.
	QObject::connect(button_export_coordinates, SIGNAL(clicked()),
			this, SLOT(handle_export()));
	
	// Create... button to open the Create Feature dialog.
	QObject::connect(button_create_feature, SIGNAL(clicked()),
			this, SLOT(handle_create()));	
	
	// Get everything else ready that may need to be set up more than once.
	initialise_geometry(POLYLINE);
}


void
GPlatesQtWidgets::DigitisationWidget::update_table_labels()
{
	// For each label (top-level QTreeWidgetItem) in the table,
	// determine what (piece of) geometry it will turn into when
	// the user hits Create.
	QTreeWidgetItem *root = treewidget_coordinates->invisibleRootItem();
	int num_children = root->childCount();
	for (int i = 0; i < num_children; ++i) {
		QTreeWidgetItem *geom_item = root->child(i);
		QString label = calculate_label_for_item(d_geometry_type, i, geom_item);
		geom_item->setText(0, label);
		
		// FIXME: Re-applying these properties has only become necessary with the
		// addition of the multi-geom aware DigitisationChangeGeometryType command.
		// Something needs to be done about this awkward situation.
		static const QBrush background(Qt::darkGray);
		static const QBrush foreground(Qt::white);
		geom_item->setBackground(0, background);
		geom_item->setForeground(0, foreground);
		geom_item->setFirstColumnSpanned(true);
		geom_item->setExpanded(true);
	}
}


void
GPlatesQtWidgets::DigitisationWidget::update_geometry()
{
	// Clear the old d_geometry_opt_ptr, so that if the table is empty we'll be left
	// with boost::none.
	d_geometry_opt_ptr = boost::none;
	
	// For each 'geometry' top level QTreeWidgetItem:
	QTreeWidgetItem *root = treewidget_coordinates->invisibleRootItem();
	int num_children = root->childCount();
	for (int i = 0; i < num_children; ++i) {
		//   build vector of pointonsphere from the lat,lon
				// std::vector<GPlatesMaths::PointOnSphere> build_points_from_table_item(*geom_item)
		//   feed that into a create_xxxx function
				// geometry_ptr_type create_polyline_on_sphere(std::vector<GPlatesMaths::PointOnSphere> &points,
				// GPlatesMaths::PolylineOnSphere::ConstructionParameterValidity& validity)
		QTreeWidgetItem *item = root->child(i);
		GeometryConstruction::GeometryConstructionValidity validity;
		geometry_opt_ptr_type geometry_opt_ptr = create_geometry_from_table_items(
				item, d_geometry_type, validity);

		// FIXME: it only handles single-linestring cases this way.
		// Set that as our new d_geometry_opt_ptr for now.
		d_geometry_opt_ptr = geometry_opt_ptr;
	}

	// Set that as our new d_geometry_opt_ptr, and render.
	if (d_geometry_opt_ptr) {
		// No problems, render the new GeometryOnSphere on screen.
		d_view_state_ptr->mouse_interaction_geometry_layer().clear();
		d_view_state_ptr->mouse_interaction_geometry_layer().push_back(*d_geometry_opt_ptr);
		d_view_state_ptr->reconstruct();
	} else {
		// Small problem - perhaps there are no valid points to make geometry from.
		// Clear any temporary rendered geometry.
		d_view_state_ptr->mouse_interaction_geometry_layer().clear();
		d_view_state_ptr->reconstruct();
	}
}


void
GPlatesQtWidgets::DigitisationWidget::clear_widget()
{
	treewidget_coordinates->clear();
	d_geometry_opt_ptr = boost::none;
}


void
GPlatesQtWidgets::DigitisationWidget::initialise_geometry(
		GeometryType geom_type)
{
	clear_widget();
	d_undo_stack.clear();
	d_geometry_type = geom_type;
}


void
GPlatesQtWidgets::DigitisationWidget::change_geometry_type(
		GeometryType geom_type)
{
	if (geom_type == d_geometry_type) {
		// Convert from one type of desired geometry to the exact same type.
		// i.e. do nothing.
		return;
	}
	
	undo_stack().push(new GPlatesUndoCommands::DigitisationChangeGeometryType(
			*this, d_geometry_type, geom_type));
}


void
GPlatesQtWidgets::DigitisationWidget::handle_create()
{
	// Feed the Create dialog the GeometryOnSphere you've set up for the current
	// points. You -have- set up a GeometryOnSphere for the current points,
	// haven't you?
	if (d_geometry_opt_ptr) {
		// Give a GeometryOnSphere::non_null_ptr_to_const_type to the Create dialog.
		bool success = d_create_feature_dialog->set_geometry_and_display(*d_geometry_opt_ptr);
		if ( ! success) {
			// The user cancelled the creation process. Return early and do not reset
			// the digitisation widget.
			return;
		}
	} else {
		QMessageBox::warning(this, tr("No geometry for feature"),
				tr("There is no valid geometry to use for creating a feature."),
				QMessageBox::Ok);
		return;
		// FIXME: Should it be possible to make geometryless features from a digitisation tool?
	}

	// FIXME: Undo! but that ties into the main 'model' QUndoStack.
	// So the simplest thing to do at this stage is clear the 'digitisation' undo stack.
	undo_stack().clear();
	
	// Then, when we're all done, reset the widget for new input.
	initialise_geometry(d_geometry_type);
}


void
GPlatesQtWidgets::DigitisationWidget::handle_clear()
{
	// Clear all geometry from the table.
	undo_stack().push(new GPlatesUndoCommands::DigitisationClearGeometry(*this));
}


void
GPlatesQtWidgets::DigitisationWidget::handle_export()
{
	// Feed the Export dialog the GeometryOnSphere you've set up for the current
	// points. You -have- set up a GeometryOnSphere for the current points,
	// haven't you?
	if (d_geometry_opt_ptr) {
		// Give a GeometryOnSphere::non_null_ptr_to_const_type to the Export dialog.
		d_export_coordinates_dialog->set_geometry_and_display(*d_geometry_opt_ptr);
	} else {
		QMessageBox::warning(this, tr("No geometry to export"),
				tr("There is no valid geometry to export."),
				QMessageBox::Ok);
	}
}


void
GPlatesQtWidgets::DigitisationWidget::append_point_to_geometry(
		double lat,
		double lon,
		QTreeWidgetItem *target_geometry_item)
{
	// Make a 'coordinate' QTreeWidgetItem, and add it to the last
	// 'geometry' top-level QTreeWidgetItem in our table.
	undo_stack().push(new GPlatesUndoCommands::DigitisationAddPoint(*this, lat, lon));
}


