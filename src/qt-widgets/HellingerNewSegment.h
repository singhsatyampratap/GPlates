/* $Id: HellingerNewSegment.h 227 2012-02-24 14:46:55Z juraj.cirbus $ */

/**
 * \file
 * $Revision: 227 $
 * $Date: 2012-02-24 15:46:55 +0100 (Fri, 24 Feb 2012) $
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

#ifndef GPLATES_QTWIDGETS_HELLINGERNEWSEGMENT_H
#define GPLATES_QTWIDGETS_HELLINGERNEWSEGMENT_H

#include <QAbstractTableModel>
#include <QItemDelegate>
#include <QtCore>
#include <QtGui>
#include <QWidget>

#include "HellingerNewSegmentUi.h"
#include "HellingerDialog.h"




namespace GPlatesQtWidgets
{
	enum ColumnType{
		COLUMN_MOVING_FIXED = 0,
		COLUMN_LAT,
		COLUMN_LON,
		COLUMN_UNCERTAINTY,

		NUM_COLUMNS
	};

	class HellingerDialog;
	class HellingerModel;
	class HellingerNewSegmentWarning;

	/**
	 * @brief The SpinBoxDelegate class
	 *
	 * This lets us customise the spinbox behaviour in the TableView. Borrowed largely from the
	 * Qt example here:
	 * http://qt-project.org/doc/qt-4.8/itemviews-spinboxdelegate.html
	 *
	 */
	class SpinBoxDelegate : public QItemDelegate
	{
		Q_OBJECT

	public:
		SpinBoxDelegate(QObject *parent = 0);

		QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
							  const QModelIndex &index) const;

		void setEditorData(QWidget *editor, const QModelIndex &index) const;
		void setModelData(QWidget *editor, QAbstractItemModel *model,
						  const QModelIndex &index) const;

		void updateEditorGeometry(QWidget *editor,
								  const QStyleOptionViewItem &option, const QModelIndex &index) const;
	};





	class HellingerNewSegment:
			public QDialog,
			protected Ui_HellingerNewSegment
	{
		Q_OBJECT
	public:
		HellingerNewSegment(
				HellingerDialog *hellinger_dialog,
				HellingerModel *hellinger_model,
				QWidget *parent_ = NULL);

		void
		reset();

		int d_type_new_segment_error;

	private Q_SLOTS:

		void
		handle_add_segment();

		void
		handle_add_line();

		void
		handle_remove_line();

		void
		add_segment_to_model();

		void
		change_table_stats_pick();

		void
		handle_item_changed(QStandardItem *item);



	private:

		void
		change_quick_set_state();

		void
		update_buttons();

		HellingerDialog *d_hellinger_dialog_ptr;
		QStandardItemModel *model;
		HellingerModel *d_hellinger_model_ptr;
		HellingerNewSegmentWarning *d_hellinger_new_segment_warning;

		int d_row_count;

		SpinBoxDelegate d_spin_box_delegate;
	};
}

#endif //GPLATES_QTWIDGETS_HELLINGERNEWSEGMENT_H
