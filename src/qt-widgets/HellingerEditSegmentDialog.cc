/* $Id: HellingerEditSegmentDialog.cc 241 2012-02-28 11:28:13Z robin.watson@ngu.no $ */

/**
 * \file
 * $Revision: 241 $
 * $Date: 2012-02-28 12:28:13 +0100 (Tue, 28 Feb 2012) $
 *
 * Copyright (C) 2011, 2012, 2013 Geological Survey of Norway
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
#include <QRadioButton>
#include <QTableView>
#include <QTextStream>


#include "HellingerDialog.h"
#include "HellingerDialogUi.h"
#include "HellingerModel.h"
#include "HellingerEditSegmentDialog.h"
#include "HellingerNewSegmentWarning.h"
#include "QtWidgetUtils.h"

namespace
{
	/**
	 * @brief translate_segment_type
	 *	Convert MOVING/DISABLED_MOVING types to a QString form of MOVING; similarly for FIXED/DISABLED_FIXED.
	 *
	 * This is copied from HellingerDialog anonymouse namespace - could be moved into a common HellingerUtils file, but
	 * this is the only candidate for that at the moment.
	 * @param type
	 * @return
	 */
	QString
	translate_segment_type(
			GPlatesQtWidgets::HellingerPickType type)
	{
		switch(type)
		{
		case GPlatesQtWidgets::MOVING_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_MOVING_PICK_TYPE:
			return QString::number(GPlatesQtWidgets::MOVING_PICK_TYPE);
			break;
		case GPlatesQtWidgets::FIXED_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_FIXED_PICK_TYPE:
			return QString::number(GPlatesQtWidgets::FIXED_PICK_TYPE);
			break;
		default:
			return QString();
		}
	}
#if 0
	/**
	 * @brief set_text_colour_according_to_enabled_state
	 * - copied from HellingerDialog - will need to be adapted for the table widget.
	 * @param item
	 * @param enabled
	 */
	void
	set_text_colour_according_to_enabled_state(
			QTreeWidgetItem *item,
			bool enabled)
	{

		const Qt::GlobalColor text_colour = enabled? Qt::black : Qt::gray;
		static const Qt::GlobalColor background_colour = Qt::white;

		item->setBackgroundColor(SEGMENT_NUMBER,background_colour);
		item->setBackgroundColor(SEGMENT_TYPE,background_colour);
		item->setBackgroundColor(LAT,background_colour);
		item->setBackgroundColor(LON,background_colour);
		item->setBackgroundColor(UNCERTAINTY,background_colour);

		item->setTextColor(SEGMENT_NUMBER,text_colour);
		item->setTextColor(SEGMENT_TYPE,text_colour);
		item->setTextColor(LAT,text_colour);
		item->setTextColor(LON,text_colour);
		item->setTextColor(UNCERTAINTY,text_colour);
	}
#endif
}

GPlatesQtWidgets::HellingerEditSegmentDialog::HellingerEditSegmentDialog(
		HellingerDialog *hellinger_dialog,
		HellingerModel *hellinger_model,
		bool create_new_segment,
		QWidget *parent_):
	QDialog(parent_,Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_hellinger_dialog_ptr(hellinger_dialog),
	d_hellinger_model_ptr(hellinger_model),
	d_hellinger_new_segment_warning(0),
	d_creating_new_segment(create_new_segment)
{
	setupUi(this);
	QObject::connect(button_add_segment, SIGNAL(clicked()), this, SLOT(handle_add_segment()));
	QObject::connect(button_add_line, SIGNAL(clicked()), this, SLOT(handle_add_line()));
	QObject::connect(button_remove_line, SIGNAL(clicked()), this, SLOT(handle_remove_line()));
	QObject::connect(radio_moving, SIGNAL(clicked()), this, SLOT(change_pick_type_of_whole_table()));
	QObject::connect(radio_fixed, SIGNAL(clicked()), this, SLOT(change_pick_type_of_whole_table()));
	QObject::connect(radio_custom, SIGNAL(clicked()), this, SLOT(change_pick_type_of_whole_table()));
	QObject::connect(button_reset,SIGNAL(clicked()), this, SLOT(handle_reset()));
	QObject::connect(button_enable,SIGNAL(clicked()), this, SLOT(handle_enable()));
	QObject::connect(button_disable,SIGNAL(clicked()), this, SLOT(handle_disable()));

	// NOTE: I've added these two so that the "remove" button is enabled whenever a row/cell is highlighted.
	// Initially nothing is selected so it would be unclear which row is the target of the removal operation.
	QObject::connect(table_new_segment->verticalHeader(),SIGNAL(sectionClicked(int)),this,SLOT(update_buttons()));
	QObject::connect(table_new_segment,SIGNAL(clicked(QModelIndex)),this,SLOT(update_buttons()));



	d_model = new QStandardItemModel(NUM_COLUMNS,1, this);
	d_model->setHorizontalHeaderItem(COLUMN_MOVING_FIXED,new QStandardItem("Moving(1)/Fixed(2)"));
	d_model->setHorizontalHeaderItem(COLUMN_LAT, new QStandardItem("Lat"));
	d_model->setHorizontalHeaderItem(COLUMN_LON, new QStandardItem("Long"));
	d_model->setHorizontalHeaderItem(COLUMN_UNCERTAINTY, new QStandardItem("Uncertainty (km)"));

	d_model->setRowCount(1);

	set_initial_row_values(0);

	table_new_segment->setModel(d_model);

	table_new_segment->horizontalHeader()->resizeSection(COLUMN_MOVING_FIXED,140);
	table_new_segment->horizontalHeader()->resizeSection(COLUMN_LAT,100);
	table_new_segment->horizontalHeader()->resizeSection(COLUMN_LON,100);
	table_new_segment->horizontalHeader()->resizeSection(COLUMN_UNCERTAINTY,100);

	table_new_segment->horizontalHeader()->setStretchLastSection(true);

	update_buttons();

	// The spinbox delegate lets us customise spinbox behaviour for the different cells.
	table_new_segment->setItemDelegate(&d_spin_box_delegate);

	// Mark row 0 (or at least an item in row 0) as the current index.
	QModelIndex index = d_model->index(0,COLUMN_MOVING_FIXED);
	table_new_segment->selectionModel()->setCurrentIndex(index,QItemSelectionModel::NoUpdate);

	if (!create_new_segment)
	{
		button_add_segment->setText(QObject::tr("Apply"));
		setWindowTitle(QObject::tr("Edit Segment"));
	}

}

void GPlatesQtWidgets::HellingerEditSegmentDialog::initialise_with_segment(
		const hellinger_segment_type &picks,
		const int &segment_number)
{
	spinbox_segment->setValue(segment_number);
	d_original_segment_number.reset(segment_number);

	d_model->removeRows(0,d_model->rowCount());

	hellinger_segment_type::const_iterator
			iter = picks.begin(),
			iter_end = picks.end();
	for (; iter != iter_end ; ++iter)
	{
		d_model->insertRow(d_model->rowCount());
		set_row_values(d_model->rowCount()-1,*iter);
	}
}

void
GPlatesQtWidgets::HellingerEditSegmentDialog::handle_add_segment()
{

	// NOTE: We don't check for contiguous segment numbers here. It could be an idea to
	// check for this here and suggest the next "available" segment number if the user has
	// entered a value greater than (highest-so-far)+1. The contiguity is checked and corrected
	// before performing the fit anyway, so it doesn't have to be here by any means.


	if (!d_creating_new_segment && d_original_segment_number)
	{
		handle_edited_segment();
	}
	else
	{
		handle_new_segment();
	}

}

void
GPlatesQtWidgets::HellingerEditSegmentDialog::add_segment_to_model()
{
	d_hellinger_dialog_ptr->store_expanded_status();
	int segment = spinbox_segment->value();

	for (int row = 0; row < d_model->rowCount(); ++row)
	{
		QModelIndex index;
		QVariant variant;

		// Moving or fixed
		index = d_model->index(row,COLUMN_MOVING_FIXED);
		variant = table_new_segment->model()->data(index);
		// The spinboxes should already ensure valid data types/values for each column.
		HellingerPickType type = static_cast<HellingerPickType>(variant.toInt());

		// Latitude
		index = d_model->index(row,COLUMN_LAT);
		variant = table_new_segment->model()->data(index);
		double lat = variant.toDouble();

		// Longitude
		index = d_model->index(row,COLUMN_LON);
		variant = table_new_segment->model()->data(index);
		double lon = variant.toDouble();

		// Uncertainty
		index = d_model->index(row,COLUMN_UNCERTAINTY);
		variant = table_new_segment->model()->data(index);
		double uncertainty = variant.toDouble();

		GPlatesQtWidgets::HellingerPick pick(type,lat,lon,uncertainty,true /* enabled */);
		d_hellinger_model_ptr->add_pick(pick,segment);
	}
	d_hellinger_dialog_ptr->update_tree_from_model();
	d_hellinger_dialog_ptr->restore_expanded_status();
	d_hellinger_dialog_ptr->expand_segment(segment);
}

void
GPlatesQtWidgets::HellingerEditSegmentDialog::handle_add_line()
{
	int insertion_row;
	if ((d_model->rowCount() == 0) ||
		 table_new_segment->selectionModel()->selection().indexes().isEmpty())
	{
		insertion_row = 0;
	}
	else
	{

		const QModelIndex index = table_new_segment->currentIndex();
		insertion_row = index.row();
	}

	d_model->insertRow(insertion_row);
	set_initial_row_values(insertion_row);
}

void
GPlatesQtWidgets::HellingerEditSegmentDialog::handle_remove_line()
{
	if (table_new_segment->selectionModel()->selection().indexes().isEmpty())
	{
		return;
	}
	const QModelIndex index = table_new_segment->currentIndex();
	int row = index.row();
	d_model->removeRow(row);
}

void
GPlatesQtWidgets::HellingerEditSegmentDialog::change_pick_type_of_whole_table()
{
	if (radio_moving->isChecked())
	{
		for (int row = 0; row < d_model->rowCount(); ++row)
		{
			QModelIndex index_move_fix = d_model->index(row, COLUMN_MOVING_FIXED);
			d_model->setData(index_move_fix, GPlatesQtWidgets::MOVING_PICK_TYPE);
		}
	}
	else if (radio_fixed->isChecked())
	{
		for (int row = 0; row < d_model->rowCount(); ++row)
		{
			QModelIndex index_move_fix = d_model ->index(row, COLUMN_MOVING_FIXED, QModelIndex());
			d_model->setData(index_move_fix, GPlatesQtWidgets::FIXED_PICK_TYPE);
		}
	}
}

void
GPlatesQtWidgets::HellingerEditSegmentDialog::update_buttons()
{
	QModelIndexList indices = table_new_segment->selectionModel()->selection().indexes();

	button_remove_line->setEnabled(!indices.isEmpty());
}

void GPlatesQtWidgets::HellingerEditSegmentDialog::handle_reset()
{

}

void GPlatesQtWidgets::HellingerEditSegmentDialog::handle_enable()
{

}

void GPlatesQtWidgets::HellingerEditSegmentDialog::handle_disable()
{

}

void GPlatesQtWidgets::HellingerEditSegmentDialog::handle_edited_segment()
{
	int segment_number = spinbox_segment->value();

	if (d_original_segment_number.get() == segment_number)
	{
		d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
		add_segment_to_model();
	}
	else if(d_hellinger_model_ptr->segment_number_exists(segment_number))
	{
		if (!d_hellinger_new_segment_warning)
		{
			d_hellinger_new_segment_warning = new GPlatesQtWidgets::HellingerNewSegmentWarning(
						d_hellinger_dialog_ptr,
						segment_number);

		}

		d_hellinger_new_segment_warning->exec();
		int value_error = d_hellinger_new_segment_warning->error_type_new_segment();
		if (value_error == ACTION_ADD_NEW_SEGMENT)
		{
			d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
			add_segment_to_model();
		}
		else if (value_error == ACTION_REPLACE_NEW_SEGMENT)
		{
			d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
			d_hellinger_model_ptr->remove_segment(segment_number);
			add_segment_to_model();
		}
		else if (value_error == ACTION_INSERT_NEW_SEGMENT)
		{
			d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
			d_hellinger_model_ptr->make_space_for_new_segment(segment_number);
			add_segment_to_model();
		}
		else
		{
			// We should only get here if the user pressed cancel. In this case we return,
			// which will keep this dialog open so that the user can adjust the fields in their
			// prospective new segment and try again if they want to.
			return;
		}
	}
	else
	{
		// Everything was cool.
		d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
		add_segment_to_model();
	}
	reject();
}

void GPlatesQtWidgets::HellingerEditSegmentDialog::handle_new_segment()
{
	int segment_number = spinbox_segment->value();

	if(d_hellinger_model_ptr->segment_number_exists(segment_number))
	{
		if (!d_hellinger_new_segment_warning)
		{
			d_hellinger_new_segment_warning = new GPlatesQtWidgets::HellingerNewSegmentWarning(
						d_hellinger_dialog_ptr,
						segment_number);

		}

		d_hellinger_new_segment_warning->exec();
		int value_error = d_hellinger_new_segment_warning->error_type_new_segment();
		if (value_error == ACTION_ADD_NEW_SEGMENT)
		{
			d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
			add_segment_to_model();
		}
		else if (value_error == ACTION_REPLACE_NEW_SEGMENT)
		{
			d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
			d_hellinger_model_ptr->remove_segment(segment_number);
			add_segment_to_model();
		}
		else if (value_error == ACTION_INSERT_NEW_SEGMENT)
		{
			d_hellinger_model_ptr->remove_segment(d_original_segment_number.get());
			d_hellinger_model_ptr->make_space_for_new_segment(segment_number);
			add_segment_to_model();
		}
		else
		{
			// We should only get here if the user pressed cancel. In this case we return,
			// which will keep this dialog open so that the user can adjust the fields in their
			// prospective new segment and try again if they want to.
			return;
		}
	}
	else
	{
		// Everything was cool.
		add_segment_to_model();
	}
	reject();
}

void GPlatesQtWidgets::HellingerEditSegmentDialog::set_initial_row_values(const int &row)
{
	QModelIndex index_move_fix = d_model->index(row, COLUMN_MOVING_FIXED);
	d_model->setData(index_move_fix, 1);

	for (int col = 1; col < NUM_COLUMNS; ++col)
	{
		QModelIndex index = d_model->index(row, col);
		d_model->setData(index, 0.00);
	}

}

void GPlatesQtWidgets::HellingerEditSegmentDialog::set_row_values(
		const int &row,
		const GPlatesQtWidgets::HellingerPick &pick)
{
	QModelIndex index = d_model->index(row,COLUMN_MOVING_FIXED);
	d_model->setData(index,pick.d_segment_type);

	index = d_model->index(row,COLUMN_LAT);
	d_model->setData(index,pick.d_lat);

	index = d_model->index(row,COLUMN_LON);
	d_model->setData(index,pick.d_lon);

	index = d_model->index(row,COLUMN_UNCERTAINTY);
	d_model->setData(index,pick.d_uncertainty);

}


GPlatesQtWidgets::SpinBoxDelegate::SpinBoxDelegate(QObject *parent_):
	QItemDelegate(parent_)
{}

QWidget*
GPlatesQtWidgets::SpinBoxDelegate::createEditor(
		QWidget *parent_,
		const QStyleOptionViewItem &/* option */,
		const QModelIndex &index) const
{
	int column = index.column();

	switch(column){
	case GPlatesQtWidgets::HellingerEditSegmentDialog::COLUMN_MOVING_FIXED:
	{
		QSpinBox *editor = new QSpinBox(parent_);
		editor->setMinimum(1);
		editor->setMaximum(2);
		return editor;
		break;
	}
	case GPlatesQtWidgets::HellingerEditSegmentDialog::COLUMN_LAT:
	{
		QDoubleSpinBox *editor = new QDoubleSpinBox(parent_);
		editor->setMinimum(-90.);
		editor->setMaximum(90.);
		return editor;
		break;
	}
	case GPlatesQtWidgets::HellingerEditSegmentDialog::COLUMN_LON:
	{
		QDoubleSpinBox *editor = new QDoubleSpinBox(parent_);
		editor->setMinimum(-360.);
		editor->setMaximum(360.);
		return editor;
		break;
	}
	case GPlatesQtWidgets::HellingerEditSegmentDialog::COLUMN_UNCERTAINTY:
	default:
	{
		QDoubleSpinBox *editor = new QDoubleSpinBox(parent_);
		editor->setMinimum(0.);
		editor->setMaximum(1000.);
		return editor;
		break;
	}
	}
}

void
GPlatesQtWidgets::SpinBoxDelegate::setEditorData(
		QWidget *editor,
		const QModelIndex &index) const
{

	int column = index.column();

	switch(column){
	case GPlatesQtWidgets::HellingerEditSegmentDialog::COLUMN_MOVING_FIXED:
	{
		int value = index.model()->data(index, Qt::EditRole).toInt();
		QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
		spinbox->setValue(value);
		break;
	}
	case GPlatesQtWidgets::HellingerEditSegmentDialog::COLUMN_LAT:
	case GPlatesQtWidgets::HellingerEditSegmentDialog::COLUMN_LON:
	case GPlatesQtWidgets::HellingerEditSegmentDialog::COLUMN_UNCERTAINTY:
	{
		int value = index.model()->data(index, Qt::EditRole).toDouble();
		QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(editor);
		spinbox->setValue(value);
		break;
	}

	}

}

void
GPlatesQtWidgets::SpinBoxDelegate::setModelData(
		QWidget *editor,
		QAbstractItemModel *model,
		const QModelIndex &index) const
{
	int column = index.column();

	QVariant value;
	switch(column){
	case GPlatesQtWidgets::HellingerEditSegmentDialog::COLUMN_MOVING_FIXED:
	{
		QSpinBox *spinbox = static_cast<QSpinBox*>(editor);
		value = spinbox->value();
		break;
	}
	case GPlatesQtWidgets::HellingerEditSegmentDialog::COLUMN_LAT:
	case GPlatesQtWidgets::HellingerEditSegmentDialog::COLUMN_LON:
	case GPlatesQtWidgets::HellingerEditSegmentDialog::COLUMN_UNCERTAINTY:
	{
		QDoubleSpinBox *spinbox = static_cast<QDoubleSpinBox*>(editor);
		value = spinbox->value();
		break;
	}
	}
	model->setData(index,value,Qt::EditRole);

}

void
GPlatesQtWidgets::SpinBoxDelegate::updateEditorGeometry(
		QWidget *editor,
		const QStyleOptionViewItem &option,
		const QModelIndex &/* index */) const
{
	editor->setGeometry(option.rect);
}

