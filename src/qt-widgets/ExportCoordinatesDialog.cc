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

#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QString>

#include "ExportCoordinatesDialog.h"

#include "InformationDialog.h"


namespace
{
	/**
	 * Enumeration for the possible formats to export to.
	 * The order of these should match the setup of the @a combobox_format
	 * as set up in the designer.
	 *
	 * FIXME: When we implement the writers, we will probably want to associate
	 * information with these enums; this would be a good point to include the
	 * combobox text so we can set it up in code.
	 */
	enum OutputFormat
	{
		PLATES4, GMT, WKT, CSV
	};
}


const QString GPlatesQtWidgets::ExportCoordinatesDialog::s_terminating_point_information_text =
		QObject::tr("<html><body>\n"
		"<h3>Including an additional terminating point for polygons</h3>"
		"<p>GPlates stores polygons using the minimum number of vertices necessary to specify "
		"a closed polygon.</p>\n"
		"<p>However, some software may expect the final point of the polygon to be identical "
		"to the first point, in order to create a closed circuit. If this box is checked, "
		"the exported data will include an additional terminating point identical to the first.</p>\n"
		"</body></html>");


GPlatesQtWidgets::ExportCoordinatesDialog::ExportCoordinatesDialog(
		QWidget *parent_):
	QDialog(parent_),
	d_export_file_dialog(new QFileDialog(this, tr("Select a filename for exporting"), QString(),
			"All files (*)")),
	d_terminating_point_information_dialog(new InformationDialog(s_terminating_point_information_text,
			tr("Polygon point conventions"), this))
{
	setupUi(this);
	
	// What happens when the user selects a format?
	QObject::connect(combobox_format, SIGNAL(currentIndexChanged(int)),
			this, SLOT(handle_format_selection(int)));
	
	// The "Terminating Point" option for polygons.
	QObject::connect(button_explain_terminating_point, SIGNAL(clicked()),
			d_terminating_point_information_dialog, SLOT(show()));
	
	// Radio buttons control destination selection widgets.
	QObject::connect(radiobutton_to_file, SIGNAL(clicked()),
			this, SLOT(select_to_file_stack()));
	QObject::connect(radiobutton_to_clipboard, SIGNAL(clicked()),
			this, SLOT(select_to_clipboard_stack()));

	// Set up the export file selection dialog.
	d_export_file_dialog->setAcceptMode(QFileDialog::AcceptSave);
	QObject::connect(button_select_file, SIGNAL(clicked()),
			this, SLOT(pop_up_file_browser()));
	
	// Default 'OK' button should read 'Export'.
	buttonbox_export->addButton(tr("Export"), QDialogButtonBox::AcceptRole);
	QObject::connect(buttonbox_export, SIGNAL(accepted()),
			this, SLOT(handle_export()));
	
	// Select the default output format, to ensure that any currently-displayed
	// widgets will be initialised to the appropriate defaults.
	combobox_format->setCurrentIndex(0);
	handle_format_selection(0);
}


void
GPlatesQtWidgets::ExportCoordinatesDialog::set_geometry(
		GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type geometry_)
{
	// FIXME: Set a member somewhere. Probably a boost::optional, given that we
	// can't take it in the constructor.
}


void
GPlatesQtWidgets::ExportCoordinatesDialog::handle_format_selection(
		int idx)
{
	OutputFormat format = OutputFormat(idx);
	switch (format)
	{
	case PLATES4:
			// Set some default options to match what the format prescribes.
			combobox_coordinate_order->setCurrentIndex(0);
			checkbox_polygon_terminating_point->setChecked(true);
			break;
			
	case GMT:
			break;
	case WKT:
			break;
	case CSV:
			break;
	default:
			// Can't possibly happen, because we set up each possible combobox state, right?
			return;
	}
}


void
GPlatesQtWidgets::ExportCoordinatesDialog::pop_up_file_browser()
{
	if (d_export_file_dialog->exec()) {
		lineedit_filename->setText(d_export_file_dialog->selectedFiles().front());
	}
}



void
GPlatesQtWidgets::ExportCoordinatesDialog::handle_export()
{
	// FIXME: Set up a text stream, either to file or a QBuffer.
	if (radiobutton_to_file->isChecked()) {
		QString filename = lineedit_filename->text();
		if (filename.isEmpty()) {
			QMessageBox::warning(this, tr("No filename specified"),
					tr("Please specify a filename to export to."));
			lineedit_filename->setFocus();
			return;
		}
	}
	// FIXME: Delegate writing to the output stream to some writer depending
	// on the desired output format. (GeometryOnSphere visitor)
	
	// If we reach here, everything has been exported successfully and we can close the dialog.
	accept();
}
