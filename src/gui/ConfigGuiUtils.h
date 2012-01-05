/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_CONFIGGUIUTILS_H
#define GPLATES_GUI_CONFIGGUIUTILS_H

#include <QWidget>
#include <QPointer>
#include <QTableView>
#include <QVariant>
#include <QAbstractButton>
#include <QLineEdit>
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>


namespace GPlatesUtils
{
	class ConfigInterface;
}

namespace GPlatesQtWidgets
{
	class ConfigTableView;
}

namespace GPlatesGui
{
	namespace ConfigGuiUtils
	{
		/**
		 * Given a ConfigBundle (or UserPreferences) and parent widget, create a QTableView that
		 * is linked to the ConfigBundle; changes in one will be reflected in the other.
		 *
		 * @param bundle - the bundle of key/value pairs.
		 * @param parent - a QWidget to serve as the parent for the returned QTableView widget.
		 *                 This is to ensure memory will be cleaned up appropriately; it is up to
		 *                 you to insert the widget into a layout somewhere.
		 */
		GPlatesQtWidgets::ConfigTableView *
		link_config_interface_to_table(
				GPlatesUtils::ConfigInterface &config,
				bool use_icons,
				QWidget *parent);
		
		
		/**
		 * Given an existing widget (of a small number of supported types), set up signal/slot
		 * connections so that the value of the widget is synchronised with a UserPreferences
		 * key.
		 */
		void
		link_widget_to_preference(
				QLineEdit *widget,
				GPlatesUtils::ConfigInterface &config,
				const QString &key,
				QAbstractButton *reset_button);

		void
		link_widget_to_preference(
				QCheckBox *widget,
				GPlatesUtils::ConfigInterface &config,
				const QString &key,
				QAbstractButton *reset_button);

		void
		link_widget_to_preference(
				QSpinBox *widget,
				GPlatesUtils::ConfigInterface &config,
				const QString &key,
				QAbstractButton *reset_button);

		void
		link_widget_to_preference(
				QDoubleSpinBox *widget,
				GPlatesUtils::ConfigInterface &config,
				const QString &key,
				QAbstractButton *reset_button);


		class ConfigWidgetAdapter :
				public QObject
		{
			Q_OBJECT
		public:
			explicit
			ConfigWidgetAdapter(
					QWidget *widget,
					GPlatesUtils::ConfigInterface &config,
					const QString &key);
			
			virtual
			~ConfigWidgetAdapter()
			{  }
		
		signals:
			
			void
			value_changed(
					const QString &value);

			void
			value_changed(
					bool value);

			void
			value_changed(
					int value);

			void
			value_changed(
					double value);
		
		public slots:
			
			void
			handle_key_value_updated(
					QString key);
		
			void
			handle_widget_value_updated(
					QString value);

			void
			handle_widget_value_updated(
					bool value);

			void
			handle_widget_value_updated(
					int value);

			void
			handle_widget_value_updated(
					double value);

			// Because QLineEdit::editingFinished() doesn't also provide the text.
			// May be needed for other widget 'finished editing (void)' signals.
			void
			handle_widget_editing_finished();
			
			void
			handle_reset_clicked();
			
			
		private:
			QPointer<QWidget> d_widget_ptr;
			GPlatesUtils::ConfigInterface &d_config;
			QString d_key;
		};
	}
}

#endif // GPLATES_GUI_CONFIGGUIUTILS_H