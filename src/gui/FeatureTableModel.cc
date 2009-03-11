/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#include "FeatureTableModel.h"

#include <QApplication>
#include <QHeaderView>
#include <QString>
#include <QLocale>
#include <QFont>
#include <QFontMetrics>
#include <QDebug>
#include <boost/none.hpp>

#include "model/types.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"
#include "feature-visitors/PlateIdFinder.h"
#include "feature-visitors/GmlTimePeriodFinder.h"
#include "feature-visitors/XsStringFinder.h"
#include "feature-visitors/GeometryFinder.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GeoTimeInstant.h"
#include "utils/UnicodeStringUtils.h"
#include "maths/LatLonPointConversions.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "maths/ConstGeometryOnSphereVisitor.h"


#define NUM_ELEMS(a) (sizeof(a) / sizeof((a)[0]))


namespace
{
	typedef const QVariant (*table_cell_accessor_type)(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry);
	
	struct ColumnHeadingInfo
	{
		const char *label;
		const char *tooltip;
		int width;
		QHeaderView::ResizeMode resize_mode;
		table_cell_accessor_type accessor;
		QFlags<Qt::AlignmentFlag> alignment;
	};


	// Accessor functions for table cells:
	
	const QVariant
	null_table_accessor(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry)
	{
		return QVariant();
	}


	const boost::optional<GPlatesModel::FeatureHandle::weak_ref>
	get_feature_weak_ref_if_valid(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry)
	{
		// We use a dynamic cast here (despite the fact that dynamic casts are generally
		// considered bad form) because we only care about one specific derivation.
		// There's no "if ... else if ..." chain, so I think it's not super-bad form.  (The
		// "if ... else if ..." chain would imply that we should be using polymorphism --
		// specifically, the double-dispatch of the Visitor pattern -- rather than updating
		// the "if ... else if ..." chain each time a new derivation is added.)
		GPlatesModel::ReconstructedFeatureGeometry *rfg =
				dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(geometry.get());
		if (rfg) {
			// It's an RFG, so let's look at the feature it's referencing.
			if (rfg->is_valid()) {
				return boost::optional<GPlatesModel::FeatureHandle::weak_ref>(
						rfg->get_feature_ref());
			}
			// Else, the weak-ref is not valid, so we'll return boost::none instead.
			return boost::none;
		}
		// Else, it's not an RFG, so there's no associated feature.
		return boost::none;
	}


	const QVariant
	get_feature_type(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry)
	{
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref) {
			return QVariant(GPlatesUtils::make_qstring_from_icu_string(
					(*weak_ref)->feature_type().build_aliased_name()));
		}
		return QVariant();
	}

	
	const QVariant
	get_reconstruction_plate_id_from_properties(
			const GPlatesModel::FeatureHandle &feature,
			bool should_print_debugging_message = false)
	{
		static const GPlatesModel::PropertyName plate_id_property_name =
				GPlatesModel::PropertyName::create_gpml("reconstructionPlateId");

		GPlatesFeatureVisitors::PlateIdFinder plate_id_finder(plate_id_property_name);
		plate_id_finder.visit_feature_handle(feature);
		if (plate_id_finder.found_plate_ids_begin() != plate_id_finder.found_plate_ids_end()) {
			// The feature has a reconstruction plate ID.
			GPlatesModel::integer_plate_id_type recon_plate_id =
					*plate_id_finder.found_plate_ids_begin();

			if (should_print_debugging_message) {
				std::cerr << "Debug log: No cached reconstruction plate ID in RFG,\n"
						<< "but reconstruction plate ID found in feature." << std::endl;
			}
			return QVariant(static_cast<quint32>(recon_plate_id));
		} else {
			// The feature doesn't have a reconstruction plate ID.
			return QVariant();
		}
	}


	const QVariant
	get_plate_id(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry)
	{
		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref) {
			// We use a dynamic cast here (despite the fact that dynamic casts are
			// generally considered bad form) because we only care about one specific
			// derivation.  There's no "if ... else if ..." chain, so I think it's not
			// super-bad form.  (The "if ... else if ..." chain would imply that we
			// should be using polymorphism -- specifically, the double-dispatch of the
			// Visitor pattern -- rather than updating the "if ... else if ..." chain
			// each time a new derivation is added.)
			GPlatesModel::ReconstructedFeatureGeometry *rfg =
					dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(geometry.get());
			if (rfg) {
				// It's an RFG, so it might have a cached reconstruction plate ID.
				if (rfg->reconstruction_plate_id()) {
					// Yes, it has a cached reconstruction plate ID.
					return QVariant(static_cast<quint32>(*rfg->reconstruction_plate_id()));
				} else {
					// Otherwise, there wasn't a reconstruction plate ID.  Let's find
					// the reconstruction plate ID the hard way -- by iterating through
					// all the properties of the referenced feature.
					return get_reconstruction_plate_id_from_properties(**weak_ref, true);
				}
			}
		}
		return QVariant();
	}


	const QString
	format_time_instant(
			const GPlatesPropertyValues::GmlTimeInstant &time_instant)
	{
		QLocale locale;
		if (time_instant.time_position().is_real()) {
			return locale.toString(time_instant.time_position().value());
		} else if (time_instant.time_position().is_distant_past()) {
			return QObject::tr("past");
		} else if (time_instant.time_position().is_distant_future()) {
			return QObject::tr("future");
		} else {
			return QObject::tr("<invalid>");
		}
	}


	const QString
	format_time_period(
			const GPlatesPropertyValues::GmlTimePeriod &time_period)
	{
		return QObject::tr("%1 - %2")
				.arg(format_time_instant(*(time_period.begin())))
				.arg(format_time_instant(*(time_period.end())));
	}


	const QVariant
	get_time_begin(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry)
	{
		static const GPlatesModel::PropertyName valid_time_property_name =
				GPlatesModel::PropertyName::create_gml("validTime");

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref) {
			GPlatesFeatureVisitors::GmlTimePeriodFinder time_period_finder(valid_time_property_name);
			time_period_finder.visit_feature_handle(**weak_ref);
			if (time_period_finder.found_time_periods_begin() != time_period_finder.found_time_periods_end()) {
				// The feature has a gml:validTime property.
				// FIXME: This could be from a gpml:TimeVariantFeature, OR a gpml:InstantaneousFeature,
				// in the latter case it has a slightly different meaning and we should be displaying the
				// gpml:reconstructedTime property instead.
				GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period =
						*time_period_finder.found_time_periods_begin();
				return format_time_instant(*(time_period->begin()));
			}
		}
		return QVariant();
	}


	const QVariant
	get_time_end(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry)
	{
		static const GPlatesModel::PropertyName valid_time_property_name =
				GPlatesModel::PropertyName::create_gml("validTime");

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref) {
			GPlatesFeatureVisitors::GmlTimePeriodFinder time_period_finder(valid_time_property_name);
			time_period_finder.visit_feature_handle(**weak_ref);
			if (time_period_finder.found_time_periods_begin() != time_period_finder.found_time_periods_end()) {
				// The feature has a gml:validTime property.
				// FIXME: This could be from a gpml:TimeVariantFeature, OR a gpml:InstantaneousFeature,
				// in the latter case it has a slightly different meaning and we should be displaying the
				// gpml:reconstructedTime property instead.
				GPlatesPropertyValues::GmlTimePeriod::non_null_ptr_to_const_type time_period =
						*time_period_finder.found_time_periods_begin();
				return format_time_instant(*(time_period->end()));
			}
		}
		return QVariant();
	}


	const QVariant
	get_name(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry)
	{
		// FIXME: Need to adapt according to user's current codeSpace setting.
		static const GPlatesModel::PropertyName name_property_name = 
				GPlatesModel::PropertyName::create_gml("name");

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref) {
			GPlatesFeatureVisitors::XsStringFinder string_finder(name_property_name);
			string_finder.visit_feature_handle(**weak_ref);
			if (string_finder.found_strings_begin() != string_finder.found_strings_end()) {
				// The feature has one or more name properties.  Use the first one
				// for now.
				GPlatesPropertyValues::XsString::non_null_ptr_to_const_type name = 
						*string_finder.found_strings_begin();
				return GPlatesUtils::make_qstring(name->value());
			}
		}
		return QVariant();
	}


	const QVariant
	get_description(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry)
	{
		static const GPlatesModel::PropertyName description_property_name = 
				GPlatesModel::PropertyName::create_gml("description");

		boost::optional<GPlatesModel::FeatureHandle::weak_ref> weak_ref =
				get_feature_weak_ref_if_valid(geometry);
		if (weak_ref) {
			GPlatesFeatureVisitors::XsStringFinder string_finder(description_property_name);
			string_finder.visit_feature_handle(**weak_ref);
			if (string_finder.found_strings_begin() != string_finder.found_strings_end()) {
				// The feature has one or more description properties.  Use the
				// first one for now.
				GPlatesPropertyValues::XsString::non_null_ptr_to_const_type description =
						*string_finder.found_strings_begin();
				return GPlatesUtils::make_qstring(description->value());
			}
		}
		return QVariant();
	}


	const QString
	format_point_or_vertex(
			const GPlatesMaths::PointOnSphere &point_or_vertex)
	{
		GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(point_or_vertex);
		QLocale locale;
		QString str = QObject::tr("(%1 ; %2)")
				.arg(locale.toString(llp.latitude()))
				.arg(locale.toString(llp.longitude()));
		return str;
	}


	const QString
	format_geometry_point(
			GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point)
	{
		return QObject::tr("point: %1")
				.arg(format_point_or_vertex(*point));
	}


	const QString
	format_geometry_multi_point(
			GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point)
	{
		QString begin_str = format_point_or_vertex(multi_point->start_point());
		QString end_str = format_point_or_vertex(multi_point->end_point());
		QString middle_str;
		if (multi_point->number_of_points() == 3) {
			middle_str = QObject::tr("... 1 more vertex ... ");
		} else if (multi_point->number_of_points() > 3) {
			middle_str = QObject::tr("... %1 more vertices ... ")
					.arg(multi_point->number_of_points() - 2);
		}
		return QObject::tr("multi-point: %1 %3%2")
				.arg(begin_str)
				.arg(end_str)
				.arg(middle_str);
	}


	const QString
	format_geometry_polygon(
			GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon)
	{
		QString begin_str = format_point_or_vertex(polygon->first_vertex());
		QString end_str = format_point_or_vertex(polygon->last_vertex());
		QString middle_str;
		if (polygon->number_of_vertices() == 3) {
			middle_str = QObject::tr("... 1 more vertex ... ");
		} else if (polygon->number_of_vertices() > 3) {
			middle_str = QObject::tr("... %1 more vertices ... ")
					.arg(polygon->number_of_vertices() - 2);
		}
		return QObject::tr("polygon: %1 %3%2")
				.arg(begin_str)
				.arg(end_str)
				.arg(middle_str);
	}


	const QString
	format_geometry_polyline(
			GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline)
	{
		QString begin_str = format_point_or_vertex(polyline->start_point());
		QString end_str = format_point_or_vertex(polyline->end_point());
		QString middle_str;
		if (polyline->number_of_vertices() == 3) {
			middle_str = QObject::tr("... 1 more vertex ... ");
		} else if (polyline->number_of_vertices() > 3) {
			middle_str = QObject::tr("... %1 more vertices ... ")
					.arg(polyline->number_of_vertices() - 2);
		}
		return QObject::tr("polyline: %1 %3%2")
				.arg(begin_str)
				.arg(end_str)
				.arg(middle_str);
	}


	/**
	 * This is a Visitor to obtain a summary of the geometry-on-sphere as a string.
	 * 
	 * FIXME: This would be great in a separate file. The Query and Edit Feature
	 * dialogs could make use of it.
	 */
	class GeometryOnSphereSummaryAsStringVisitor:
			public GPlatesMaths::ConstGeometryOnSphereVisitor
	{
	public:
		GeometryOnSphereSummaryAsStringVisitor()
		{  }

		~GeometryOnSphereSummaryAsStringVisitor()
		{  }

		const QString &
		geometry_summary() const
		{
			return d_string;
		}

		// Please keep these geometries ordered alphabetically.

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_multi_point_on_sphere(
				GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type multi_point_on_sphere)
		{
			d_string = format_geometry_multi_point(multi_point_on_sphere);
		}

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_point_on_sphere(
				GPlatesMaths::PointOnSphere::non_null_ptr_to_const_type point_on_sphere)
		{
			d_string = format_geometry_point(point_on_sphere);
		}

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polygon_on_sphere(
				GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type polygon_on_sphere)
		{
			d_string = format_geometry_polygon(polygon_on_sphere);
		}

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit_polyline_on_sphere(
				GPlatesMaths::PolylineOnSphere::non_null_ptr_to_const_type polyline_on_sphere)
		{
			d_string = format_geometry_polyline(polyline_on_sphere);
		}

	private:
		QString d_string;
	};


	const boost::optional<GPlatesModel::FeatureHandle::properties_iterator>
	get_geometry_property_if_valid(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry)
	{
		// We use a dynamic cast here (despite the fact that dynamic casts are generally
		// considered bad form) because we only care about one specific derivation.
		// There's no "if ... else if ..." chain, so I think it's not super-bad form.  (The
		// "if ... else if ..." chain would imply that we should be using polymorphism --
		// specifically, the double-dispatch of the Visitor pattern -- rather than updating
		// the "if ... else if ..." chain each time a new derivation is added.)
		GPlatesModel::ReconstructedFeatureGeometry *rfg =
				dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(geometry.get());
		if (rfg) {
			// It's an RFG, so let's look at the property iterator of the geometry.
			if (rfg->property().is_valid()) {
				// Okay fine, but is the property NULL (i.e. deleted)?
				if (*rfg->property() != NULL) {
					return boost::optional<GPlatesModel::FeatureHandle::properties_iterator>(
							rfg->property());
				}
			}
			// Else, the iterator is not valid, so we'll return boost::none instead.
			return boost::none;
		}
		// Else, it's not an RFG, so there's no associated feature.
		return boost::none;
	}


	const QVariant
	get_present_day_geometry(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry)
	{
		boost::optional<GPlatesModel::FeatureHandle::properties_iterator> property =
				get_geometry_property_if_valid(geometry);
		if (property) {
			GPlatesFeatureVisitors::GeometryFinder geometry_finder;
			(**property)->accept_visitor(geometry_finder);
			if (geometry_finder.has_found_geometries()) {
				GeometryOnSphereSummaryAsStringVisitor geometry_visitor;
				(*geometry_finder.found_geometries_begin())->accept_visitor(geometry_visitor);
				return QVariant(geometry_visitor.geometry_summary());
			}
		}
		return QVariant();
	}


	const QVariant
	get_clicked_geometry_property(
			GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry)
	{
		boost::optional<GPlatesModel::FeatureHandle::properties_iterator> property =
				get_geometry_property_if_valid(geometry);
		if (property) {
			return QVariant(GPlatesUtils::make_qstring_from_icu_string(
					(**property)->property_name().build_aliased_name()));
		}
		return QVariant();
	}


	// The dispatch table for the above functions:
	
	static const ColumnHeadingInfo column_heading_info_table[] = {
		{ QT_TR_NOOP("Feature type"), QT_TR_NOOP("The type of this feature"),
				140, QHeaderView::ResizeToContents,
				get_feature_type, Qt::AlignLeft | Qt::AlignVCenter },

		{ QT_TR_NOOP("Plate ID"), QT_TR_NOOP("The plate ID used to reconstruct this feature"),
				60, QHeaderView::Fixed,
				get_plate_id, Qt::AlignCenter },

		{ QT_TR_NOOP("Name"), QT_TR_NOOP("A convenient label for this feature"),
				140, QHeaderView::ResizeToContents,
				get_name, Qt::AlignLeft | Qt::AlignVCenter },

		{ QT_TR_NOOP("Clicked geometry"), QT_TR_NOOP("The geometry which was clicked"),
				140, QHeaderView::ResizeToContents,
				get_clicked_geometry_property, Qt::AlignLeft | Qt::AlignVCenter },

		{ QT_TR_NOOP("Begin"), QT_TR_NOOP("The time of appearance (Ma)"),
				60, QHeaderView::Fixed,
				get_time_begin, Qt::AlignCenter },

		{ QT_TR_NOOP("End"), QT_TR_NOOP("The time of disappearance (Ma)"),
				60, QHeaderView::Fixed,
				get_time_end, Qt::AlignCenter }, 

		{ QT_TR_NOOP("Present-day geometry (lat ; lon)"), QT_TR_NOOP("A summary of the present-day coordinates"),
				240, QHeaderView::ResizeToContents,
				get_present_day_geometry, Qt::AlignCenter },
	};


	const QString
	get_column_heading(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return QObject::tr("");
		}
		return QObject::tr(column_heading_info_table[column].label);
	}

	const QString
	get_column_tooltip(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return QObject::tr("");
		}
		return QObject::tr(column_heading_info_table[column].tooltip);
	}

	int
	get_column_width(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return 0;
		}
		return column_heading_info_table[column].width;
	}

	table_cell_accessor_type
	get_column_accessor(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return null_table_accessor;
		}
		return column_heading_info_table[column].accessor;
	}

	QFlags<Qt::AlignmentFlag>
	get_column_alignment(
			int column)
	{
		if (column < 0 || column >= static_cast<int>(NUM_ELEMS(column_heading_info_table))) {
			return Qt::AlignLeft | Qt::AlignVCenter;
		}
		return column_heading_info_table[column].alignment;
	}
}



GPlatesGui::FeatureTableModel::FeatureTableModel(
		FeatureFocus &feature_focus,
		QObject *parent_):
	QAbstractTableModel(parent_),
	d_feature_focus_ptr(&feature_focus)
{
	QObject::connect(d_feature_focus_ptr,
			SIGNAL(focused_feature_modified(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)),
			this,
			SLOT(handle_feature_modified(GPlatesModel::FeatureHandle::weak_ref,
					GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)));
}



int
GPlatesGui::FeatureTableModel::rowCount(
		const QModelIndex &parent_) const
{
	return static_cast<int>(d_sequence.size());
}

int
GPlatesGui::FeatureTableModel::columnCount(
		const QModelIndex &parent_) const
{
	return NUM_ELEMS(column_heading_info_table);
}

Qt::ItemFlags
GPlatesGui::FeatureTableModel::flags(
		const QModelIndex &idx) const
{
	return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
}

QVariant
GPlatesGui::FeatureTableModel::headerData(
		int section,
		Qt::Orientation orientation,
		int role) const
{
	// The new way we are attempting to return an appropriate vertical
	// and horizontal size in the Qt::SizeHintRole for the header!
	QFontMetrics fm = QApplication::fontMetrics();
	
#if 0
	qDebug() << "\nFONT METRICS DEBUGGING:";
	qDebug() << "QApplication::style() == " << QApplication::style()->metaObject()->className();
	qDebug() << "QApplication::font().toString() == " << QApplication::font().toString();
	qDebug() << "QLocale().name() == " << QLocale().name();
	qDebug() << "fm.ascent() == " << fm.ascent();
	qDebug() << "fm.descent() == " << fm.descent();
	qDebug() << "fm.boundingRect(Q) == " << fm.boundingRect('Q');
	qDebug() << "fm.boundingRect(y) == " << fm.boundingRect('y');
	qDebug() << "fm.boundingRect(QylLj!|[]`~_) == " << fm.boundingRect("QylLj!|[]`~_");
	qDebug() << "fm.height() == " << fm.height();
	qDebug() << "fm.lineSpacing() == " << fm.lineSpacing();
	qDebug() << "fm.leading() == " << fm.leading();
#endif
	
	// We are only interested in modifying the horizontal header.
	if (orientation == Qt::Horizontal) {
		// No need to supply tooltip data, etc. for headers. We are only interested in a
		// few roles.

		if (role == Qt::DisplayRole) {
			return get_column_heading(section);

		} else if (role == Qt::ToolTipRole) {
			return get_column_tooltip(section);
			
		} else if (role == Qt::SizeHintRole) {
			// Annoyingly, the metrics alone do not appear to be sufficient to supply
			// the height of the header, so a few pixels are added.
			//
			// Furthermore, for some reason, the height is now being set correctly but
			// the width does not updated until a call to
			// QTableView::resizeColumnsToContents();
			return QSize(get_column_width(section), fm.height()+4);
			
		} else {
			return QVariant();
		}
		
	} else {
		// Vertical header; ignore.
		return QVariant();
	}
}


QVariant
GPlatesGui::FeatureTableModel::data(
		const QModelIndex &idx,
		int role) const
{
	if ( ! idx.isValid()) {
		return QVariant();
	}
	if (idx.row() < 0 || idx.row() >= rowCount()) {
		return QVariant();
	}
	
	if (role == Qt::DisplayRole) {
		GPlatesModel::ReconstructionGeometry::non_null_ptr_type geometry =
				d_sequence.at(idx.row());

		// Cell contents is returned via call to column-specific dispatch function.
		table_cell_accessor_type accessor = get_column_accessor(idx.column());
		return (*accessor)(geometry);
	} else if (role == Qt::TextAlignmentRole) {
		return QVariant(get_column_alignment(idx.column()));
	}
	return QVariant();
}


void
GPlatesGui::FeatureTableModel::handle_selection_change(
		const QItemSelection &selected,
		const QItemSelection &deselected)
{
	if (selected.indexes().isEmpty()) {
		d_feature_focus_ptr->unset_focus();
		return;
	}
	// We assume that the view has been constrained to allow only single-row selections,
	// so only concern ourselves with the first index in the list.
	QModelIndex idx = selected.indexes().first();
	if ( ! idx.isValid()) {
		return;
	}
	GPlatesModel::ReconstructionGeometry *rg = d_sequence.at(idx.row()).get();

	// We use a dynamic cast here (despite the fact that dynamic casts are generally
	// considered bad form) because we only care about one specific derivation.
	// There's no "if ... else if ..." chain, so I think it's not super-bad form.  (The
	// "if ... else if ..." chain would imply that we should be using polymorphism --
	// specifically, the double-dispatch of the Visitor pattern -- rather than updating
	// the "if ... else if ..." chain each time a new derivation is added.)
	GPlatesModel::ReconstructedFeatureGeometry *rfg =
			dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(rg);
	if (rfg) {
		// It's an RFG, so let's look at the feature it's referencing.
		if ( ! rfg->is_valid()) {
			return;
		}

		// When the user clicks a line of the table, we change the currently focused
		// feature.
		//
		// FIXME: If we end up using this class elsewhere, e.g. search results, we may
		// want to re-evaluate this behaviour.
		d_feature_focus_ptr->set_focus(rfg->get_feature_ref(), rfg);
	}
}


void
GPlatesGui::FeatureTableModel::handle_feature_modified(
		GPlatesModel::FeatureHandle::weak_ref modified_feature_ref,
		GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type)
{
	// First, figure out which row(s) of the table (if any) contains the modified feature
	// weak-ref.  Note that, since each row of the table corresponds to a single geometry
	// rather than a single feature, there might be multiple rows which match this feature.
	int row = 0;
	geometry_sequence_type::const_iterator it = d_sequence.begin();
	geometry_sequence_type::const_iterator end = d_sequence.end();
	for ( ; it != end; ++it, ++row) {
		GPlatesModel::ReconstructionGeometry *rg = it->get();

		// We use a dynamic cast here (despite the fact that dynamic casts are generally
		// considered bad form) because we only care about one specific derivation.
		// There's no "if ... else if ..." chain, so I think it's not super-bad form.  (The
		// "if ... else if ..." chain would imply that we should be using polymorphism --
		// specifically, the double-dispatch of the Visitor pattern -- rather than updating
		// the "if ... else if ..." chain each time a new derivation is added.)
		GPlatesModel::ReconstructedFeatureGeometry *rfg =
				dynamic_cast<GPlatesModel::ReconstructedFeatureGeometry *>(rg);
		if (rfg) {
			// It's an RFG, so let's look at the feature it's referencing.
			if (rfg->get_feature_ref() == modified_feature_ref) {
				QModelIndex idx_begin = index(row, 0);
				QModelIndex idx_end = index(row, NUM_ELEMS(column_heading_info_table) - 1);
				emit dataChanged(idx_begin, idx_end);
			}
		}
		// Else, it's not an RFG, so it doesn't reference a feature.
	}
}


void
GPlatesGui::FeatureTableModel::set_default_resize_modes(
		QHeaderView &header)
{
	for (int column = 0; column < static_cast<int>(NUM_ELEMS(column_heading_info_table)); ++column) {
		header.setResizeMode(column, column_heading_info_table[column].resize_mode);
	}
}


#if 0
QModelIndex
GPlatesGui::FeatureTableModel::get_index_for_feature(
		GPlatesModel::FeatureHandle::weak_ref feature_ref)
{
	// Figure out which row of the table (if any) contains the target feature.
	FeatureWeakRefSequence::const_iterator it = d_sequence_ptr->begin();
	FeatureWeakRefSequence::const_iterator end = d_sequence_ptr->end();
	int row = 0;
	for ( ; it != end; ++it, ++row) {
		if (*it == feature_ref) {
			QModelIndex idx = index(row, 0);
			return idx;
		}
	}
	
	// No such feature exists in our table, return an invalid index.
	return QModelIndex();
}
#endif
