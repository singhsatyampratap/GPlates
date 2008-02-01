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

#ifndef GPLATES_GUI_FEATURETABLEMODEL_H
#define GPLATES_GUI_FEATURETABLEMODEL_H

#include <QAbstractTableModel>
#include <QItemSelection>
#include <QHeaderView>

#include "gui/FeatureWeakRefSequence.h"
#include "model/FeatureHandle.h"

namespace GPlatesGui
{
	/**
	 * This class is used by Qt to map a FeatureWeakRefSequence to a QTableView.
	 * 
	 * It uses Qt's model/view framework, not to be confused with GPlates' own model/view,
	 * to provide multi-column data to a view. See the following urls for more information:
	 * http://trac.gplates.org/wiki/QtModelView
	 * http://doc.trolltech.com/4.3/model-view-programming.html
	 *
	 * To link a FeatureTableModel to the GUI, simply create a QTableView (either in the
	 * designer or in code) and call table_view->setModel(FeatureTableModel *);
	 * It is best to follow this with a call to table_view->resizeColumnsToContents();
	 * as Qt seems very picky about size hints for header items.
	 *
	 * To constrain which columns you wish to display using your QTableView, use the
	 * table_view->horizontalHeader() function to obtain a pointer to a QHeaderView.
	 * This will allow you to show, hide, and rearrange columns.
	 * The vertical header supplied by this model does not contain any useful information,
	 * and can be safely hidden with table_view->verticalHeader()->hide().
	 *
	 * The model is currently non-editable.
	 */
	class FeatureTableModel:
			public QAbstractTableModel
	{
		Q_OBJECT
		
	public:
	
		explicit
		FeatureTableModel(
				QObject *parent_ = NULL);
		
		/**
		 * Accessor for the underlying data structure.
		 * Be aware that if you use this function to add or remove features,
		 * you must call begin_insert_features(int first, int last) and end_insert_features()
		 * on this FeatureTableModel before and after the change.
		 *
		 * See http://doc.trolltech.com/4.3/qabstractitemmodel.html#beginInsertRows for
		 * more details.
		 */
		FeatureWeakRefSequence::non_null_ptr_type
		feature_sequence()
		{
			return d_sequence_ptr;
		}
		
		
		/**
		 * Qt Model/View function used to access row count, which will depend on the number
		 * of features in the FeatureWeakRefSequence.
		 * For our table model, parent_ will always be a dummy QModelIndex().
		 */
		int
		rowCount(
				const QModelIndex &parent_ = QModelIndex()) const;
		
		/**
		 * Qt Model/View function used to access column count, which will be a fixed number.
		 * For our table model, parent_ will always be a dummy QModelIndex().
		 */
		int
		columnCount(
				const QModelIndex &parent_ = QModelIndex()) const;
		
		/**
		 * Qt Model/View function used to access editable/selectable/etc status of cells.
		 */
		Qt::ItemFlags
		flags(
				const QModelIndex &idx) const;
		
		/**
		 * Qt Model/View function used to access header data, both horizontal and vertical.
		 * Pay careful attention to the role specified; it is used to select between many
		 * different types of data that might be requested by the view.
		 */
		QVariant
		headerData(
				int section,
				Qt::Orientation orientation,
				int role = Qt::DisplayRole) const;

		/**
		 * Qt Model/View function used to access individual cells of data.
		 * Pay careful attention to the role specified; it is used to select between many
		 * different types of data that might be requested by the view.
		 */
		QVariant
		data(
				const QModelIndex &idx,
				int role) const;

		/**
		 * Convenience function which will clear() the FeatureWeakRefSequence and notify any
		 * QTableViews of the change in layout.
		 */
		void
		clear()
		{
			layoutAboutToBeChanged();
			d_sequence_ptr->clear();
			layoutChanged();
		}
		
		/**
		 * If you are modifying the underlying FeatureWeakRefSequence directly,
		 * call this function before any major changes to the table data happen.
		 */
		void
		sequence_about_to_be_changed()
		{
			layoutAboutToBeChanged();
		}

		/**
		 * If you are modifying the underlying FeatureWeakRefSequence directly,
		 * call this function after any major changes to the table data happen.
		 */
		void
		sequence_changed()
		{
			layoutChanged();
		}

		/**
		 * If you are modifying the underlying FeatureWeakRefSequence directly,
		 * call this function before features are inserted. [first, last] is an
		 * inclusive range, and correspond to the row numbers the new features will
		 * have after they have been inserted.
		 */
		void
		begin_insert_features(int first, int last)
		{
			beginInsertRows(QModelIndex(), first, last);
		}

		/**
		 * If you are modifying the underlying FeatureWeakRefSequence directly,
		 * call this function after features have been inserted.
		 */
		void
		end_insert_features()
		{
			endInsertRows();
		}

		/**
		 * Convenience function to initialise a QHeaderView with the suggested
		 * resize mode appropriate for each column.
		 */
		static
		void
		set_default_resize_modes(
				QHeaderView &header);
			
	public slots:
		
		void
		handle_selection_change(
				const QItemSelection &selected,
				const QItemSelection &deselected);
	
	signals:
	
		/**
		 * Signal emitted when a user has clicked on a row of the table.
		 */
		void
		selected_feature_changed(
				GPlatesModel::FeatureHandle::weak_ref feature_ref);

	private:
		
		FeatureWeakRefSequence::non_null_ptr_type d_sequence_ptr;
	
	};
}

#endif // GPLATES_GUI_FEATURETABLEMODEL_H