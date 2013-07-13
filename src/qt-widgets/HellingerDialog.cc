 /* $Id: HellingerDialog.cc 260 2012-05-30 13:47:23Z robin.watson@ngu.no $ */

/**
 * \file
 * $Revision: 260 $
 * $Date: 2012-05-30 15:47:23 +0200 (Wed, 30 May 2012) $
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
#include <vector>

#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QMessageBox>
#include <QProgressBar>
#include <QTextStream>


#include "api/PythonInterpreterLocker.h"
#include "app-logic/ApplicationState.h"
#include "app-logic/UserPreferences.h"
#include "file-io/HellingerReader.h"
#include "file-io/HellingerWriter.h"
#include "global/CompilerWarnings.h"
#include "maths/PointOnSphere.h"
#include "presentation/ViewState.h"
#include "view-operations/RenderedGeometryFactory.h"
#include "HellingerDialog.h"
#include "HellingerEditPointDialog.h"
#include "HellingerEditSegmentDialog.h"
#include "HellingerNewPoint.h"
#include "HellingerStatsDialog.h"
#include "HellingerThread.h"
#include "ReadErrorAccumulationDialog.h"

#include "QtWidgetUtils.h"


const double SLIDER_MULTIPLIER = -10000.;
const int SYMBOL_SIZE = 2;

namespace{

	enum PickColumns
	{
		SEGMENT_NUMBER,
		SEGMENT_TYPE,
		LAT,
		LON,
		UNCERTAINTY
	};

	void
	add_pick_to_segment(
			QTreeWidgetItem *parent_item,
			const int &segment_number,
			const GPlatesQtWidgets::HellingerPick &pick)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(SEGMENT_NUMBER, QString::number(segment_number));
		item->setText(SEGMENT_TYPE, QString::number(pick.d_segment_type));
		item->setText(LAT, QString::number(pick.d_lat));
		item->setText(LON, QString::number(pick.d_lon));
		item->setText(UNCERTAINTY, QString::number(pick.d_uncertainty));
		parent_item->addChild(item);

		if (!pick.d_is_enabled)
		{
			item->setTextColor(SEGMENT_NUMBER,Qt::gray);
			item->setTextColor(SEGMENT_TYPE,Qt::gray);
			item->setTextColor(LAT,Qt::gray);
			item->setTextColor(LON,Qt::gray);
			item->setTextColor(UNCERTAINTY,Qt::gray);
		}
	}

	void
	add_pick_to_tree(
			const int &segment_number,
			const GPlatesQtWidgets::HellingerPick &pick,
			QTreeWidget *tree)
	{
		QString segment_as_string = QString::number(segment_number);
		QList<QTreeWidgetItem*> items = tree->findItems(
					segment_as_string, Qt::MatchExactly, 0);
		QTreeWidgetItem *item;
		if (items.isEmpty())
		{
			item = new QTreeWidgetItem(tree);
			item->setText(0, segment_as_string);

		}
		else
		{
			item = items.at(0);
		}
		add_pick_to_segment(item, segment_number, pick);
	}
}

GPlatesQtWidgets::HellingerDialog::HellingerDialog(
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ReadErrorAccumulationDialog &read_error_accumulation_dialog,
		QWidget *parent_):
	GPlatesDialog(
		parent_,
		Qt::Window),
		//Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_view_state(view_state),
	d_hellinger_layer(*view_state.get_rendered_geometry_collection().get_main_rendered_layer(
						  GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_TOOL_LAYER)),
	d_read_error_accumulation_dialog(read_error_accumulation_dialog),
	d_hellinger_model(0),
	d_hellinger_stats_dialog(0),
	d_hellinger_thread(0),
	d_moving_plate_id(0),
	d_fixed_plate_id(0),
	d_recon_time(0.),
	d_chron_time(0.),
	d_moving_symbol(GPlatesGui::Symbol::CROSS, SYMBOL_SIZE, true),
	d_fixed_symbol(GPlatesGui::Symbol::SQUARE, SYMBOL_SIZE, false),
	d_thread_type(POLE_THREAD_TYPE)
{
	setupUi(this);

	d_hellinger_thread = new HellingerThread(this, d_hellinger_model);

	set_up_connections();

	//FIXME: think about when we should deactivate this layer....and/or do we make it an orthogonal layer?
	d_view_state.get_rendered_geometry_collection().set_main_layer_active(
				GPlatesViewOperations::RenderedGeometryCollection::HELLINGER_TOOL_LAYER);
	d_hellinger_layer.set_active();

	// Path copied from PythonUtils / PythonManager.

	// Look in system-specific locations for supplied sample scripts, site-specific scripts, etc.
	// The default location will be platform-dependent and is currently set up in UserPreferences.cc.
	// d_python_path = d_view_state.get_application_state().get_user_preferences().get_value("paths/python_system_script_dir").toString();
	// d_python_path = "scripts";

	// Temporary path during development
	d_python_path = "/home/robin/Desktop/Hellinger/scripts";

	qDebug() << "python path: " << d_python_path;

	d_python_path.append(QDir::separator());
	d_python_file = d_python_path + "py_hellinger.py";
	d_temporary_path = d_python_path;

	d_temp_pick_file = QString("temp_file");
	d_temp_result = QString("temp_file_temp_result");
	d_temp_par = QString("temp_file_par");
	d_temp_res = QString("temp_file_res");

	d_hellinger_model = new HellingerModel(d_python_path);

	progress_bar->setEnabled(false);
	progress_bar->setMinimum(0.);
	progress_bar->setMaximum(1.);
	progress_bar->setValue(0.);

	update_from_model();

	// For eventual insertion of generated pole into the model.
	groupbox_rotation->hide();

	// For eventual interruption of the python thread.
	button_cancel->hide();

	QStringList labels;
	labels << "Segment" << "Moving(1)/Fixed(2)" << "Latitude" << "Longitude" << "Uncertainty (km)";
	tree_widget_picks->setHeaderLabels(labels);


	tree_widget_picks->header()->resizeSection(SEGMENT_NUMBER,90);
	tree_widget_picks->header()->resizeSection(SEGMENT_TYPE,150);
	tree_widget_picks->header()->resizeSection(LAT,90);
	tree_widget_picks->header()->resizeSection(LON,90);
	tree_widget_picks->header()->resizeSection(UNCERTAINTY,90);
}

void
GPlatesQtWidgets::HellingerDialog::handle_selection_changed(
		const QItemSelection & new_selection,
		const QItemSelection & old_selection)
{
	//TODO: Refactor this method
	qDebug() << "Selection changed.";

	if (new_selection.empty())
	{
		// Nothing selected:
		qDebug() << "Nothing selected";
		set_buttons_for_no_selection();
	}
	else if (tree_widget_picks->currentItem()->text(1).isEmpty()) // Segment selected
	{
		qDebug() << "Segment selected";
		set_buttons_for_segment_selected();
	}
	else // pick selected
	{
		qDebug() << "Pick selected";
		const QModelIndex index = tree_widget_picks->selectionModel()->currentIndex();
		QString segment = tree_widget_picks->currentItem()->text(0);
		int row = index.row();
		int segment_int = segment.toInt();
		bool state = d_hellinger_model->get_pick_state(segment_int, row);

		set_buttons_for_pick_selected(state);
	}

	// if we have selected a pick:
	//		get its state and update the enable/disable buttons
	//		enable the edit/remove-pick buttons
	//		disable the edit/remove segment buttons

	// If we have selected a segment
	//		disable both enable/disable buttons
	//		enable the edit/remove semgnet buttons
	//		disable the edit/remove pick buttons

	// If nothing is selected:
	//	 disable everything (except the new pick / new segment buttons - which are always enabled anyway)


	// Update the highlighted (if any) point. Begin by resetting the hellinger layer
	// on the canvas.
	d_hellinger_layer.clear_rendered_geometries();
	update_canvas();

	if (!tree_widget_picks->currentItem()->text(1).isEmpty())
	{
		double lat = tree_widget_picks->currentItem()->text(2).toDouble();
		double lon = tree_widget_picks->currentItem()->text(3).toDouble();
		HellingerPickType type =
				static_cast<HellingerPickType>(tree_widget_picks->currentItem()->text(1).toInt());
		highlight_selected_point(lat, lon, type);
	}

}

void GPlatesQtWidgets::HellingerDialog::handle_cancel()
{
// TODO: This is where we would (if we can) interrupt the thread running the python code.
}

void
GPlatesQtWidgets::HellingerDialog::highlight_selected_point(
		const double &lat,
		const double &lon,
		const int &type_segment)
{

	GPlatesGui::Symbol moving_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CROSS, SYMBOL_SIZE, true);
	GPlatesGui::Symbol fixed_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::SQUARE, SYMBOL_SIZE, false);
	GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
				GPlatesMaths::LatLonPoint(lat,lon));
	GPlatesViewOperations::RenderedGeometry pick_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				point.get_non_null_pointer(),
				GPlatesGui::Colour::get_yellow(),
				2, /* point thickness */
				2, /* line thickness */
				false /* fill */,
				type_segment == MOVING_PICK_TYPE ? moving_symbol : fixed_symbol);

	d_hellinger_layer.add_rendered_geometry(pick_geometry);

}

void
GPlatesQtWidgets::HellingerDialog::handle_pick_state_changed()
{
	store_expanded_status();
	// TODO: Refactor this method.
	const QModelIndex index = tree_widget_picks->selectionModel()->currentIndex();
	QString segment = tree_widget_picks->currentItem()->text(0);
	int row = index.row();
	int segment_int = segment.toInt();
	QStringList get_data_line = d_hellinger_model->get_pick_as_string(segment_int, row);
	QStringList data_to_model;
	QString segment_str = get_data_line.at(0);
	QString move_fix = get_data_line.at(1);
	QString lat = get_data_line.at(2);
	QString lon = get_data_line.at(3);
	QString uncert = get_data_line.at(4);

	bool state = d_hellinger_model->get_pick_state(segment_int, row);
	if (state)
	{
		if (move_fix == "1")
		{
			move_fix = QString("%1").arg(DISABLED_MOVING_PICK_TYPE);
			data_to_model << move_fix << segment_str << lat << lon << uncert;
			d_hellinger_model->remove_pick(segment_int,row);
			d_hellinger_model->add_pick(data_to_model);
			update_from_model();
		}
		else if (move_fix == "2")
		{
			move_fix = QString("%1").arg(DISABLED_FIXED_PICK_TYPE);
			data_to_model << move_fix << segment_str << lat << lon << uncert;
			d_hellinger_model->remove_pick(segment_int,row);
			d_hellinger_model->add_pick(data_to_model);
			update_from_model();
		}

	}
	else if (!state)
	{
		QString change_state = "0";
		data_to_model << move_fix << segment_str << lat << lon << uncert;
		//        qDebug()<<data_to_model<<change_state;
		d_hellinger_model->remove_pick(segment_int,row);
		d_hellinger_model->add_pick(data_to_model);
		update_from_model();
	}
	restore_expanded_status();

}

void
GPlatesQtWidgets::HellingerDialog::handle_edit_point()
{

	QScopedPointer<GPlatesQtWidgets::HellingerEditPointDialog> dialog(
			new GPlatesQtWidgets::HellingerEditPointDialog(this,d_hellinger_model));

	const QModelIndex index = tree_widget_picks->selectionModel()->currentIndex();
	QString segment = tree_widget_picks->currentItem()->text(0);
	int row = index.row();
	int segment_int = segment.toInt();

	dialog->initialise_with_pick(segment_int, row);
	dialog->exec();
	restore_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::handle_edit_segment()
{

	QScopedPointer<GPlatesQtWidgets::HellingerEditSegmentDialog> dialog(
				new GPlatesQtWidgets::HellingerEditSegmentDialog(this,d_hellinger_model,false /* create new segment */));

	QString segment = tree_widget_picks->currentItem()->text(SEGMENT_NUMBER);
	int segment_number = segment.toInt();

	qDebug() << "Seg number" << segment_number;
	dialog->initialise_with_segment(
				d_hellinger_model->get_segment(segment_number),segment_number);

	dialog->exec();
	restore_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::handle_remove_point()
{	
	QMessageBox message_box;
	message_box.setIcon(QMessageBox::Warning);
	message_box.setWindowTitle(tr("Remove point"));
	message_box.setText(
				tr("Are you sure you want to remove the point?"));
	message_box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
	message_box.setDefaultButton(QMessageBox::Ok);
	int ret = message_box.exec();

	if (ret == QMessageBox::Cancel)
	{
		return;
	}
	else
	{
		const QModelIndex index = tree_widget_picks->selectionModel()->currentIndex();
		QString segment = tree_widget_picks->currentItem()->text(0);
		int row = index.row();
		int segment_int = segment.toInt();
		d_hellinger_model->remove_pick(segment_int, row);
		update_from_model();
		restore_expanded_status();
	}
}

void
GPlatesQtWidgets::HellingerDialog::handle_remove_segment()
{
	QMessageBox message_box;
	message_box.setIcon(QMessageBox::Warning);
	message_box.setWindowTitle(tr("Remove segment"));
	message_box.setText(
				tr("Are you sure you want to remove the segment?"));
	message_box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
	message_box.setDefaultButton(QMessageBox::Ok);
	int ret = message_box.exec();

	if (ret == QMessageBox::Cancel)
	{
		return;
	}
	else
	{
		QString segment = tree_widget_picks->currentItem()->text(0);
		int segment_int = segment.toInt();
		d_hellinger_model->remove_segment(segment_int);
		button_renumber->setEnabled(true);
		update_from_model();
		restore_expanded_status();
	}
}

void
GPlatesQtWidgets::HellingerDialog::initialise()
{
	update_buttons();
	update_from_model();
	restore_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::import_hellinger_file()
{
	QString filters;
	filters = QObject::tr("Hellinger pick file (*.pick)");
	filters += ";;";
	filters += QObject::tr("Hellinger com file (*.com)");
	filters += ";;";

	QString active_filter = QObject::tr("All Hellinger files (*.pick *.com)");
	filters += active_filter;

	QString path = QFileDialog::getOpenFileName(
				this,
				QObject::tr("Open Hellinger .pick or .com file"),
				d_view_state.get_last_open_directory(),
				filters,
				&active_filter);

	if (path.isEmpty())
	{
		return;
	}
	QFile file(path);
	QFileInfo file_info(file.fileName());
	QStringList file_name = file_info.fileName().split(".", QString::SkipEmptyParts);
	QString type_file = file_name.at(1);

	d_hellinger_model->reset_model();



	GPlatesFileIO::ReadErrorAccumulation &read_errors = d_read_error_accumulation_dialog.read_errors();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_initial_errors = read_errors.size();
	if (type_file == "pick")
	{
		GPlatesFileIO::HellingerReader::read_pick_file(path,*d_hellinger_model,read_errors);
	}
	else if (type_file == "com")
	{
		GPlatesFileIO::HellingerReader::read_com_file(path,*d_hellinger_model,read_errors);
	}


	d_read_error_accumulation_dialog.update();
	GPlatesFileIO::ReadErrorAccumulation::size_type num_final_errors = read_errors.size();
	if (num_initial_errors != num_final_errors)
	{
		d_read_error_accumulation_dialog.show();
	}

	line_import_file->setText(path);
	d_hellinger_layer.clear_rendered_geometries();

	update_buttons();
	update_from_model();
	initialise();
	handle_expand_all();

}

void
GPlatesQtWidgets::HellingerDialog::handle_spinbox_radius_changed()
{
	button_calculate_fit->setEnabled(spinbox_radius->value() > 0.0);
}

void
GPlatesQtWidgets::HellingerDialog::update_initial_guess()
{
	boost::optional<GPlatesQtWidgets::HellingerComFileStructure> com_file_data = d_hellinger_model->get_com_file();

	if (com_file_data)
	{
		spinbox_lat->setValue(com_file_data.get().d_lat);
		spinbox_lon->setValue(com_file_data.get().d_lon);
		spinbox_rho->setValue(com_file_data.get().d_rho);
		spinbox_radius->setValue(com_file_data.get().d_search_radius);
		checkbox_grid_search->setChecked(com_file_data.get().d_perform_grid_search);
		spinbox_sig_level->setValue(com_file_data.get().d_significance_level);

		checkbox_kappa->setChecked(com_file_data.get().d_estimate_kappa);
		checkbox_graphics->setChecked(com_file_data.get().d_generate_output_files);

		d_filename_dat = com_file_data.get().d_data_filename;
		d_filename_up = com_file_data.get().d_up_filename;
		d_filename_down = com_file_data.get().d_down_filename;
	}

}

void
GPlatesQtWidgets::HellingerDialog::handle_calculate_stats()
{
	d_thread_type = STATS_THREAD_TYPE;
	button_stats->setEnabled(false);
	d_hellinger_thread->initialise_stats_calculation(
				d_path,
				d_file_name,
				d_filename_dat,
				d_filename_up,
				d_filename_down,
				d_python_file,
				d_temporary_path,
				d_temp_pick_file,
				d_temp_result,
				d_temp_par,
				d_temp_res);
	d_hellinger_thread->set_python_script_type(d_thread_type);
	progress_bar->setEnabled(true);
	progress_bar->setMaximum(0.);
	d_hellinger_thread->start();
	button_cancel->setEnabled(true);
}

void
GPlatesQtWidgets::HellingerDialog::handle_export_pick_file()
{
	QString file_name = QFileDialog::getSaveFileName(this, tr("Save File"), "",
													 tr("Hellinger Pick Files (*.pick);"));
	
	if (!file_name.isEmpty())
	{
		GPlatesFileIO::HellingerWriter::write_pick_file(file_name,*d_hellinger_model,true /*export disabled poles */);
	}
}

void
GPlatesQtWidgets::HellingerDialog::handle_export_com_file()
{
	QString file_name = QFileDialog::getSaveFileName(this,tr("Save settings file"),"",
													 tr("Hellinger .com files (*.com);"));

	if (!file_name.isEmpty())
	{
		// Update Hellinger model with data from UI
		update_model_with_com_data();

		GPlatesFileIO::HellingerWriter::write_com_file(file_name,*d_hellinger_model);
	}
}

void
GPlatesQtWidgets::HellingerDialog::show_stat_details()
{
	if (!d_hellinger_stats_dialog)
	{
		d_hellinger_stats_dialog = new GPlatesQtWidgets::HellingerStatsDialog(d_python_path,this);
	}
	d_hellinger_stats_dialog->update();
	d_hellinger_stats_dialog->show();
}

void
GPlatesQtWidgets::HellingerDialog::handle_add_new_point()
{    
	QScopedPointer<GPlatesQtWidgets::HellingerEditPointDialog> dialog(
				new GPlatesQtWidgets::HellingerEditPointDialog(this,d_hellinger_model,true));
	dialog->exec();
	restore_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::handle_add_new_segment()
{
	QScopedPointer<GPlatesQtWidgets::HellingerEditSegmentDialog> dialog(
				new GPlatesQtWidgets::HellingerEditSegmentDialog(this,
																d_hellinger_model,
																true /*create new segment */));

	dialog->exec();
	restore_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::handle_calculate_fit()
{        
	// TODO: Refactor this method.
	// FIXME: This assumes that the state of the button always reflects the ordered state of the picks in the model. Check
	// that this is indeed the case.
	if (button_renumber->isEnabled())
	{
		QMessageBox message_box;
		message_box.setIcon(QMessageBox::Information);
		message_box.setWindowTitle(tr("Segment ordering"));
		message_box.setText(
					tr("The segments are not currently ordered. Press OK to reorder the segments and continue with the calculation."));
		message_box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
		message_box.setDefaultButton(QMessageBox::Ok);
		int ret = message_box.exec();

		if (ret == QMessageBox::Cancel)
		{
			return;
		}
		else
		{
			reorder_picks();
		}

	}

	if (!(spinbox_rho->value() > 0))
	{
		QMessageBox::critical(this,tr("Initial guess values"),
							  tr("The value of rho in the initial guess is zero. Please enter a non-zero value"),
							  QMessageBox::Ok,QMessageBox::Ok);
		return;
	}

	QFile python_code(d_python_file);
	if (python_code.exists())
	{
		QString path = d_python_path + d_temp_pick_file;
		GPlatesFileIO::HellingerWriter::write_pick_file(path,*d_hellinger_model,false);
		QString import_file_line = line_import_file->text();
		// TODO: check if we actually need to update the buttons here.
		update_buttons();

		// TODO: is there a cleaner way of sending input data to python?
		// Can we use a bitset or a vector of bools instead of std::vector<int> for example?
		std::vector<double> input_data;
		std::vector<int> bool_data;
		input_data.push_back(spinbox_lat->value());
		input_data.push_back(spinbox_lon->value());
		input_data.push_back(spinbox_rho->value());
		input_data.push_back(spinbox_radius->value());
		input_data.push_back(spinbox_sig_level->value());
		int iteration;

		if (checkbox_grid_search->isChecked())
		{
			iteration = spinbox_iteration->value();
			bool_data.push_back(1);

		}
		else
		{
			iteration = 0;
			bool_data.push_back(0);
		}

		if (checkbox_kappa->isChecked())
		{
			bool_data.push_back(1);
		}
		else
		{
			bool_data.push_back(0);
		}

		if (checkbox_graphics->isChecked())
		{
			bool_data.push_back(1);
		}
		else
		{
			bool_data.push_back(0);
		}

		d_hellinger_thread->initialise_pole_calculation(
					import_file_line,
					input_data,
					bool_data,
					iteration,
					d_python_file,
					d_temporary_path,
					d_temp_pick_file,
					d_temp_result,
					d_temp_par,
					d_temp_res);
		d_thread_type = POLE_THREAD_TYPE;

		update_canvas();
		d_hellinger_thread->set_python_script_type(d_thread_type);

		progress_bar->setEnabled(true);
		progress_bar->setMaximum(0.);
		d_hellinger_thread->start();
		button_cancel->setEnabled(true);
	}
	else
	{
		QString message;
		QTextStream(&message) << tr("The Hellinger python scripts could not be found.");
		QMessageBox::critical(this,tr("Python scripts not found"),message,QMessageBox::Ok,QMessageBox::Ok);
		qWarning() << message;
	}
}

void
GPlatesQtWidgets::HellingerDialog::handle_thread_finished()
{
	progress_bar->setEnabled(false);
	progress_bar->setMaximum(1.);
	if (d_thread_type == POLE_THREAD_TYPE)
	{
		QString path = d_python_path + "temp_file_temp_result";
		QFile data_file(path);
		if (data_file.open(QFile::ReadOnly))
		{
			QTextStream in(&data_file);
			QString line = in.readLine();
			QStringList fields = line.split(" ",QString::SkipEmptyParts);
			d_hellinger_model->set_fit(fields);
			data_file.close();
			update_result();
			button_stats->setEnabled(true);
		}
	}
	else if(d_thread_type == STATS_THREAD_TYPE)
	{
		button_stats->setEnabled(false);

		d_hellinger_model->get_error_ellipse_points();
		draw_error_ellipse();
	}
}

void
GPlatesQtWidgets::HellingerDialog::update_buttons()
{
	// Update based on if we have some picks loaded or not.
	if (!picks_loaded())
	{
		button_expand_all->setEnabled(false);
		button_collapse_all->setEnabled(false);
		button_export_pick_file->setEnabled(false);
		button_export_com_file->setEnabled(false);
		button_calculate_fit->setEnabled(false);
		button_details->setEnabled(false);
		button_remove_segment->setEnabled(false);
		button_remove_point->setEnabled(false);
		button_stats->setEnabled(false);
	}
	else
	{
		button_expand_all->setEnabled(true);
		button_collapse_all->setEnabled(true);
		button_export_pick_file->setEnabled(true);
		button_export_com_file->setEnabled(true);
		button_calculate_fit->setEnabled(spinbox_radius->value() > 0.0);
	}
}

void
GPlatesQtWidgets::HellingerDialog::update_from_model()
{
	tree_widget_picks->clear();

	load_data_from_model();
	update_canvas();
	update_buttons();
}


void
GPlatesQtWidgets::HellingerDialog::update_buttons_statistics(bool info)
{
	if (info)
	{
		spinbox_result_lat -> setEnabled(true);
		spinbox_result_lon -> setEnabled(true);
		spinbox_result_angle -> setEnabled(true);
		//        spinbox_result_eps -> setEnabled(true);
		button_details->setEnabled(true);

	}
	else
	{
		button_details->setEnabled(false);
	}
}

void
GPlatesQtWidgets::HellingerDialog::draw_pole(
		const double &lat,
		const double &lon)
{
	GPlatesGui::Symbol results_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CIRCLE, 2, true);
	GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
				GPlatesMaths::LatLonPoint(lat,lon));
	GPlatesViewOperations::RenderedGeometry pick_results =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				point.get_non_null_pointer(),
				GPlatesGui::Colour::get_red(),
				2, /* point thickness */
				2, /* line thickness */
				false /* fill */,
				results_symbol);

	d_hellinger_layer.add_rendered_geometry(pick_results);
}


void
GPlatesQtWidgets::HellingerDialog::update_result()
{
	// FIXME: sort out the sequence of calling update_canvas, update_result etc... this function (update_result) is getting
	// called 5 times after a fit is completed. On the second call, the lon is always zero for some reason. Investigate.
	boost::optional<GPlatesQtWidgets::HellingerFitStructure> data_fit_struct = d_hellinger_model->get_fit();

	if (data_fit_struct)
	{
		qDebug() << "Drawing pole at " << data_fit_struct.get().d_lat << ", " << data_fit_struct.get().d_lon;
		spinbox_result_lat->setValue(data_fit_struct.get().d_lat);
		spinbox_result_lon->setValue(data_fit_struct.get().d_lon);
		spinbox_result_angle->setValue(data_fit_struct.get().d_angle);
		draw_pole(data_fit_struct.get().d_lat, data_fit_struct.get().d_lon);
	}

}

void
GPlatesQtWidgets::HellingerDialog::draw_error_ellipse()
{
	const std::vector<GPlatesMaths::LatLonPoint> &data_points = d_hellinger_model->get_error_ellipse_points();
	if (!data_points.empty())
	{
		std::vector<GPlatesMaths::LatLonPoint>::const_iterator iter;

		for (iter = data_points.begin(); iter != data_points.end(); ++iter)
		{
			GPlatesMaths::LatLonPoint llp = *iter;
			double lat = llp.latitude();
			double lon = llp.longitude();
			GPlatesGui::Symbol results_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CROSS, SYMBOL_SIZE, true);
			GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
						GPlatesMaths::LatLonPoint(lat,lon));
			GPlatesViewOperations::RenderedGeometry pick_results =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
						point.get_non_null_pointer(),
						GPlatesGui::Colour::get_red(),
						2, /* point thickness */
						2, /* line thickness */
						false /* fill */,
						results_symbol);
			d_hellinger_layer.add_rendered_geometry(pick_results);
		}
	}

}


void
GPlatesQtWidgets::HellingerDialog::load_data_from_model()
{    

	hellinger_model_type::const_iterator
			iter = d_hellinger_model->begin(),
			end = d_hellinger_model->end();

	for (; iter != end ; ++iter)
	{
		add_pick_to_tree(iter->first,iter->second,tree_widget_picks);
	}
	update_buttons();
	update_initial_guess();
}

void
GPlatesQtWidgets::HellingerDialog::create_feature_collection()
{

}

void
GPlatesQtWidgets::HellingerDialog::close_dialog()
{
	d_hellinger_layer.clear_rendered_geometries();
}

void
GPlatesQtWidgets::HellingerDialog::update_canvas()
{
	d_hellinger_layer.clear_rendered_geometries();
	draw_fixed_picks();
	draw_moving_picks();
	update_result();
	draw_error_ellipse();

}


void
GPlatesQtWidgets::HellingerDialog::set_segment_colours(
		int num_colour)
{
	num_colour = num_colour%7;
	switch (num_colour)
	{
	case 0:
		d_segment_colour = GPlatesGui::Colour::get_green();
		break;
	case 1:
		d_segment_colour = GPlatesGui::Colour::get_blue();
		break;
	case 2:
		d_segment_colour = GPlatesGui::Colour::get_maroon();
		break;
	case 3:
		d_segment_colour = GPlatesGui::Colour::get_purple();
		break;
	case 4:
		d_segment_colour = GPlatesGui::Colour::get_fuchsia();
		break;
	case 5:
		d_segment_colour = GPlatesGui::Colour::get_olive();
		break;
	case 6:
		d_segment_colour = GPlatesGui::Colour::get_navy();
		break;
	default:
		d_segment_colour = GPlatesGui::Colour::get_navy();
	}
}

void
GPlatesQtWidgets::HellingerDialog::draw_fixed_picks()
{
	hellinger_model_type::const_iterator it = d_hellinger_model->begin();

	int num_segment = 0;
	int num_colour = 0;

	for (; it != d_hellinger_model->end(); ++it)
	{
		if (it->second.d_is_enabled)
		{
			if (num_segment != it->first)
			{
				++num_colour;
				++num_segment;
			}

			set_segment_colours(num_colour);

			if (it->second.d_segment_type == FIXED_PICK_TYPE)
			{
				GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
							GPlatesMaths::LatLonPoint(it->second.d_lat,it->second.d_lon));

				GPlatesViewOperations::RenderedGeometry pick_geometry =
						GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
							point.get_non_null_pointer(),
							d_segment_colour,
							2, /* point thickness */
							2, /* line thickness */
							false /* fill */,
							it->second.d_segment_type == MOVING_PICK_TYPE ? d_moving_symbol : d_fixed_symbol);

				d_hellinger_layer.add_rendered_geometry(pick_geometry);
			}
		}
	}
}

void
GPlatesQtWidgets::HellingerDialog::draw_moving_picks()
{
	hellinger_model_type::const_iterator it = d_hellinger_model->begin();
	int num_segment = 0;
	int num_colour = 0;
	for (; it != d_hellinger_model->end(); ++it)
	{
		if (it->second.d_is_enabled)
		{
			if (num_segment != it->first)
			{
				++num_colour;
				++num_segment;
			}

			set_segment_colours(num_colour);

			if (it->second.d_segment_type == MOVING_PICK_TYPE)
			{
				GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
							GPlatesMaths::LatLonPoint(it->second.d_lat,it->second.d_lon));

				GPlatesViewOperations::RenderedGeometry pick_geometry =
						GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
							point.get_non_null_pointer(),
							d_segment_colour,
							2, /* point thickness */
							2, /* line thickness */
							false /* fill */,
							it->second.d_segment_type == MOVING_PICK_TYPE ? d_moving_symbol : d_fixed_symbol);

				d_hellinger_layer.add_rendered_geometry(pick_geometry);
			}
		}
	}

}

void
GPlatesQtWidgets::HellingerDialog::reconstruct_picks()
{

	d_hellinger_layer.clear_rendered_geometries();
	draw_fixed_picks();
	update_result();
	boost::optional<GPlatesQtWidgets::HellingerFitStructure> data_fit_struct = d_hellinger_model->get_fit();

	double recon_time = spinbox_recon_time->value();

	double lat = data_fit_struct.get().d_lat;
	double lon = data_fit_struct.get().d_lon;
	double angle;
	if (recon_time > 0 )
	{
		angle = (recon_time/spinbox_chron->value())*data_fit_struct.get().d_angle;
		double convert_angle = GPlatesMaths::convert_deg_to_rad(angle);
		hellinger_model_type::const_iterator it = d_hellinger_model->begin();

		GPlatesMaths::LatLonPoint llp(lat,lon);
		GPlatesMaths::PointOnSphere point = make_point_on_sphere(llp);

		GPlatesMaths::FiniteRotation rotation =
				GPlatesMaths::FiniteRotation::create(point,convert_angle);
		int num_segment = 0;
		int num_colour = 0;
		for (; it != d_hellinger_model->end(); ++it)
		{
			if (it->second.d_is_enabled)
			{
				if (num_segment != it->first)
				{
					++num_colour;
					++num_segment;
				}

				set_segment_colours(num_colour);

				if (it->second.d_segment_type == MOVING_PICK_TYPE)
				{

					GPlatesMaths::LatLonPoint llp_move(it->second.d_lat,it->second.d_lon);
					GPlatesMaths::PointOnSphere point_move = make_point_on_sphere(llp_move);
					GPlatesMaths::PointOnSphere rotated_point = rotation*point_move;
					GPlatesMaths::LatLonPoint transform_llp = make_lat_lon_point(rotated_point);

					GPlatesMaths::PointOnSphere point_sphere = GPlatesMaths::make_point_on_sphere(
								GPlatesMaths::LatLonPoint(transform_llp.latitude(),transform_llp.longitude()));

					GPlatesViewOperations::RenderedGeometry pick_geometry =
							GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
								point_sphere.get_non_null_pointer(),
								d_segment_colour,
								2, /* point thickness */
								2, /* line thickness */
								false /* fill */,
								it->second.d_segment_type == MOVING_PICK_TYPE ? d_moving_symbol : d_fixed_symbol);

					d_hellinger_layer.add_rendered_geometry(pick_geometry);
				}
			}
		}
	}
	else
	{
		draw_moving_picks();
	}


}

bool GPlatesQtWidgets::HellingerDialog::picks_loaded()
{
	return (tree_widget_picks->topLevelItemCount() != 0);
}

void GPlatesQtWidgets::HellingerDialog::set_buttons_for_no_selection()
{
	button_activate_pick->setEnabled(false);
	button_deactivate_pick->setEnabled(false);
	button_edit_point->setEnabled(false);
	button_edit_segment->setEnabled(false);
	button_remove_point->setEnabled(false);
	button_remove_segment->setEnabled(false);
}

void GPlatesQtWidgets::HellingerDialog::set_buttons_for_segment_selected()
{
	button_activate_pick->setEnabled(false);
	button_deactivate_pick->setEnabled(false);
	button_edit_point->setEnabled(false);
	button_edit_segment->setEnabled(true);
	button_remove_point->setEnabled(false);
	button_remove_segment->setEnabled(true);
}

void GPlatesQtWidgets::HellingerDialog::set_buttons_for_pick_selected(
		bool state_is_active)
{
	button_activate_pick->setEnabled(!state_is_active);
	button_deactivate_pick->setEnabled(state_is_active);
	button_edit_point->setEnabled(true);
	button_edit_segment->setEnabled(false);
	button_remove_point->setEnabled(true);
	button_remove_segment->setEnabled(false);

}

void
GPlatesQtWidgets::HellingerDialog::handle_chron_time_changed(
		const double &time)
{
	d_chron_time = spinbox_chron->value();
	slider_recon_time->setMinimum(d_chron_time*SLIDER_MULTIPLIER);
	slider_recon_time->setMaximum(0.);

	spinbox_recon_time->setMaximum(time);

	if (d_recon_time > d_chron_time)
	{
		d_recon_time = d_chron_time;

	}
	d_hellinger_layer.clear_rendered_geometries();

	draw_fixed_picks();
	draw_moving_picks();
	update_result();

}

void
GPlatesQtWidgets::HellingerDialog::handle_recon_time_spinbox_changed(
		const double &time)
{
	slider_recon_time->setValue(SLIDER_MULTIPLIER*time);

	reconstruct_picks();
}

void
GPlatesQtWidgets::HellingerDialog::handle_recon_time_slider_changed(
		const int &value)
{
	spinbox_recon_time->setValue(static_cast<double>(value)/SLIDER_MULTIPLIER);
}

void
GPlatesQtWidgets::HellingerDialog::handle_fit_spinboxes_changed()
{
	//TODO: think about if we want this behaviour - i.e. allowing a user to changed
	// the "fit result" values and the corresponding display. I think I will disable
	// this for now.

	GPlatesQtWidgets::HellingerFitStructure fit(spinbox_result_lat->value(),
											   spinbox_result_lon->value(),
											   spinbox_result_angle->value());
	d_hellinger_model->set_fit(fit);
	update_canvas();
}

void
GPlatesQtWidgets::HellingerDialog::reorder_picks()
{
	d_hellinger_model->reorder_picks();
	tree_widget_picks->clear();
	load_data_from_model();
	button_renumber->setEnabled(false);
	restore_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::renumber_segments()
{
	// TODO: check the distinction between "renumber_segments" and "reorder_picks" - do we need them both?
	reorder_picks();
}

void GPlatesQtWidgets::HellingerDialog::update_model_with_com_data()
{
	HellingerComFileStructure com_file_struct;
	com_file_struct.d_pick_file = line_import_file->text();
	com_file_struct.d_lat = spinbox_lat->value();
	com_file_struct.d_lon = spinbox_lon->value();
	com_file_struct.d_rho = spinbox_rho->value();
	com_file_struct.d_search_radius = spinbox_radius->value();
	com_file_struct.d_perform_grid_search = checkbox_grid_search->isChecked();
	com_file_struct.d_significance_level = spinbox_sig_level->value();
	com_file_struct.d_estimate_kappa = checkbox_kappa->isChecked();
	com_file_struct.d_generate_output_files = checkbox_graphics->isChecked();
	// Remaining fields in the .com file are not currently configurable from the interface.
	com_file_struct.d_data_filename = QString("hellinger.dat");
	com_file_struct.d_up_filename = QString("hellinger.up");
	com_file_struct.d_down_filename = QString("hellinger.do");

	d_hellinger_model->set_com_file_structure(com_file_struct);
}

void GPlatesQtWidgets::HellingerDialog::set_up_connections()
{
	QObject::connect(button_calculate_fit, SIGNAL(clicked()),this, SLOT(handle_calculate_fit()));
	QObject::connect(button_import_file, SIGNAL(clicked()), this, SLOT(import_hellinger_file()));
	QObject::connect(button_details, SIGNAL(clicked()), this, SLOT(show_stat_details()));
	QObject::connect(button_new_point, SIGNAL(clicked()), this, SLOT(handle_add_new_point()));
	QObject::connect(button_export_pick_file, SIGNAL(clicked()), this, SLOT(handle_export_pick_file()));
	QObject::connect(button_export_com_file, SIGNAL(clicked()), this, SLOT(handle_export_com_file()));
	QObject::connect(button_expand_all, SIGNAL(clicked()), this, SLOT(handle_expand_all()));
	QObject::connect(button_collapse_all, SIGNAL(clicked()), this, SLOT(handle_collapse_all()));
	QObject::connect(button_edit_point, SIGNAL(clicked()), this, SLOT(handle_edit_point()));
	QObject::connect(button_remove_point, SIGNAL(clicked()), this, SLOT(handle_remove_point()));
	QObject::connect(button_remove_segment, SIGNAL(clicked()), this, SLOT(handle_remove_segment()));
	QObject::connect(button_new_segment, SIGNAL(clicked()), this, SLOT(handle_add_new_segment()));
	QObject::connect(button_edit_segment, SIGNAL(clicked()), this, SLOT(handle_edit_segment()));
	QObject::connect(button_stats, SIGNAL(clicked()), this, SLOT(handle_calculate_stats()));
	QObject::connect(button_activate_pick, SIGNAL(clicked()), this, SLOT(handle_pick_state_changed()));
	QObject::connect(button_deactivate_pick, SIGNAL(clicked()), this, SLOT(handle_pick_state_changed()));
	QObject::connect(button_renumber, SIGNAL(clicked()), this, SLOT(renumber_segments()));
	QObject::connect(button_close, SIGNAL(rejected()), this, SLOT(close_dialog()));

	QObject::connect(spinbox_chron, SIGNAL(valueChanged(double)), this, SLOT(handle_chron_time_changed(double)));
	QObject::connect(spinbox_recon_time, SIGNAL(valueChanged(double)), this, SLOT(handle_recon_time_spinbox_changed(double)));
	QObject::connect(slider_recon_time, SIGNAL(valueChanged(int)), this, SLOT(handle_recon_time_slider_changed(int)));
	QObject::connect(spinbox_result_lat, SIGNAL(valueChanged(double)), this, SLOT(handle_fit_spinboxes_changed()));
	QObject::connect(spinbox_result_lon, SIGNAL(valueChanged(double)), this, SLOT(handle_fit_spinboxes_changed()));
	QObject::connect(spinbox_result_angle, SIGNAL(valueChanged(double)), this, SLOT(handle_fit_spinboxes_changed()));
	QObject::connect(spinbox_radius, SIGNAL(valueChanged(double)), this, SLOT(handle_spinbox_radius_changed()));
	QObject::connect(checkbox_grid_search, SIGNAL(clicked()), this, SLOT(handle_checkbox_grid_search_changed()));
	QObject::connect(tree_widget_picks,SIGNAL(collapsed(QModelIndex)),this,SLOT(store_expanded_status()));
	QObject::connect(tree_widget_picks,SIGNAL(expanded(QModelIndex)),this,SLOT(store_expanded_status()));
	QObject::connect(tree_widget_picks->selectionModel(), SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
					 this, SLOT(handle_selection_changed(const QItemSelection &, const QItemSelection &)));

	QObject::connect(d_hellinger_thread, SIGNAL(finished()),this, SLOT(handle_thread_finished()));
	QObject::connect(button_cancel,SIGNAL(clicked()),this,SLOT(handle_cancel()));
}

void
GPlatesQtWidgets::HellingerDialog::handle_expand_all()
{
	tree_widget_picks->expandAll();
	store_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::handle_collapse_all()
{
	tree_widget_picks->collapseAll();
	store_expanded_status();
}

void
GPlatesQtWidgets::HellingerDialog::handle_checkbox_grid_search_changed()
{
	spinbox_iteration->setEnabled(checkbox_grid_search->isChecked());
}

void
GPlatesQtWidgets::HellingerDialog::store_expanded_status()
{
	int count = tree_widget_picks->topLevelItemCount();

	d_segment_expanded_statuses.clear();
	for (int i = 0; i < count; ++i)
	{
		d_segment_expanded_statuses.insert(std::make_pair<int,bool>(i,tree_widget_picks->topLevelItem(i)->isExpanded()));
	}
}

void
GPlatesQtWidgets::HellingerDialog::restore_expanded_status()
{
	int top_level_items = tree_widget_picks->topLevelItemCount();

	for (int i = 0; i < top_level_items ; ++i)
	{
		int segment = tree_widget_picks->topLevelItem(i)->text(0).toInt();
		expanded_status_map_type::const_iterator iter = d_segment_expanded_statuses.find(segment);
		if (iter != d_segment_expanded_statuses.end())
		{
			tree_widget_picks->topLevelItem(i)->setExpanded(iter->second);
		}

	}
}

