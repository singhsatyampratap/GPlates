/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_VISUALLAYERSLISTVIEW_H
#define GPLATES_QTWIDGETS_VISUALLAYERSLISTVIEW_H

#include <QListView>


namespace GPlatesPresentation
{
	class VisualLayers;
}

namespace GPlatesQtWidgets
{
	class VisualLayersListView :
			public QListView
	{
	public:

		VisualLayersListView(
				GPlatesPresentation::VisualLayers &visual_layers,
				QWidget *parent_ = NULL);

		virtual
		void
		dropEvent(QDropEvent *event_);

	protected slots:

		virtual
		void
		rowsInserted(
				const QModelIndex &parent_,
				int start,
				int end);

	private:

		/**
		 * Opens the persistent editor for entries in the list from @a begin_row up
		 * to the entry before @a end_row (i.e. half-open range).
		 */
		void
		open_persistent_editors(
				int begin_row,
				int end_row);
	};

}


#endif	// GPLATES_QTWIDGETS_VISUALLAYERSLISTVIEW_H
