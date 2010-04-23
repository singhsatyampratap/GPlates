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

#include <QApplication>
#include <QDebug>
#include <QMenuBar>
#include <QMenu>
#include <QMetaMethod>

#include "GuiDebug.h"

#include "model/FeatureCollectionHandle.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/FeatureCollectionFileState.h"
#include "qt-widgets/ViewportWindow.h"
#include "qt-widgets/TaskPanel.h"


namespace
{

	/**
	 * Given a QObject, introspect it for slots that take no arguments (and optionally
	 * only ones that start with a given prefix), and add a menu entry for each slot to
	 * the supplied menu.
	 */
	void
	add_slots_to_menu(
			const QObject *object,
			QString prefix,
			QMenu *menu)
	{
		if ( ! object) {
			return;
		}
		const QMetaObject *introspect = object->metaObject();
		for (int i = introspect->methodOffset(); i < introspect->methodCount(); ++i) {
			QMetaMethod method = introspect->method(i);
			// Aha! A method of ours. Is it a slot which takes no arguments?
			if (method.methodType() == QMetaMethod::Slot && method.parameterTypes().isEmpty()) {
				QString label(method.signature());
				// does it match the given prefix?
				if (prefix.isNull() || prefix.isEmpty() || label.startsWith(prefix)) {
					// Below I use a little hack to emulate the SLOT() macro on a dynamic char*:
					QString slot("1");
					slot.append(method.signature());
					// Add to menu.
					menu->addAction(label, object, slot.toAscii().constData());
				}
			}
		}
	}

	/**
	 * Convenience version of @a add_slots_to_menu that only adds slots with the
	 * prefix 'debug_'.
	 */
	void
	add_debug_slots_to_menu(
			const QObject *object,
			QMenu *menu)
	{
		if ( ! object) {
			return;
		}
		add_slots_to_menu(object, "debug_", menu);
	}
	
	/**
	 * Convenience version of @a add_slots_to_menu that adds menu items under a
	 * submenu with the class name of the object.
	 */
	void
	add_slots_as_submenu(
			const QObject *object,
			QString prefix,
			QMenu *menu)
	{
		if ( ! object) {
			return;
		}
		QMenu *submenu = menu->addMenu(object->metaObject()->className());
		// Tearable menus are delicious.
		submenu->setTearOffEnabled(true);
		add_slots_to_menu(object, prefix, submenu);
	}
}



GPlatesGui::GuiDebug::GuiDebug(
		GPlatesQtWidgets::ViewportWindow &viewport_window_,
		GPlatesAppLogic::ApplicationState &app_state_,
		QObject *parent_):
	QObject(parent_),
	d_viewport_window_ptr(&viewport_window_),
	d_app_state_ptr(&app_state_)
{
	create_menu();
}


void
GPlatesGui::GuiDebug::create_menu()
{
	// Create and add the main Debug menu.
	QMenu *debug_menu = new QMenu(tr("&Debug"), d_viewport_window_ptr);
	d_viewport_window_ptr->menuBar()->addMenu(debug_menu);
	// Tearable menus should really be the standard everywhere ever.
	debug_menu->setTearOffEnabled(true);
	
	// Add and connect actions to the menu.
	QAction *action_Gui_Debug_Action = new QAction(QIcon(":/info_sign_16.png"), tr("GUI Debug &Action"), this);
	action_Gui_Debug_Action->setShortcutContext(Qt::ApplicationShortcut);
	action_Gui_Debug_Action->setShortcut(tr("Ctrl+Alt+/"));
	debug_menu->addAction(action_Gui_Debug_Action);
	QObject::connect(action_Gui_Debug_Action, SIGNAL(triggered()),
			this, SLOT(handle_gui_debug_action()));
	
	debug_menu->addSeparator();
	
	// Automagically add any slot of ours beginning with 'debug_'.
	// If you don't need a keyboard shortcut for it, this is a fantastic way to
	// quickly add some test code you can trigger at-will at runtime.
	add_debug_slots_to_menu(this, debug_menu);
	
	// For bonus points, let's add ALL no-argument slots from ViewportWindow and friends.
	add_slots_as_submenu(d_viewport_window_ptr, "", debug_menu);
	add_slots_as_submenu(d_viewport_window_ptr->task_panel_ptr(), "", debug_menu);
	add_slots_as_submenu(find_child_qobject("ManageFeatureCollectionsDialog"), "", debug_menu);
	add_slots_as_submenu(find_child_qobject("UnsavedChangesTracker"), "", debug_menu);
}


QObject *
GPlatesGui::GuiDebug::find_child_qobject(
		QString name)
{
	QObject *found = d_viewport_window_ptr->findChild<QObject *>(name);
	if ( ! found) {
		qDebug() << "GuiDebug::find_child_qobject("<<name<<"): Couldn't find this one. Is it parented"
				<< " (directly or indirectly) to ViewportWindow, and does it have a proper objectName set?";
	}
	return found;
}


void
GPlatesGui::GuiDebug::handle_gui_debug_action()
{
	// Some handy information that may aid debugging:

	// "Where the hell did my keyboard focus go?"
	qDebug() << "Current focus:" << QApplication::focusWidget();

	// "What's the name of the current style so I can test against it?"
	qDebug() << "Current style:" << d_viewport_window_ptr->style()->objectName();


	// "What's this thing doing there?"
	QWidget *cursor_widget = QApplication::widgetAt(QCursor::pos());
	qDebug() << "Current widget under cursor:" << cursor_widget;
	while (cursor_widget && cursor_widget->parentWidget()) {
		cursor_widget = cursor_widget->parentWidget();
		qDebug() << "\twhich is inside:" << cursor_widget;
	}
}


void
GPlatesGui::GuiDebug::debug_set_all_files_clean()
{
	qDebug() << "GPlatesGui::GuiDebug::debug_set_all_files_clean()";

	// Grab the FeatureCollectionFileState and just go through all loaded files' feature collections.
	GPlatesAppLogic::FeatureCollectionFileState &fcfs = 
			d_app_state_ptr->get_feature_collection_file_state();
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator_range it_range =
			fcfs.get_loaded_files();
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator it = it_range.begin;
	GPlatesAppLogic::FeatureCollectionFileState::file_iterator end = it_range.end;
	
	for (; it != end; ++it) {
		GPlatesModel::FeatureCollectionHandle::weak_ref feature_collection_ref = it->get_feature_collection();

		if (feature_collection_ref.is_valid()) {
			feature_collection_ref->clear_unsaved_changes();
		}
	}
}
