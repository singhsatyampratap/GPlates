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
 
#ifndef GPLATES_GUI_DOCKSTATE_H
#define GPLATES_GUI_DOCKSTATE_H

#include <QObject>
#include <QList>
#include <QPointer>


namespace GPlatesQtWidgets
{
	class ViewportWindow;
	class DockWidget;
}


namespace GPlatesGui
{
	/**
	 * This GUI class tracks all the GPlatesQtWidgets::DockWidget used
	 * by GPlates, remembering which docks currently occupy which positions.
	 * It takes dock-organising code out of ViewportWindow and helps
	 * deal with the micro-management that is sometimes necessary.
	 */
	class DockState: 
			public QObject
	{
		Q_OBJECT
		
	public:
	
		explicit
		DockState(
				GPlatesQtWidgets::ViewportWindow &viewport_window_,
				QObject *parent_);

		virtual
		~DockState()
		{  }


		/**
		 * Adds signal/slot connections to track dock positions.
		 */
		void
		register_dock(
				GPlatesQtWidgets::DockWidget &dock);

		/**
		 * A replacement for the addDockWidget() etc methods on ViewportWindow (QMainWindow).
		 */
		void
		move_dock(
				GPlatesQtWidgets::DockWidget &dock,
				Qt::DockWidgetArea area,
				bool tabify_as_appropriate);

		/**
		 * Check if tabification with another DockWidget is possible for given location.
		 *
		 * @a dock - a reference to the DockWidget that would be tabifying itself
		 * into @a area; this is required since one should not attempt to tabify with oneself.
		 */
		bool
		can_tabify(
				Qt::DockWidgetArea area,
				const GPlatesQtWidgets::DockWidget &dock);

	signals:

		void
		dock_configuration_changed();

	private slots:

		void
		react_dockwidget_change(
				GPlatesQtWidgets::DockWidget &dock,
				Qt::DockWidgetArea area,
				bool floating);

	private:

		/**
		 * Remove the given DockWidget from all the 'dock location' lists,
		 * typically so that it can be added to a new location.
		 */
		void
		remove_from_location_lists(
				GPlatesQtWidgets::DockWidget *remove);

		/**
		 * Pointer to the ViewportWindow so we can access Qt's dock code.
		 */
		QPointer<GPlatesQtWidgets::ViewportWindow> d_viewport_window_ptr;

		/**
		 * A list of guarded QPointers to DockWidgets for each area of the main window
		 * that docks can reside in. The lists are maintained by listening to state
		 * change signals emitted from the QDockWidget class. We want to remember these
		 * states so that dock manipulation can be done intelligently (e.g. tabifying
		 * a dock widget with a dock widget that already occupies that space).
		 *
		 * They are QPointers because we do not own the QObjects they point to, but
		 * for sanity's sake we need to ensure we never get a dangling pointer due to
		 * those docks being deleted (which should never happen until app exit anyway).
		 */
		QList< QPointer<GPlatesQtWidgets::DockWidget> > d_floating;
		QList< QPointer<GPlatesQtWidgets::DockWidget> > d_docked_top;
		QList< QPointer<GPlatesQtWidgets::DockWidget> > d_docked_bottom;
		QList< QPointer<GPlatesQtWidgets::DockWidget> > d_docked_left;
		QList< QPointer<GPlatesQtWidgets::DockWidget> > d_docked_right;
	};
}


#endif	// GPLATES_GUI_DOCKSTATE_H