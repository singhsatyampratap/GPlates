/* $Id: EditTimeSequenceWidget.cc 8310 2010-05-06 15:02:23Z rwatson $ */

/**
 * \file 
 * $Revision: 8310 $
 * $Date: 2010-05-06 17:02:23 +0200 (to, 06 mai 2010) $ 
 * 
 * Copyright (C) 2011 Geological Survey of Norway
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
#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QStandardItemModel>

#include "app-logic/TRSUtils.h"
#include "feature-visitors/TotalReconstructionSequencePlateIdFinder.h"
#include "feature-visitors/TotalReconstructionSequenceTimePeriodFinder.h"
#include "feature-visitors/PropertyValueFinder.h"
#include "model/FeatureHandle.h"
#include "model/FeatureVisitor.h"
#include "model/ModelUtils.h"
#include "model/TopLevelProperty.h"
#include "model/TopLevelPropertyInline.h"
#include "property-values/GmlTimeInstant.h"
#include "property-values/GpmlIrregularSampling.h"
#include "property-values/GpmlFiniteRotation.h"
#include "property-values/GpmlTimeSample.h"

#include "TotalReconstructionSequencesDialog.h"
#include "EditTableActionWidget.h"
#include "EditTotalReconstructionSequenceWidget.h"

namespace ColumnNames
{
	enum ColumnName
	{
		TIME = 0,
		LATITUDE,
		LONGITUDE,
		ANGLE,
		COMMENT,
		ACTIONS,

		NUMCOLS // Should always be last.
	};
}

namespace
{

	/**
	 * Borrowed from the TopologySectionsTable. Will try using it for itemChanged here. 
	 *
	 * Tiny convenience class to help suppress the @a QTableWidget::cellChanged()
	 * notification in situations where we are updating the table data
	 * programatically. This allows @a react_cell_changed() to differentiate
	 * between changes made by us, and changes made by the user.
	 *
	 * For it to work properly, you must declare one in any TopologySectionsTable
	 * method that directly mucks with table cell data.
	 */
	struct TableUpdateGuard :
			public boost::noncopyable
	{
		TableUpdateGuard(
				bool &guard_flag_ref):
			d_guard_flag_ptr(&guard_flag_ref)
		{
			// Nesting these guards is an error.
			Q_ASSERT(*d_guard_flag_ptr == false);
			*d_guard_flag_ptr = true;
		}
		
		~TableUpdateGuard()
		{
			*d_guard_flag_ptr = false;
		}
		
		bool *d_guard_flag_ptr;
	};

	/**
	 * Creates an irregular sampling property from the values in @a table.                                                             
	 */
	GPlatesModel::TopLevelProperty::non_null_ptr_type
	make_irregular_sampling_from_table(
		QTableWidget *table)
	{
		static QLocale locale_;

		std::vector<GPlatesModel::ModelUtils::TotalReconstructionPole> poles_data;
		
		for (int i = 0; i < table->rowCount(); ++i)
		{
                    static QString indet_string = QObject::tr("indet");
                        // FIXME: handle bad "ok"s
			bool ok;
			double time = locale_.toDouble(table->item(i,ColumnNames::TIME)->text(),&ok);

                        QString lat_string = table->item(i,ColumnNames::LATITUDE)->text();
                        QString lon_string = table->item(i,ColumnNames::LONGITUDE)->text();
                        double lat,lon;
                        if (lat_string == indet_string)
                        {
                            lat = 0.;
                        }
                        else
                        {
                            lat = locale_.toDouble(lat_string,&ok);
                        }
                        if (lon_string == indet_string)
                        {
                            lon = 0.;
                        }
                        else
                        {
                            lon = locale_.toDouble(lon_string,&ok);
                        }
			double angle = locale_.toDouble(table->item(i,ColumnNames::ANGLE)->text(),&ok);
			QString comment = table->item(i,ColumnNames::COMMENT)->text();
                        GPlatesModel::ModelUtils::TotalReconstructionPole pole_data = {
				time,lat,lon,angle,comment};
			poles_data.push_back(pole_data);
		}

		return GPlatesModel::ModelUtils::create_total_reconstruction_pole(poles_data);
	}

	void
	fill_table_with_comment(
		QTableWidget *table,
		unsigned int row_count,
		const QString &comment)
	{
		QTableWidgetItem *comment_item = new QTableWidgetItem;
		comment_item->setText(comment);

		comment_item->setFlags(comment_item->flags() | Qt::ItemIsEditable);
		table->setItem(row_count,ColumnNames::COMMENT,comment_item);
	}

	void
	fill_table_with_finite_rotation(
		QTableWidget *table,
		unsigned int row_count,
		const GPlatesPropertyValues::GpmlFiniteRotation &finite_rotation,
		const QLocale &locale_)
	{
		QTableWidgetItem *lat_item = new QTableWidgetItem();
		QTableWidgetItem *lon_item = new QTableWidgetItem();
		QTableWidgetItem *angle_item = new QTableWidgetItem();

		const GPlatesMaths::FiniteRotation &fr = finite_rotation.finite_rotation();
		const GPlatesMaths::UnitQuaternion3D &uq = fr.unit_quat();
		if (GPlatesMaths::represents_identity_rotation(uq)) {
			// It's an identity rotation (ie, a rotation of angle == 0.0), so there's
			// no determinate axis of rotation.
			static const double zero_angle = 0.0;

			// Assume that this string won't change after the first time this function
			// is called, so we can keep the QString in a static local var.
			static QString indeterm_tr_str =
				QObject::tr(
				"indet");

			lat_item->setText(indeterm_tr_str);
			lon_item->setText(indeterm_tr_str);
			angle_item->setText(locale_.toString(zero_angle));
		} else {
			// There is a well-defined axis of rotation and a non-zero angle.
			using namespace GPlatesMaths;

			UnitQuaternion3D::RotationParams params = uq.get_rotation_params(fr.axis_hint());
			PointOnSphere euler_pole(params.axis);
			LatLonPoint llp = make_lat_lon_point(euler_pole);
			double angle = convert_rad_to_deg(params.angle).dval();

			lat_item->setText(locale_.toString(llp.latitude()));
			lon_item->setText(locale_.toString(llp.longitude()));
			angle_item->setText(locale_.toString(angle));
		}

		lat_item->setFlags(lat_item->flags() | Qt::ItemIsEditable);
		lon_item->setFlags(lon_item->flags() | Qt::ItemIsEditable);
		angle_item->setFlags(angle_item->flags() | Qt::ItemIsEditable);

		table->setItem(row_count,ColumnNames::LATITUDE,lat_item);
		table->setItem(row_count,ColumnNames::LONGITUDE,lon_item);			
		table->setItem(row_count,ColumnNames::ANGLE,angle_item);
	}

	void
	fill_table_with_pole(
		QTableWidget *table,
		unsigned int row_count,
		const GPlatesModel::PropertyValue::non_null_ptr_to_const_type &time_sample_value,
		const QLocale &locale_)
	{
		using namespace GPlatesPropertyValues;

		const GpmlFiniteRotation *finite_rotation =
			dynamic_cast<const GpmlFiniteRotation *>(time_sample_value.get());
		if (finite_rotation) {
			// OK, so we definitely have a FiniteRotation.  Now we have to determine
			// whether it's an identity rotation or a rotation with a well-defined axis.
			fill_table_with_finite_rotation(table, row_count, *finite_rotation, locale_);
		} else {
			// The value of the TimeSample was NOT a FiniteRotation as it should
			// have been.  Hence, we can only display an error message in place of
			// the rotation.
			// Assume that this string won't change after the first time this function
			// is called, so we can keep the QString in a static local var.
			static QString not_found =
				QObject::tr(
				"x");
			QTableWidgetItem *lat_item = new QTableWidgetItem();
			QTableWidgetItem *lon_item = new QTableWidgetItem();
			QTableWidgetItem *angle_item = new QTableWidgetItem();
			lat_item->setText(not_found);
			lon_item->setText(not_found);
			angle_item->setText(not_found);
			
			table->setItem(row_count,ColumnNames::LATITUDE,lat_item);
			table->setItem(row_count,ColumnNames::LONGITUDE,lon_item);			
			table->setItem(row_count,ColumnNames::ANGLE,angle_item);
		}
	}

	void
	fill_table_with_time_instant(
		QTableWidget *table,
		unsigned int row_count,
		const GPlatesPropertyValues::GeoTimeInstant &geo_time_instant,
		const QLocale &locale_)
	{
		QTableWidgetItem *item = new QTableWidgetItem();
		if (geo_time_instant.is_real()) {
			// Use setData here so that the table can be sorted numerically by the time column. 
			item->setData(Qt::DisplayRole,geo_time_instant.value());
		} else {
			// This is a string to display if the geo-time instant is in either
			// the distant past or distant future (which it should not be).
			// Assume that this string won't change after the first time this function
			// is called, so we can keep the QString in a static local var.
			static QString invalid_time =
				QObject::tr(
				"invalid time");
			item->setData(Qt::DisplayRole,invalid_time);

		}
		table->setItem(row_count,ColumnNames::TIME,item);
	}

	/**
	 * Fill row @a row_count in the QTableWidget @a table with the time,lat,lon,angle and comment from the
	 * GpmlTimeSample @a time_sample.
	 */
	void
	insert_table_row(
		QTableWidget *table,
		unsigned int row_count,
		const GPlatesPropertyValues::GpmlTimeSample &time_sample,
		const QLocale &locale_)
	{
		table->insertRow(row_count);
		fill_table_with_time_instant(table,row_count,time_sample.valid_time()->time_position(),locale_);
		
		fill_table_with_pole(table,row_count,time_sample.value(),locale_);

		QString comment;
		if (time_sample.description())
		{
			comment = GPlatesUtils::make_qstring_from_icu_string(
				time_sample.description()->value().get());
		}
		fill_table_with_comment(table, row_count, comment);

	}

	GPlatesPropertyValues::GpmlTimeSample
	create_time_sample_property_from_row(
		double time, double lat, double lon, double angle, const QString &comment)
	{
		// Should we check the validity of any entered values here, or later when we try to commit to the model?

		using namespace GPlatesModel;
		using namespace GPlatesPropertyValues;

		// Attempt to create a valid time
		std::map<XmlAttributeName, XmlAttributeValue> xml_attributes;
		XmlAttributeName xml_attribute_name = XmlAttributeName::create_gml("frame");
		XmlAttributeValue xml_attribute_value("http://gplates.org/TRS/flat");
		xml_attributes.insert(std::make_pair(xml_attribute_name, xml_attribute_value));

		GeoTimeInstant geo_time_instant(time);
		GmlTimeInstant::non_null_ptr_type gml_time_instant =
			GmlTimeInstant::create(geo_time_instant, xml_attributes);


		// Note the (lon,lat) ordering in the pair.
		PropertyValue::non_null_ptr_type value =
			GpmlFiniteRotation::create(std::make_pair(lon,lat),angle);

		boost::intrusive_ptr<XsString> description =
			XsString::create(GPlatesUtils::make_icu_string_from_qstring(comment)).get();
		TemplateTypeParameterType value_type =
			TemplateTypeParameterType::create_gpml("FiniteRotation");
		return GpmlTimeSample(value, gml_time_instant, description, value_type);
	}

	/**
	 * Set appropriate limits for the spinbox according to its column - e.g. -90 to 90 for latitude.                                                                     
	 */
	void
	set_spinbox_properties(
		QDoubleSpinBox *spinbox,
		int column)
	{
		switch(column){
			case ColumnNames::TIME:
				spinbox->setMinimum(0.);
				spinbox->setMaximum(1000.);
				break;
			case ColumnNames::LATITUDE:
				spinbox->setMinimum(-90.);
				spinbox->setMaximum(90.);
				break;
			case ColumnNames::LONGITUDE:
				spinbox->setMinimum(-360.);
				spinbox->setMaximum(360.);
				break;
			case ColumnNames::ANGLE:
				spinbox->setMinimum(-360.);
				spinbox->setMaximum(360.);
				break;
		}
        spinbox->setDecimals(4);
	}

	/**
	 * Commit any spinbox widget value from the most recently spinbox-ified cell to the table.                                                                    
	 */
	void
	update_table_from_last_active_cell(
		QTableWidget *table)
	{
		int row = table->currentRow();
		int column = table->currentColumn();
		
		if ((column >= ColumnNames::TIME) && (column <= ColumnNames::ANGLE))
		{
			QWidget *widget = table->cellWidget(row,column);
			if (widget)
			{
				QTableWidgetItem *item = new QTableWidgetItem();
				QVariant variant = static_cast<QDoubleSpinBox*>(widget)->value();
				item->setData(Qt::DisplayRole,variant.toDouble());
				table->setItem(row,column,item);
			}

		}
	}

	void
	fill_row_with_defaults(
		QTableWidget *table,
		int row)
	{

		QTableWidgetItem *time_item = new QTableWidgetItem();
		time_item->setData(Qt::DisplayRole,0);

		QTableWidgetItem *lat_item = new QTableWidgetItem();
		lat_item->setData(Qt::DisplayRole,0);

		QTableWidgetItem *lon_item = new QTableWidgetItem();
		lon_item->setData(Qt::DisplayRole,0);

		QTableWidgetItem *angle_item = new QTableWidgetItem();
		angle_item->setData(Qt::DisplayRole,0);

		QTableWidgetItem *comment_item = new QTableWidgetItem();
		comment_item->setText(QString());

		table->setItem(row,ColumnNames::TIME,time_item);
		table->setItem(row,ColumnNames::LATITUDE,lat_item);
		table->setItem(row,ColumnNames::LONGITUDE,lon_item);
		table->setItem(row,ColumnNames::ANGLE,angle_item);
		table->setItem(row,ColumnNames::COMMENT,comment_item);
	}

	/**
	 * Returns true if the time values (i.e. values in ColumnNames::Time of @a table):
	 *   1) are not empty AND
	 *   2) do not contain duplicate times.
	 */ 
	bool
	table_times_are_valid(
		QTableWidget *table)
	{
		std::vector<double> times;
		for (int i=0; i < table->rowCount(); ++i)
		{
			bool ok;
			QTableWidgetItem *item = table->item(i,ColumnNames::TIME);
			if (!item)
			{
				continue;
			}

			// The item text should have been derived from a spinbox, but check we
			// have a double anyway.
			double time = table->item(i,ColumnNames::TIME)->text().toDouble(&ok);
			if (!ok)
			{
				return false;
			}
			if (std::find(times.begin(),times.end(),time) != times.end())
			{
				return false;
			}
			times.push_back(time);
		}
		return (!times.empty());
	}

	/**
	 * Changes any of the lat/lon fields in row @a row to "indet" if their corresponding angle field is zero.                                                                     
	 */
	void
	set_indeterminate_fields_for_row(
		QTableWidget *table,
		int row)
	{
		// Make sure we have a valid QTableWidgetItem first.
		QTableWidgetItem *item = table->item(row,ColumnNames::ANGLE);
		if (!item)
		{
			return;
		}
		double angle = table->item(row,ColumnNames::ANGLE)->text().toDouble();
		if (GPlatesMaths::are_almost_exactly_equal(angle,0.))
		{
			QTableWidgetItem *indet_lat_item = new QTableWidgetItem();
			indet_lat_item->setText(QObject::tr("indet"));

			QTableWidgetItem *indet_lon_item = new QTableWidgetItem();
			indet_lon_item->setText(QObject::tr("indet"));

			table->setItem(row,ColumnNames::LATITUDE,indet_lat_item);
			table->setItem(row,ColumnNames::LONGITUDE,indet_lon_item);			
		}
	}

	/**
	 * Changes any of the lat/lon fields in @a table to "indet" if their corresponding angle fields are zero.                                                                     
	 */
	void
	set_indeterminate_fields_for_table(
		QTableWidget *table)
	{
		for (int i = 0; i < table->rowCount() ; ++i)
		{
			set_indeterminate_fields_for_row(table,i);
		}
	}


}



GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::EditTotalReconstructionSequenceWidget(
	QWidget *parent_):
		QWidget(parent_),
		EditTableWidget(),
		d_suppress_update_notification_guard(false),
		d_spinbox_row(0),
		d_spinbox_column(0),
		d_moving_plate_changed(false),
		d_fixed_plate_changed(false)
{
	setupUi(this);

	// For setting minimum sizes.
	EditTableActionWidget dummy(this,NULL);
	table_sequences->horizontalHeader()->setResizeMode(ColumnNames::COMMENT,QHeaderView::Stretch);
	table_sequences->horizontalHeader()->setResizeMode(ColumnNames::ACTIONS,QHeaderView::Fixed);
	table_sequences->horizontalHeader()->resizeSection(ColumnNames::ACTIONS,dummy.width());
	table_sequences->verticalHeader()->setDefaultSectionSize(dummy.height());

	// FIXME: In addition to any text in label_validation, 
	// consider displaying some kind of warning icon as well. 
	label_validation->setText("");

	// Experiment with signals from cells. 
	// FIXME: remember to remove any experimental / unneeded signal connections. 
	QObject::connect(
		table_sequences,SIGNAL(itemChanged(QTableWidgetItem*)),this,SLOT(handle_item_changed(QTableWidgetItem*)));
	QObject::connect(
		button_insert,SIGNAL(pressed()),this,SLOT(handle_insert_new_pole()));
	QObject::connect(
		table_sequences,SIGNAL(currentCellChanged(int,int,int,int)),
		this,SLOT(handle_current_cell_changed(int,int,int,int)));
	QObject::connect(
		spinbox_moving,SIGNAL(valueChanged(int)),
		this,SLOT(handle_plate_ids_changed()));


	QObject::connect(
		spinbox_fixed,SIGNAL(valueChanged(int)),
		this,SLOT(handle_plate_ids_changed()));

	table_sequences->setRowCount(0);

	
	label_validation->setStyleSheet("QLabel {color: red;}");
}


void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::update_table_widget_from_property(
	GPlatesPropertyValues::GpmlIrregularSampling::non_null_ptr_type irreg_sampling)
{
	TableUpdateGuard guard(d_suppress_update_notification_guard);

	// We use this to express floating-point values (the TimeSample time positions)
	// in the correct format for this locale.
	QLocale locale_;

	using namespace GPlatesPropertyValues;

	// Note that this is clearContents() and not clear() - calling clear() will also clear the header text (which has
	// been set up in QtDesigner) resulting in only numerical headers appearing. 
	table_sequences->clearContents();

	std::vector<GpmlTimeSample>::const_iterator iter =
		irreg_sampling->time_samples().begin();
	std::vector<GpmlTimeSample>::const_iterator end =
		irreg_sampling->time_samples().end();

	table_sequences->setRowCount(0);
	unsigned int row_count = 0;

	for ( ; iter != end; ++iter, ++row_count) {
		insert_table_row(table_sequences,row_count,*iter,locale_);
	}
	table_sequences->setRowCount(row_count);

    set_indeterminate_fields_for_table(table_sequences);
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_item_changed(
	QTableWidgetItem *item)
{
	if (d_suppress_update_notification_guard)
	{
		return;
	}

    validate();
}


void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_insert_row_above(
	const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		insert_blank_row(row);
	}
	validate();
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_insert_row_below(
	const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		insert_blank_row(row+1);
	}
	validate();
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_delete_row(
	const EditTableActionWidget *action_widget)
{
	int row = get_row_for_action_widget(action_widget);
	if (row >= 0) {
		delete_row(row);
		set_action_widget_in_row(row);
	}
	validate();
}

GPlatesModel::TopLevelProperty::non_null_ptr_type
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::get_irregular_sampling_property_value_from_table_widget()
{
	update_table_from_last_active_cell(table_sequences);
	return make_irregular_sampling_from_table(table_sequences);
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_insert_new_pole()
{
	using namespace GPlatesPropertyValues;
	using namespace GPlatesModel;

    double time = spinbox_time->value();
	double lat = spinbox_lat->value();
	double lon = spinbox_lon->value();
	double angle = spinbox_angle->value();
	QString comment = lineedit_comment->text();

	static QLocale locale_;

	GpmlTimeSample time_sample = create_time_sample_property_from_row(time,lat,lon,angle,comment);

	insert_table_row(table_sequences,table_sequences->rowCount(),time_sample,locale_);
	if (table_sequences->rowCount() > 0)
	{
		set_action_widget_in_row(table_sequences->rowCount()-1);
	}

	table_sequences->sortItems(ColumnNames::TIME);
    set_indeterminate_fields_for_table(table_sequences);
	validate();
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_current_cell_changed(
		int current_row, int current_column, int previous_row, int previous_column)
{

	static QLocale locale_;

	// Move the action widget to the current row.
	if ((current_row != previous_row) && current_row >=0)
	{
		set_action_widget_in_row(current_row);
	}

	// Remove the spinbox from the previous cell. The value from the previous cell should
	// have been committed to the table in the "editingFinished()" method.
	if ((previous_column >= ColumnNames::TIME) && (previous_column <= ColumnNames::ANGLE))
	{
		table_sequences->removeCellWidget(previous_row,previous_column);
	}

	// Put a new spinbox in the current cell, and set it up with the value in the cell. 
	// The table will take ownership of the spinbox widget.
	if ((current_column >= ColumnNames::TIME) && (current_column <= ColumnNames::ANGLE))
	{
		QDoubleSpinBox *spinbox = new QDoubleSpinBox();
		table_sequences->setCellWidget(current_row,current_column,spinbox);
		d_spinbox_column = current_column;
		d_spinbox_row = current_row;
		QTableWidgetItem *current_item = table_sequences->item(current_row,current_column);
		if (current_item)
		{
			bool ok;
			double cell_value;
			current_item->data(Qt::DisplayRole).toDouble(&ok);
			if (ok)
			{
				cell_value = (current_item->data(Qt::DisplayRole)).toDouble();
			}
			else
			{
				cell_value = 0.;
			}

			set_spinbox_properties(spinbox,current_column);
			spinbox->setValue(cell_value);

			QObject::connect(spinbox,SIGNAL(editingFinished()),
				this,SLOT(handle_editing_finished()));
        }
	}

}

int
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::get_row_for_action_widget(
	const EditTableActionWidget *action_widget)
{
	for (int i = 0; i < table_sequences->rowCount(); ++i)
	{
		if (table_sequences->cellWidget(i, ColumnNames::ACTIONS) == action_widget) {
			return i;
		}
	}
	return -1;
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::insert_blank_row(
	int row)
{
	// Insert a new blank row.
	table_sequences->insertRow(row);

	fill_row_with_defaults(table_sequences,row);

	// Not yet sure if the work-around used in other Edit... widgets which involve tables 
	// is necessary in the EditTRSWidget....

	// Open up an editor for the first time field.
	QTableWidgetItem *time_item = table_sequences->item(row, ColumnNames::TIME);
	if (time_item != NULL) {
		table_sequences->setCurrentItem(time_item);
		table_sequences->editItem(time_item);
	}
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::delete_row(
	int row)
{
	// Before we delete the row, delete the action widget. removeRow() messes with the previous/current
	// row indices, and then calls handle_current_cell_changed, which cannot delete the old action widget, 
	// the upshot being that we end up with a surplus action widget which we can't get rid of. 
	table_sequences->removeCellWidget(row,ColumnNames::ACTIONS);
	// Delete the given row.
	table_sequences->removeRow(row);

	// May need the glitch work-around here too.
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::sort_table_by_time()
{
	update_table_from_last_active_cell(table_sequences);
	table_sequences->sortItems(ColumnNames::TIME);
}

bool
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::validate()
{
	// Until we have a mechanism for enabling/disabling poles and sequences, disallow editing/creation of
	// sequences with 999 plate-ids. 

    bool times_valid = false;
    bool plates_valid = false;

	if (table_sequences->rowCount() == 0)
	{
        times_valid = false;
		label_validation->setText(QObject::tr("No values in table.\nTo delete a sequence use the Delete Sequence button in the Total Reconstruction Seqeunce Dialog."));
	}
	else
	{
        times_valid = table_times_are_valid(table_sequences);
        if (times_valid)
		{
			label_validation->setText("");
		}
		else
		{
			label_validation->setText(QObject::tr("Table contains samples with equal time instants."));
		}
	}
    plates_valid = ((spinbox_moving->value() != 999) && (spinbox_fixed->value() != 999));
    if (!plates_valid)
    {
        // This will over-write any time-related feedback. But once any plate-id related issues are fixed,
        // the table goes through validation again, and so any time-related feedback will appear.
        label_validation->setText(QObject::tr("Plate ids of 999 not currently supported in creation/editing."));
    }

	// This signal can be picked up for example by the parent Edit... and Create... 
	// dialogs to update their Apply/Create buttons. 
    emit table_validity_changed(times_valid && plates_valid);

    return (times_valid && plates_valid);
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_editing_finished()
{
#if 0
	qDebug();
	qDebug() << "spinbox editing finished";
	qDebug() << "Current table row: " << table_sequences->currentRow();
	qDebug() << "Current table column: " << table_sequences->currentColumn();
	qDebug() << "Spinbox row " << d_spinbox_row;
	qDebug() << "Spinbox column" << d_spinbox_column;
	qDebug();
#endif
	QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(table_sequences->cellWidget(d_spinbox_row,d_spinbox_column));
	if (spinbox)
	{
		double spinbox_value = spinbox->value();
		QTableWidgetItem *item = table_sequences->item(d_spinbox_row,d_spinbox_column);
		item->setData(Qt::DisplayRole,spinbox_value);

		if (d_spinbox_column == ColumnNames::TIME)
		{
			table_sequences->sortItems(ColumnNames::TIME);
			validate();
		}

		if (d_spinbox_column == ColumnNames::ANGLE)
		{
			set_indeterminate_fields_for_row(table_sequences,d_spinbox_row);
		}
	}
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::set_fixed_plate_id(
        const GPlatesModel::integer_plate_id_type &fixed_plate_id_)
{
        spinbox_fixed->setValue(fixed_plate_id_);
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::set_moving_plate_id(
        const GPlatesModel::integer_plate_id_type &moving_plate_id_)
{
        spinbox_moving->setValue(moving_plate_id_);
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::handle_plate_ids_changed()
{
    if (validate())
    {
        emit plate_ids_have_changed();
    }
}

GPlatesModel::integer_plate_id_type
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::moving_plate_id() const
{
	return spinbox_moving->value();
}

GPlatesModel::integer_plate_id_type
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::fixed_plate_id() const
{
	return spinbox_fixed->value();
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::initialise()
{
	TableUpdateGuard guard(d_suppress_update_notification_guard);

    table_sequences->clearContents();
    table_sequences->setRowCount(0);
    insert_blank_row(0);
    spinbox_moving->setValue(0);
    spinbox_fixed->setValue(0);
	validate();
}

void
GPlatesQtWidgets::EditTotalReconstructionSequenceWidget::set_action_widget_in_row(
	int row)
{
	if (row < 0)
	{
		return;
	}

	if (row >= table_sequences->rowCount())
	{
		row = table_sequences->rowCount() - 1;
	}

	// Remove any existing action widget.
	for (int i = 0; i < table_sequences->rowCount() ; ++i)
	{
		if (table_sequences->cellWidget(i,ColumnNames::ACTIONS))
		{
			table_sequences->removeCellWidget(i,ColumnNames::ACTIONS);
		}
	}

	// Insert action widget in desired row.
	GPlatesQtWidgets::EditTableActionWidget *action_widget =
		new GPlatesQtWidgets::EditTableActionWidget(this, this);
	table_sequences->setCellWidget(row, ColumnNames::ACTIONS, action_widget);
}