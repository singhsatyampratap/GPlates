/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
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

#include <QTableWidget>
#include <QHeaderView>
#include <QFileDialog>
#include <QMessageBox>
#include "ApplicationState.h"
#include "ViewportWindow.h"
#include "file-io/FileInfo.h"
#include "file-io/FeatureCollectionFileFormat.h"
#include "file-io/ErrorOpeningFileForWritingException.h"
#include "file-io/ErrorOpeningPipeToGzipException.h"
#include "file-io/FileFormatNotSupportedException.h"
#include "file-io/GpmlOnePointSixOutputVisitor.h"
#include "global/UnexpectedEmptyFeatureCollectionException.h"
#include "ManageFeatureCollectionsActionWidget.h"
#include "ManageFeatureCollectionsDialog.h"
#include "GMTHeaderFormatDialog.h"


namespace
{
	namespace ColumnNames
	{
		/**
		 * These should match the columns set up in the designer.
		 */
		enum ColumnName
		{
			FILENAME, FORMAT, IN_USE, ACTIONS
		};
	}
	
	const QString &
	get_format_for_file(
			const QFileInfo &qfileinfo)
	{
		static const QString format_line(QObject::tr("PLATES4 line"));
		static const QString format_rotation(QObject::tr("PLATES4 rotation"));
		static const QString format_shapefile(QObject::tr("ESRI shapefile"));
		static const QString format_gpml(QObject::tr("GPlates Markup Language"));
		static const QString format_gpml_gz(QObject::tr("Compressed GPML"));
		static const QString format_unknown(QObject::tr(""));
		
		switch ( GPlatesFileIO::get_feature_collection_file_format(qfileinfo) )
		{
		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_LINE:
			return format_line;

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_ROTATION:
			return format_rotation;

		case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
			return format_shapefile;

		case GPlatesFileIO::FeatureCollectionFileFormat::GPML:
			return format_gpml;

		case GPlatesFileIO::FeatureCollectionFileFormat::GPML_GZ:
			return format_gpml_gz;

		default:
			return format_unknown;
		}
	}
	
	QString
	get_output_filters_for_file(
			GPlatesFileIO::FileInfo &fileinfo,
			bool has_gzip)
	{
		// Appropriate filters for available output formats.
		// Note that since we cannot write Shapefiles yet, we use PLATES4 line format as
		// the default when the user clicks "Save a Copy" etc on shapefiles.
		static const QString filter_gmt(QObject::tr("GMT xy (*.xy)"));
		static const QString filter_line(QObject::tr("PLATES4 line (*.dat *.pla)"));
		static const QString filter_rotation(QObject::tr("PLATES4 rotation (*.rot)"));
		static const QString filter_shapefile(QObject::tr("ESRI shapefile (*.shp)"));
		static const QString filter_gpml(QObject::tr("GPlates Markup Language (*.gpml)"));
		static const QString filter_gpml_gz(QObject::tr("Compressed GPML (*.gpml.gz)"));
		static const QString filter_all(QObject::tr("All files (*)"));
		
		QFileInfo qfileinfo = fileinfo.get_qfileinfo();
		
		switch ( GPlatesFileIO::get_feature_collection_file_format(qfileinfo) )
		{
		case GPlatesFileIO::FeatureCollectionFileFormat::GMT:
			{
				QStringList filters;
				filters << filter_gmt;
				if (has_gzip) {
					filters << filter_gpml_gz;
				}
				filters << filter_line;
				filters << filter_gpml;
				filters << filter_all;
				return filters.join(";;");

			}
			break;

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_LINE:
			{
				QStringList filters;
				filters << filter_line;
				if (has_gzip) {
					filters << filter_gpml_gz;
				}
				filters << filter_gmt;
				filters << filter_gpml;
				filters << filter_all;
				return filters.join(";;");

			}
			break;

		case GPlatesFileIO::FeatureCollectionFileFormat::PLATES4_ROTATION:
			{
				QStringList filters;
				filters << filter_rotation;
				if (has_gzip) {
					filters << filter_gpml_gz;
				}
				filters << filter_gpml;
				filters << filter_all;
				return filters.join(";;");

			}
			break;
			
		case GPlatesFileIO::FeatureCollectionFileFormat::SHAPEFILE:
			{
				// No shapefile writing support yet! Write shapefiles to PLATES4 line files by default.
				QStringList filters;
				filters << filter_line;
				if (has_gzip) {
					filters << filter_gpml_gz;
				}
				filters << filter_gmt;
				filters << filter_gpml;
				filters << filter_all;
				return filters.join(";;");

			}
			break;

		case GPlatesFileIO::FeatureCollectionFileFormat::GPML:
			{
				QStringList filters;
				filters << filter_gpml;			// Save uncompressed by default, same as original
				if (has_gzip) {
					filters << filter_gpml_gz;	// Option to change to compressed version.
				}
				filters << filter_gmt;
				filters << filter_line;
				// FIXME: Only offer to save as PLATES4 .rot if feature collection
				// actually has rotations in it! Ditto with collections that have no features!
				filters << filter_rotation;
				filters << filter_all;
				return filters.join(";;");

			}
		case GPlatesFileIO::FeatureCollectionFileFormat::GPML_GZ:
			{
				QStringList filters;
				if (has_gzip) {
					filters << filter_gpml_gz;	// Save compressed by default, assuming we can.
				}
				filters << filter_gpml;			// Option to change to uncompressed version.
				filters << filter_gmt;
				filters << filter_line;
				// FIXME: Only offer to save as PLATES4 .rot if feature collection
				// actually has rotations in it! Ditto with collections that have no features!
				filters << filter_rotation;
				filters << filter_all;
				return filters.join(";;");
			
			}
			break;
			
		default:
			return filter_all;
		}

		return filter_all;
	}
}


GPlatesQtWidgets::ManageFeatureCollectionsDialog::ManageFeatureCollectionsDialog(
		ViewportWindow &viewport_window,
		QWidget *parent_):
	QDialog(parent_),
	d_viewport_window_ptr(&viewport_window),
	d_open_file_path(""),
	d_gzip_available(false)
{
	setupUi(this);
	
	// Try to adjust column widths.
	QHeaderView *header = table_feature_collections->horizontalHeader();
	header->setResizeMode(ColumnNames::FILENAME, QHeaderView::Stretch);
	header->resizeSection(ColumnNames::FORMAT, 128);
	header->resizeSection(ColumnNames::IN_USE, 56);
	header->resizeSection(ColumnNames::ACTIONS, 216);

	// Enforce minimum row height for the Actions widget's sake.
	QHeaderView *sider = table_feature_collections->verticalHeader();
	sider->setResizeMode(QHeaderView::Fixed);
	sider->setDefaultSectionSize(34);
	
	// Set up slots for Open File and Save All
	QObject::connect(button_open_file, SIGNAL(clicked()), this, SLOT(open_file()));
	QObject::connect(button_save_all, SIGNAL(clicked()), this, SLOT(save_all()));
	
	// Test if we can offer on-the-fly gzip compression.
	// FIXME: Ideally we should let the user know WHY we're concealing this option.
	// The user will still be able to type in a .gpml.gz file name and activate the
	// gzipped saving code, however this will produce an exception which pops up
	// a suitable message box (See ViewportWindow.cc)
	d_gzip_available = GPlatesFileIO::GpmlOnePointSixOutputVisitor::s_gzip_program.test();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::update()
{
	GPlatesAppState::ApplicationState *state = GPlatesAppState::ApplicationState::instance();
	
	GPlatesAppState::ApplicationState::file_info_iterator it = state->files_begin();
	GPlatesAppState::ApplicationState::file_info_iterator end = state->files_end();
	
	clear_rows();
	for (; it != end; ++it) {
		add_row(it);
	}
}

void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::edit_configuration(
	ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	// The "edit configuration" method only makes sense for shapefiles 
	// (until there is some sort of equivalent requirement for other types of 
	// feature collection), and as such, only shapefiles have the "edit configuration" 
	// icon enabled in the ManageFeatureCollectionsActionWidget. 
	//  
	// For shapefiles, "edit configuration" translates to "re-map shapefile attributes to model properties". 
	// 
	GPlatesAppState::ApplicationState::file_info_iterator file_it =
			action_widget_ptr->get_file_info_iterator();

	d_viewport_window_ptr->remap_shapefile_attributes(*file_it);
}

void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_file(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppState::ApplicationState::file_info_iterator file_it =
			action_widget_ptr->get_file_info_iterator();

	// Get the format to write feature collection in.
	// This is usually determined by file extension but some format also
	// require user preferences (eg, style of feature header in file).
	GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format =
		get_feature_collection_write_format(*file_it);
	
	try
	{
		// FIXME: Saving files should not be handled by the viewport window.
		d_viewport_window_ptr->save_file(
			*file_it,
			feature_collection_write_format);
		
	}
	catch (GPlatesFileIO::ErrorOpeningFileForWritingException &e)
	{
		QString message = tr("An error occurred while saving the file '%1'")
				.arg(e.filename());
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
				
	}
	catch (GPlatesFileIO::ErrorOpeningPipeToGzipException &e)
	{
		QString message = tr("GPlates was unable to use the '%1' program to save the file '%2'."
				" Please check that gzip is installed and in your PATH. You will still be able to save"
				" files without compression.")
				.arg(e.command())
				.arg(e.filename());
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
				
	}
	catch (GPlatesGlobal::UnexpectedEmptyFeatureCollectionException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		QString message = tr("Error: Attempted to write an empty feature collection.");
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch (GPlatesFileIO::FileFormatNotSupportedException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		QString message = tr("Error: Writing files in this format is currently not supported.");
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_file_as(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppState::ApplicationState::file_info_iterator file_it =
			action_widget_ptr->get_file_info_iterator();
	
	QString filename = QFileDialog::getSaveFileName(d_viewport_window_ptr, tr("Save File As"),
			file_it->get_qfileinfo().path(), get_output_filters_for_file(*file_it, d_gzip_available));
	if ( filename.isEmpty() ) {
		return;
	}
		
	// Make a new FileInfo object to tell save_file_as() what the new name should be.
	GPlatesFileIO::FileInfo new_fileinfo(filename);

	// Get the format to write feature collection in.
	// This is usually determined by file extension but some format also
	// require user preferences (eg, style of feature header in file).
	GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format =
		get_feature_collection_write_format(new_fileinfo);

	try
	{
		// FIXME: Saving files should not be handled by the viewport window.
		d_viewport_window_ptr->save_file_as(
			new_fileinfo,
			file_it,
			feature_collection_write_format);
		
	}
	catch (GPlatesFileIO::ErrorOpeningFileForWritingException &e)
	{
		QString message = tr("An error occurred while saving the file '%1'")
				.arg(e.filename());
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
				
	}
	catch (GPlatesFileIO::ErrorOpeningPipeToGzipException &e)
	{
		QString message = tr("GPlates was unable to use the '%1' program to save the file '%2'."
				" Please check that gzip is installed and in your PATH. You will still be able to save"
				" files without compression.")
				.arg(e.command())
				.arg(e.filename());
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
				
	}
	catch (GPlatesGlobal::UnexpectedEmptyFeatureCollectionException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		QString message = tr("Error: Attempted to write an empty feature collection.");
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch (GPlatesFileIO::FileFormatNotSupportedException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		QString message = tr("Error: Writing files in this format is currently not supported.");
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
	
	update();
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::save_file_copy(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppState::ApplicationState::file_info_iterator file_it =
			action_widget_ptr->get_file_info_iterator();
	
	QString filename = QFileDialog::getSaveFileName(d_viewport_window_ptr,
			tr("Save a copy of the file with a different name"),
			file_it->get_qfileinfo().path(), get_output_filters_for_file(*file_it, d_gzip_available));
	if ( filename.isEmpty() ) {
		return;
	}
		
	// Make a new FileInfo object to tell save_file_copy() what the copy name should be.
	GPlatesFileIO::FileInfo new_fileinfo(filename);

	// Get the format to write feature collection in.
	// This is usually determined by file extension but some format also
	// require user preferences (eg, style of feature header in file).
	GPlatesFileIO::FeatureCollectionWriteFormat::Format feature_collection_write_format =
		get_feature_collection_write_format(new_fileinfo);

	try
	{
		// FIXME: Saving files should not be handled by the viewport window.
		d_viewport_window_ptr->save_file_copy(
			new_fileinfo,
			file_it,
			feature_collection_write_format);
		
	}
	catch (GPlatesFileIO::ErrorOpeningFileForWritingException &e)
	{
		QString message = tr("An error occurred while saving the file '%1'")
				.arg(e.filename());
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
				
	}
	catch (GPlatesFileIO::ErrorOpeningPipeToGzipException &e)
	{
		QString message = tr("GPlates was unable to use the '%1' program to save the file '%2'."
				" Please check that gzip is installed and in your PATH. You will still be able to save"
				" files without compression.")
				.arg(e.command())
				.arg(e.filename());
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
				
	}
	catch (GPlatesGlobal::UnexpectedEmptyFeatureCollectionException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		QString message = tr("Error: Attempted to write an empty feature collection.");
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
	catch (GPlatesFileIO::FileFormatNotSupportedException &)
	{
		// The argument name in the above expression was removed to
		// prevent "unreferenced local variable" compiler warnings under MSVC
		QString message = tr("Error: Writing files in this format is currently not supported.");
		QMessageBox::critical(this, tr("Error Saving File"), message,
				QMessageBox::Ok, QMessageBox::Ok);
	}
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::reload_file(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppState::ApplicationState::file_info_iterator file_it =
			action_widget_ptr->get_file_info_iterator();
	
	// BIG FIXME: Unloading/Reloading files should be handled by ApplicationState, which should
	// then update all ViewportWindows' ViewState's active files list. However, we
	// don't have that built yet, so the current method of unloading files is to call
	// ViewportWindow and let it do the ApplicationState call.
	d_viewport_window_ptr->reload_file(file_it);
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::unload_file(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	GPlatesAppState::ApplicationState::file_info_iterator file_it =
			action_widget_ptr->get_file_info_iterator();
	
	// BIG FIXME: Unloading files should be handled by ApplicationState, which should
	// then update all ViewportWindows' ViewState's active files list. However, we
	// don't have that built yet, so the current method of unloading files is to call
	// ViewportWindow and let it do the ApplicationState call.
	d_viewport_window_ptr->deactivate_loaded_file(file_it);
	d_viewport_window_ptr->reconstruct();
	
	remove_row(action_widget_ptr);
}



void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::open_file()
{
	static const QString filters = tr(
			"All loadable files (*.dat *.pla *.rot *.shp *.gpml *.gpml.gz);;"
			"PLATES4 line (*.dat *.pla);;"
			"PLATES4 rotation (*.rot);;"
			"ESRI shapefile (*.shp);;"
			"GPlates Markup Language (*.gpml *.gpml.gz);;"
			"All files (*)" );
	
	QStringList filenames = QFileDialog::getOpenFileNames(d_viewport_window_ptr,
			tr("Open Files"), d_open_file_path, filters);
	
	if ( ! filenames.isEmpty() ) {
		d_viewport_window_ptr->load_files(filenames);
		d_viewport_window_ptr->reconstruct();
		
		QFileInfo last_opened_file(filenames.last());
		d_open_file_path = last_opened_file.path();
	}
}



void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::clear_rows()
{
	table_feature_collections->clearContents();	// Do not clear the header items as well.
	table_feature_collections->setRowCount(0);	// Do remove the newly blanked rows.
}


GPlatesQtWidgets::ManageFeatureCollectionsActionWidget *
GPlatesQtWidgets::ManageFeatureCollectionsDialog::add_row(
		GPlatesAppState::ApplicationState::file_info_iterator file_it)
{
	static const QString in_use_str(tr("Yes"));
	static const QString not_in_use_str(tr(""));

	// Obtain information from the FileInfo
	const QFileInfo &qfileinfo = file_it->get_qfileinfo();
	
	QString display_name = file_it->get_display_name(false);
	QString filename_str = qfileinfo.fileName();
	QString filepath_str = qfileinfo.path();
	QString format_str = get_format_for_file(qfileinfo);
	bool in_use = d_viewport_window_ptr->is_file_active(file_it);
	
	// Add blank row.
	int row = table_feature_collections->rowCount();
	table_feature_collections->insertRow(row);
	
	// Add filename item.
	QTableWidgetItem *filename_item = new QTableWidgetItem(display_name);
	filename_item->setToolTip(tr("Location: %1").arg(filepath_str));
	filename_item->setFlags(Qt::ItemIsEnabled);
	table_feature_collections->setItem(row, ColumnNames::FILENAME, filename_item);

	// Add file format item.
	QTableWidgetItem *format_item = new QTableWidgetItem(format_str);
	format_item->setFlags(Qt::ItemIsEnabled);
	table_feature_collections->setItem(row, ColumnNames::FORMAT, format_item);

	// Add in use status.
	QTableWidgetItem *in_use_item = new QTableWidgetItem();
	if (in_use)	{
		in_use_item->setText(in_use_str);
	} else {
		in_use_item->setText(not_in_use_str);
	}
	in_use_item->setFlags(Qt::ItemIsEnabled);
	in_use_item->setTextAlignment(Qt::AlignCenter);
	table_feature_collections->setItem(row, ColumnNames::IN_USE, in_use_item);
	
	// Add action buttons widget.
	ManageFeatureCollectionsActionWidget *action_widget_ptr =
			new ManageFeatureCollectionsActionWidget(*this, file_it, this);
	table_feature_collections->setCellWidget(row, ColumnNames::ACTIONS, action_widget_ptr);
	
	// Enable the edit_configuration button if we have a shapefile. 
	if (format_str == "ESRI shapefile")
	{
		action_widget_ptr->enable_edit_configuration_button();
	}

	return action_widget_ptr;
}


int
GPlatesQtWidgets::ManageFeatureCollectionsDialog::find_row(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	int row = 0;
	int end = table_feature_collections->rowCount();
	for (; row < end; ++row) {
		if (table_feature_collections->cellWidget(row, ColumnNames::ACTIONS) == action_widget_ptr) {
			return row;
		}
	}
	return end;
}


void
GPlatesQtWidgets::ManageFeatureCollectionsDialog::remove_row(
		ManageFeatureCollectionsActionWidget *action_widget_ptr)
{
	int row = find_row(action_widget_ptr);
	if (row < table_feature_collections->rowCount()) {
		table_feature_collections->removeRow(row);
	}
}

GPlatesFileIO::FeatureCollectionWriteFormat::Format
GPlatesQtWidgets::ManageFeatureCollectionsDialog::get_feature_collection_write_format(
	const GPlatesFileIO::FileInfo& file_info)
{
	switch (get_feature_collection_file_format(file_info) )
	{
	case GPlatesFileIO::FeatureCollectionFileFormat::GMT:
		{
			GMTHeaderFormatDialog gmt_header_format_dialog(d_viewport_window_ptr);
			gmt_header_format_dialog.exec();

			return gmt_header_format_dialog.get_header_format();
		}

	default:
		return GPlatesFileIO::FeatureCollectionWriteFormat::USE_FILE_EXTENSION;
	}
}
