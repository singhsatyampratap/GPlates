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
 
#ifndef GPLATES_QTWIDGETS_TASKPANEL_H
#define GPLATES_QTWIDGETS_TASKPANEL_H

#include <QWidget>
#include "TaskPanelUi.h"

#include "DigitisationWidget.h"
#include "ActionButtonBox.h"
#include "gui/FeatureFocus.h"


namespace GPlatesQtWidgets
{
	/**
	 * The Xtreme Task Panel - A contextual tabbed interface to expose powerful tasks
	 * to manipulate GPlates.... to the XTREME!
	 */
	class TaskPanel:
			public QWidget,
			protected Ui_TaskPanel
	{
		Q_OBJECT
		
	public:
		explicit
		TaskPanel(
				GPlatesGui::FeatureFocus &feature_focus_,
				QWidget *parent_ = NULL);
		
		/**
		 * Accessor for the Action Button Box of the Feature Tab.
		 *
		 * This lets ViewportWindow set up the QActions for the Feature tab
		 * to use. ViewportWindow needs to be the owner of the QActions so that
		 * they can be used as menu entries.
		 */
		ActionButtonBox &
		feature_action_button_box() const
		{
			return *d_feature_action_button_box_ptr;
		}

		/**
		 * Accessor for the Digitisation Widget of the Digitisation Tab.
		 *
		 * This lets the DigitiseGeometry canvas tool interact with the
		 * DigitisationWidget.
		 */
		DigitisationWidget &
		digitisation_widget() const
		{
			return *d_digitisation_widget_ptr;
		}
	
	public slots:
		
		void
		choose_feature_tab()
		{
			tabwidget_task_panel->setCurrentWidget(tab_feature);
		}
		
		void
		choose_digitisation_tab()
		{
			tabwidget_task_panel->setCurrentWidget(tab_digitisation);
		}		
		
	private:
		
		/**
		 * Sets up the "Current Feature" tab in the X-Treme Task Panel.
		 * This connects all the QToolButtons in the "Feature" tab to QActions,
		 * and adds the special FeatureSummaryWidget.
		 */
		void
		set_up_feature_tab(
				GPlatesGui::FeatureFocus &feature_focus);


		/**
		 * Sets up the "Digitisation" tab in the eXtreme Task Panel.
		 * This connects all the QToolButtons in the "Digitisation" tab to QActions,
		 * and adds the special DigitisationWidget.
		 */
		void
		set_up_digitisation_tab();


		/**
		 * Widget responsible for the buttons in the Feature Tab.
		 * Memory managed by Qt.
		 *
		 * ActionButtonBox:
		 * NOTE: The buttons you see in the Task Panel are QToolButtons.
		 * This is done so that they can be connected to a QAction and immediately inherit
		 * the icon, text, tooltips, etc of that action; furthermore, if the action is
		 * disabled/enabled, the button automagically reflects that. This is great, since
		 * most of the buttons in the Task Panel should also be available as menu items.
		 *
		 * The buttons are automatically set up by a GPlatesQtWidgets::ActionButtonBox.
		 * All you need to do is tell it how many columns to use and the pixel size of
		 * their icons (done in TaskPanel's initialiser list).
		 */
		GPlatesQtWidgets::ActionButtonBox *d_feature_action_button_box_ptr;

		/**
		 * Widget responsible for the controls in the Digitisation Tab.
		 * Memory managed by Qt.
		 */
		GPlatesQtWidgets::DigitisationWidget *d_digitisation_widget_ptr;
		
	};
}

#endif  // GPLATES_QTWIDGETS_TASKPANEL_H