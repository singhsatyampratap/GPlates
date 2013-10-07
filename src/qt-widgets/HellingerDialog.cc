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
#include <iterator> // std::advance
#include <vector>

#include <boost/foreach.hpp>

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
#include "HellingerStatsDialog.h"
#include "HellingerThread.h"
#include "ReadErrorAccumulationDialog.h"

#include "QtWidgetUtils.h"


const double SLIDER_MULTIPLIER = -10000.;
const int DEFAULT_SYMBOL_SIZE = 2;
const int ENLARGED_SYMBOL_SIZE = 3;
const QString TEMP_PICK_FILENAME("temp_pick");
const QString TEMP_RESULT_FILENAME("temp_pick_temp_result");
const QString TEMP_PAR_FILENAME("temp_par");
const QString TEMP_RES_FILENAME("temp_res");
const double DEFAULT_POINT_SIZE = 2;
const double DEFAULT_LINE_THICKNESS = 2;
const double ENLARGED_POINT_SIZE = 6;

// TODO: check tooltips throughout the whole Hellinger workflow.
// TODO: check button/widget focus throughout Hellinger workflow.
// TODO: restore picks on canvas when re-opening the dialog
// TODO: consider removing the close button and keeping the dialog open as long as the
// tool is selected.
// TODO: clean up the system of filenames which are passed to python.

namespace{

	enum PickColumns
	{
		SEGMENT_NUMBER,
		SEGMENT_TYPE,
		LAT,
		LON,
		UNCERTAINTY,

		NUM_COLUMNS
	};

	/**
	 * @brief The PickDisplayState enum defines the state of the pick in the treewidget.
	 * This is purely for colouring purposes, for which the tree widget item is in one of
	 * four display states.
	 */
	enum PickDisplayState
	{
		ENABLED,
		DISABLED,
		HOVERED,
		SELECTED,

		NUM_STATES
	};


	bool
	edit_operation_active(const GPlatesQtWidgets::CanvasOperationType &type)
	{
		bool result = type != GPlatesQtWidgets::SELECT_OPERATION;
		qDebug() << "edit operation active: " << result;
		return type != GPlatesQtWidgets::SELECT_OPERATION;
	}

	/**
	 * For debugging.
	 */
	void
	display_map(
			const GPlatesQtWidgets::HellingerDialog::expanded_status_map_type &map)
	{
		BOOST_FOREACH(GPlatesQtWidgets::HellingerDialog::expanded_status_map_type::value_type pair,map)
		{
			qDebug() << "key: " << pair.first << ", value: " << pair.second;
		}
	}



	void
	add_pick_geometry_to_layer(
			const GPlatesQtWidgets::HellingerPick &pick,
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type &layer,
			const GPlatesGui::Colour &colour,
			bool use_enlarged_symbol_size = false)
	{
		const GPlatesMaths::LatLonPoint llp(pick.d_lat,pick.d_lon);
		const GPlatesMaths::PointOnSphere point = make_point_on_sphere(llp);

		static const GPlatesGui::Symbol default_moving_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CROSS, DEFAULT_SYMBOL_SIZE, true);
		static const GPlatesGui::Symbol default_fixed_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::SQUARE, DEFAULT_SYMBOL_SIZE, false);
		static const GPlatesGui::Symbol enlarged_moving_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CROSS, ENLARGED_SYMBOL_SIZE, true);
		static const GPlatesGui::Symbol enlarged_fixed_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::SQUARE, ENLARGED_SYMBOL_SIZE, false);

		GPlatesGui::Symbol moving_symbol = use_enlarged_symbol_size ? enlarged_moving_symbol : default_moving_symbol;
		GPlatesGui::Symbol fixed_symbol = use_enlarged_symbol_size ? enlarged_fixed_symbol : default_fixed_symbol;

		GPlatesViewOperations::RenderedGeometry pick_geometry =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
					point.get_non_null_pointer(),
					colour,
					DEFAULT_POINT_SIZE, /* point size */
					DEFAULT_LINE_THICKNESS, /* line thickness */
					false, /* fill polygon */
					false, /* fill polyline */
					GPlatesGui::Colour::get_white(), // dummy colour argument
					pick.d_segment_type == GPlatesQtWidgets::MOVING_PICK_TYPE ?
						moving_symbol :
						fixed_symbol);

		layer->add_rendered_geometry(pick_geometry);
	}

	void
	add_segment_geometries_to_layer(
			const GPlatesQtWidgets::hellinger_model_const_range_type &segment,
			GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type &layer,
			const GPlatesGui::Colour &colour)
	{
		GPlatesQtWidgets::hellinger_model_type::const_iterator it = segment.first;
		for (; it != segment.second; ++it)
		{
			add_pick_geometry_to_layer(
						it->second,
						layer,
						colour);
		}
	}

	bool
	tree_item_is_pick_item(
			const QTreeWidgetItem *item)
	{
		return !item->text(SEGMENT_TYPE).isEmpty();
	}

	bool
	tree_item_is_segment_item(
			const QTreeWidgetItem *item)
	{
		return item->text(SEGMENT_TYPE).isEmpty();
	}

	/**
	 * @brief renumber_expanded_status_map
	 * On return the keys of @a map will be contiguous from 1.
	 * @param map
	 */
	void
	renumber_expanded_status_map(
			GPlatesQtWidgets::HellingerDialog::expanded_status_map_type &map)
	{
		GPlatesQtWidgets::HellingerDialog::expanded_status_map_type new_map;
		int new_index = 1;
		BOOST_FOREACH(GPlatesQtWidgets::HellingerDialog::expanded_status_map_type::value_type pair,map)
		{
			new_map.insert(std::pair<int,bool>(new_index++,pair.second));
		}
		map = new_map;
	}

	void
	set_text_colour_according_to_enabled_state(
			QTreeWidgetItem *item,
			bool enabled)
	{

		const Qt::GlobalColor text_colour = enabled? Qt::black : Qt::gray;
		static const Qt::GlobalColor background_colour = Qt::white;

		item->setBackgroundColor(SEGMENT_NUMBER,background_colour);
		item->setBackgroundColor(SEGMENT_TYPE,background_colour);
		item->setBackgroundColor(LAT,background_colour);
		item->setBackgroundColor(LON,background_colour);
		item->setBackgroundColor(UNCERTAINTY,background_colour);

		item->setTextColor(SEGMENT_NUMBER,text_colour);
		item->setTextColor(SEGMENT_TYPE,text_colour);
		item->setTextColor(LAT,text_colour);
		item->setTextColor(LON,text_colour);
		item->setTextColor(UNCERTAINTY,text_colour);
	}

	void
	set_hovered_item(
			QTreeWidgetItem *item)
	{

		static const Qt::GlobalColor text_colour = Qt::black;
		static const Qt::GlobalColor background_colour = Qt::yellow;

		item->setBackgroundColor(SEGMENT_NUMBER,background_colour);
		item->setBackgroundColor(SEGMENT_TYPE,background_colour);
		item->setBackgroundColor(LAT,background_colour);
		item->setBackgroundColor(LON,background_colour);
		item->setBackgroundColor(UNCERTAINTY,background_colour);

		item->setTextColor(SEGMENT_NUMBER,text_colour);
		item->setTextColor(SEGMENT_TYPE,text_colour);
		item->setTextColor(LAT,text_colour);
		item->setTextColor(LON,text_colour);
		item->setTextColor(UNCERTAINTY,text_colour);
	}

	void
	reset_hovered_item(
			QTreeWidgetItem *item,
			bool original_state)
	{
		set_text_colour_according_to_enabled_state(item,original_state);
	}

	boost::optional<int>
	current_segment_number(
			QTreeWidget *tree_widget)
	{

		if (tree_widget->currentItem())
		{
			int segment = tree_widget->currentItem()->text(0).toInt();
			return boost::optional<int>(segment);
		}
		return boost::none;
	}

	/**
	 * @brief translate_segment_type
	 *	Convert MOVING/DISABLED_MOVING types to a QString form of MOVING; similarly for FIXED/DISABLED_FIXED.
	 * @param type
	 * @return
	 */
	QString
	translate_segment_type(
			GPlatesQtWidgets::HellingerPickType type)
	{
		switch(type)
		{
		case GPlatesQtWidgets::MOVING_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_MOVING_PICK_TYPE:
			return QString::number(GPlatesQtWidgets::MOVING_PICK_TYPE);
			break;
		case GPlatesQtWidgets::FIXED_PICK_TYPE:
		case GPlatesQtWidgets::DISABLED_FIXED_PICK_TYPE:
			return QString::number(GPlatesQtWidgets::FIXED_PICK_TYPE);
			break;
		default:
			return QString();
		}
	}

	void
	add_pick_to_segment(
			QTreeWidgetItem *parent_item,
			const int &segment_number,
			const GPlatesQtWidgets::HellingerPick &pick,
			GPlatesQtWidgets::HellingerDialog::geometry_to_tree_item_map_type &geometry_to_tree_item_map,
			bool set_as_selected)
	{
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(SEGMENT_NUMBER, QString::number(segment_number));
		item->setText(SEGMENT_TYPE, translate_segment_type(pick.d_segment_type));
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
		if (pick.d_is_enabled)
		{
			geometry_to_tree_item_map.push_back(item);
		}
		item->setSelected(set_as_selected);
	}

	void
	add_pick_to_tree(
			const int &segment_number,
			const GPlatesQtWidgets::HellingerPick &pick,
			QTreeWidget *tree,
			GPlatesQtWidgets::HellingerDialog::geometry_to_tree_item_map_type &geometry_to_tree_item_map,
			bool set_as_selected_pick)
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
		add_pick_to_segment(item,
							segment_number,
							pick,
							geometry_to_tree_item_map,
							set_as_selected_pick);

	}
}

GPlatesQtWidgets::HellingerDialog::HellingerDialog(
		GPlatesPresentation::ViewState &view_state,
		GPlatesQtWidgets::ReadErrorAccumulationDialog &read_error_accumulation_dialog,
		QWidget *parent_):
	GPlatesDialog(
		parent_,
		//Qt::Window),
		Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint),
	d_view_state(view_state),
	d_rendered_geom_collection_ptr(&view_state.get_rendered_geometry_collection()),
	d_read_error_accumulation_dialog(read_error_accumulation_dialog),
	d_hellinger_model(0),
	d_hellinger_stats_dialog(0),
	d_hellinger_edit_point_dialog(0),
	d_hellinger_new_point_dialog(0),
	d_hellinger_edit_segment_dialog(0),
	d_hellinger_thread(0),
	d_moving_plate_id(0),
	d_fixed_plate_id(0),
	d_recon_time(0.),
	d_chron_time(0.),
	d_moving_symbol(GPlatesGui::Symbol::CROSS, DEFAULT_SYMBOL_SIZE, true),
	d_fixed_symbol(GPlatesGui::Symbol::SQUARE, DEFAULT_SYMBOL_SIZE, false),
	d_thread_type(POLE_THREAD_TYPE),
	d_hovered_item_original_state(true),
	d_edit_point_is_enlarged(false),
	d_canvas_operation_type(SELECT_OPERATION)
{
	setupUi(this);

	// Path copied from PythonUtils / PythonManager.

	// Look in system-specific locations for supplied sample scripts, site-specific scripts, etc.
	// The default location will be platform-dependent and is currently set up in UserPreferences.cc.
	//d_python_path = d_view_state.get_application_state().get_user_preferences().get_value("paths/python_system_script_dir").toString();
	// d_python_path = "scripts";

	// Temporary path during development
	d_python_path = "/home/robin/Desktop/Hellinger/scripts";

	qDebug() << "python path: " << d_python_path;

	d_hellinger_model = new HellingerModel(d_python_path);
	d_hellinger_thread = new HellingerThread(this, d_hellinger_model);
	d_hellinger_edit_point_dialog = new HellingerEditPointDialog(this,d_hellinger_model,false,this);
	d_hellinger_new_point_dialog = new HellingerEditPointDialog(this,d_hellinger_model,true /* create new point */,this);
	d_hellinger_edit_segment_dialog = new HellingerEditSegmentDialog(this,d_hellinger_model);

	set_up_connections();

	set_up_child_layers();

	activate_layers(true);

	d_python_path.append(QDir::separator());
	d_python_file = d_python_path + "py_hellinger.py";
	d_temporary_path = d_python_path;

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
	labels << QObject::tr("Segment")
		   << QObject::tr("Moving(1)/Fixed(2)")
		   << QObject::tr("Latitude")
		   << QObject::tr("Longitude")
		   << QObject::tr("Uncertainty (km)");
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

	clear_selection_layer();

	if (!tree_widget_picks->currentItem())
	{
		return;
	}

	if (new_selection.empty())
	{
		// Nothing selected:
		set_buttons_for_no_selection();
		d_selected_segment.reset();
		d_selected_pick.reset();
		if (d_hellinger_edit_point_dialog->isVisible())
		{
			d_hellinger_edit_point_dialog->set_active(false);
		}
	}
	else if (tree_item_is_segment_item(tree_widget_picks->currentItem())) // Segment selected
	{
		set_buttons_for_segment_selected();
		QString segment_string = tree_widget_picks->currentItem()->text(0);
		int segment = segment_string.toInt();
		d_selected_segment.reset(segment);
		d_selected_pick.reset();

		if (d_hellinger_edit_point_dialog->isVisible())
		{
			d_hellinger_edit_point_dialog->set_active(false);
		}
	}
	else // pick selected
	{
		const QModelIndex index = tree_widget_picks->selectionModel()->currentIndex();
		QString segment_string = tree_widget_picks->currentItem()->text(0);
		int row = index.row();
		int segment = segment_string.toInt();
		bool state = d_hellinger_model->get_pick_state(segment, row);

		set_buttons_for_pick_selected(state);
		d_selected_pick.reset(d_hellinger_model->get_pick(segment,row));
		d_selected_segment.reset();
		if (d_hellinger_edit_point_dialog->isVisible())
		{
			d_hellinger_edit_point_dialog->set_active(true);
			d_hellinger_edit_point_dialog->update_pick_from_model(segment,row);
		}
	}
	update_canvas();
}

void GPlatesQtWidgets::HellingerDialog::handle_cancel()
{
	// TODO: This is where we would (if we can) interrupt the thread running the python code.
}

void GPlatesQtWidgets::HellingerDialog::handle_finished_editing()
{
	qDebug() << "finished_editing";
	d_canvas_operation_type = SELECT_OPERATION;
	update_buttons();
	d_editing_layer_ptr->clear_rendered_geometries();
	d_editing_layer_ptr->set_active(false);
	d_feature_highlight_layer_ptr->set_active(false);
}

void GPlatesQtWidgets::HellingerDialog::handle_update_point_editing()
{
	d_editing_layer_ptr->clear_rendered_geometries();
	if (is_in_edit_point_state())
	{
		add_pick_geometry_to_layer(
				d_hellinger_edit_point_dialog->current_pick(),
				d_editing_layer_ptr,
				GPlatesGui::Colour::get_yellow(),
				d_edit_point_is_enlarged);
	}
	else if (is_in_new_point_state())
	{
		qDebug() << "Adding new point to layer";
		add_pick_geometry_to_layer(
				d_hellinger_new_point_dialog->current_pick(),
				d_editing_layer_ptr,
				GPlatesGui::Colour::get_yellow(),
				d_edit_point_is_enlarged);
	}
}

void GPlatesQtWidgets::HellingerDialog::handle_update_segment_editing()
{
	if (d_hellinger_edit_segment_dialog->current_pick())
	{
		d_editing_layer_ptr->clear_rendered_geometries();
		add_pick_geometry_to_layer(
					*(d_hellinger_edit_segment_dialog->current_pick()),
					d_editing_layer_ptr,
					GPlatesGui::Colour::get_yellow(),
					d_edit_point_is_enlarged);
	}
}

void
GPlatesQtWidgets::HellingerDialog::highlight_selected_pick(
		const HellingerPick &pick)
{
	GPlatesGui::Colour colour = pick.d_is_enabled ? GPlatesGui::Colour::get_white() : GPlatesGui::Colour::get_grey();
	add_pick_geometry_to_layer(pick,d_selection_layer_ptr,colour);
}

void GPlatesQtWidgets::HellingerDialog::highlight_selected_segment(
		const int &segment_number)
{
	hellinger_segment_type segment = d_hellinger_model->get_segment(segment_number);

	BOOST_FOREACH(HellingerPick pick, segment)
	{
		highlight_selected_pick(pick);
	}
}

void
GPlatesQtWidgets::HellingerDialog::handle_pick_state_changed()
{

	const QModelIndex index = tree_widget_picks->selectionModel()->currentIndex();
	int segment = tree_widget_picks->currentItem()->text(0).toInt();
	int row = index.row();

	qDebug() << "previous state: " << 	d_hellinger_model->get_pick_state(segment, row);
	bool new_enabled_state = !d_hellinger_model->get_pick_state(segment, row);

	qDebug() << "new enabled state: " << new_enabled_state;

	d_hellinger_model->set_pick_state(segment,row,new_enabled_state);
	qDebug() << "model state now set.";
	qDebug() << "Model state is: " << d_hellinger_model->get_pick_state(segment, row);

	set_buttons_for_pick_selected(new_enabled_state);


	set_text_colour_according_to_enabled_state(tree_widget_picks->currentItem(),new_enabled_state);
}

void
GPlatesQtWidgets::HellingerDialog::handle_edit_pick()
{
	d_canvas_operation_type = EDIT_POINT_OPERATION;
	update_buttons();

	d_editing_layer_ptr->set_active(true);
	const QModelIndex index = tree_widget_picks->selectionModel()->currentIndex();
	QString segment_string = tree_widget_picks->currentItem()->text(0);
	int row = index.row();
	int segment = segment_string.toInt();

	d_hellinger_edit_point_dialog->update_pick_from_model(segment, row);
	d_hellinger_edit_point_dialog->show();
	d_hellinger_edit_point_dialog->raise();

	add_pick_geometry_to_layer((d_hellinger_model->get_pick(segment,row)->second),d_editing_layer_ptr,GPlatesGui::Colour::get_yellow());
}

void
GPlatesQtWidgets::HellingerDialog::handle_edit_segment()
{
	d_canvas_operation_type = EDIT_SEGMENT_OPERATION;
	update_buttons();

	d_editing_layer_ptr->set_active(true);

	QString segment_string = tree_widget_picks->currentItem()->text(SEGMENT_NUMBER);
	int segment = segment_string.toInt();

	d_hellinger_edit_segment_dialog->initialise_with_segment(
				d_hellinger_model->get_segment_as_range(segment),segment);

	d_hellinger_edit_segment_dialog->show();
	d_hellinger_edit_segment_dialog->raise();



	add_segment_geometries_to_layer(
				d_hellinger_model->get_segment_as_range(segment),
				d_editing_layer_ptr,
				GPlatesGui::Colour::get_yellow());
#if 0
	if (!d_hellinger_model->segments_are_ordered())
	{
		button_renumber->setEnabled(true);
	}
	update_canvas();
#endif
}

void
GPlatesQtWidgets::HellingerDialog::handle_remove_pick()
{	
	QMessageBox message_box;
	message_box.setIcon(QMessageBox::Warning);
	message_box.setWindowTitle(tr("Remove pick"));
	message_box.setText(
				tr("Are you sure you want to remove the pick?"));
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

		if (d_selected_pick && *d_selected_pick == d_hellinger_model->get_pick(segment_int,row))
		{
			d_selected_pick.reset();
		}

		d_hellinger_model->remove_pick(segment_int, row);
		update_tree_from_model();
		update_canvas();
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
		store_expanded_status();
		QString segment = tree_widget_picks->currentItem()->text(0);
		unsigned int segment_int = segment.toInt();

		if (d_selected_segment && *d_selected_segment == segment_int)
		{
			d_selected_segment.reset();
		}

		d_hellinger_model->remove_segment(segment_int);
		button_renumber->setEnabled(true);
		update_tree_from_model();
		restore_expanded_status();
		if (!d_hellinger_model->segments_are_ordered())
		{
			button_renumber->setEnabled(true);
		}
	}
}

void
GPlatesQtWidgets::HellingerDialog::initialise()
{
	update_from_model();
}

void GPlatesQtWidgets::HellingerDialog::restore()
{
	activate_layers();
	update_from_model();}

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
	clear_rendered_geometries();

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
#if 0
		checkbox_kappa->setChecked(com_file_data.get().d_estimate_kappa);
		checkbox_graphics->setChecked(com_file_data.get().d_generate_output_files);
#endif
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
				TEMP_PICK_FILENAME,
				TEMP_RESULT_FILENAME,
				TEMP_PAR_FILENAME,
				TEMP_RES_FILENAME);
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
GPlatesQtWidgets::HellingerDialog::handle_add_new_pick()
{    
	d_canvas_operation_type = NEW_POINT_OPERATION;
	update_buttons();

	d_editing_layer_ptr->set_active(true);
	d_feature_highlight_layer_ptr->set_active(true);

	if (boost::optional<int> segment = current_segment_number(tree_widget_picks))
	{
		d_hellinger_new_point_dialog->update_segment_number(segment.get());
	}



	d_hellinger_new_point_dialog->show();
	d_hellinger_new_point_dialog->raise();

	d_hellinger_new_point_dialog->update_pick_coords(
				GPlatesMaths::LatLonPoint(0,0));
}

void
GPlatesQtWidgets::HellingerDialog::handle_add_new_segment()
{
	d_canvas_operation_type = NEW_SEGMENT_OPERATION;
	update_buttons();

	QScopedPointer<GPlatesQtWidgets::HellingerEditSegmentDialog> dialog(
				new GPlatesQtWidgets::HellingerEditSegmentDialog(this,
																d_hellinger_model,
																true /*create new segment */));

	dialog->exec();
}

void
GPlatesQtWidgets::HellingerDialog::handle_calculate_fit()
{        
	// TODO: Refactor this method.
	if (!d_hellinger_model->segments_are_ordered())
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
			renumber_segments();
		}

	}

	if (!(spinbox_rho->value() > 0))
	{
		QMessageBox::critical(this,tr("Initial guess values"),
							  tr("The value of rho in the initial guess is zero. Please enter a non-zero value"),
							  QMessageBox::Ok,QMessageBox::Ok);
		return;
	}

	d_hellinger_model->set_initial_guess(
				spinbox_lat->value(),
				spinbox_lon->value(),
				spinbox_rho->value(),
				spinbox_radius->value());

	QFile python_code(d_python_file);
	if (python_code.exists())
	{
		QString path = d_python_path + TEMP_PICK_FILENAME;
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
#if 0
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
#endif
		bool_data.push_back(1); // kappa
		bool_data.push_back(1); // graphics

		d_hellinger_thread->initialise_pole_calculation(
					import_file_line,
					input_data,
					bool_data,
					iteration,
					d_python_file,
					d_temporary_path,
					TEMP_PICK_FILENAME,
					TEMP_RESULT_FILENAME,
					TEMP_PAR_FILENAME,
					TEMP_RES_FILENAME);
		d_thread_type = POLE_THREAD_TYPE;

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
	qDebug() << "Thread finished" << "type" << d_thread_type;
	progress_bar->setEnabled(false);
	progress_bar->setMaximum(1.);
	if (d_thread_type == POLE_THREAD_TYPE)
	{
		QString path = d_python_path + TEMP_RESULT_FILENAME;
		qDebug() << "Result file path: " << path;
		QFile data_file(path);
		if (data_file.open(QFile::ReadOnly))
		{
			QTextStream in(&data_file);
			QString line = in.readLine();
			QStringList fields = line.split(" ",QString::SkipEmptyParts);
			d_hellinger_model->set_fit(fields);
			data_file.close();
			update_result();
			qDebug() << "updated result";
			button_stats->setEnabled(true);
			button_details->setEnabled(true);
		}
	}
	else if(d_thread_type == STATS_THREAD_TYPE)
	{
		d_hellinger_model->read_error_ellipse_points();
		draw_error_ellipse();
		button_details->setEnabled(true);
	}
}

void
GPlatesQtWidgets::HellingerDialog::update_buttons()
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
	button_clear->setEnabled(false);

	// Update based on if we have some picks loaded or not.
	if (picks_loaded())
	{
		button_expand_all->setEnabled(true);
		button_collapse_all->setEnabled(true);
		button_export_pick_file->setEnabled(true);
		button_export_com_file->setEnabled(true);
		button_calculate_fit->setEnabled(spinbox_radius->value() > 0.0);
		button_clear->setEnabled(true);
	}


	// Disable some edit / fit buttons if we are in the middle of an editing/creation operation
	// It's simpler to disable the whole main dialog, but this also disables the child dialogs (e.g. the
	// edit dialogs) so we'd be stuck in that state.
	bool new_button_state = !edit_operation_active(d_canvas_operation_type);
	qDebug() << "new button state: " << new_button_state;
	button_calculate_fit->setEnabled(new_button_state);
	button_edit_point->setEnabled(new_button_state && d_selected_pick);
	button_new_pick->setEnabled(new_button_state);
	button_edit_segment->setEnabled(new_button_state && d_selected_segment);
	button_new_segment->setEnabled(new_button_state);



}

void
GPlatesQtWidgets::HellingerDialog::update_from_model()
{
	d_pick_layer_ptr->set_active(true);
	update_tree_from_model();
	update_initial_guess();
}

void
GPlatesQtWidgets::HellingerDialog::draw_pole(
		const double &lat,
		const double &lon)
{
	GPlatesGui::Symbol results_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CIRCLE, DEFAULT_SYMBOL_SIZE, true);
	GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
				GPlatesMaths::LatLonPoint(lat,lon));
	GPlatesViewOperations::RenderedGeometry pick_results =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				point.get_non_null_pointer(),
				GPlatesGui::Colour::get_red(),
				2, /* point thickness */
				2, /* line thickness */
				false, /* fill polygon */
				false, /* fill polyline */
				GPlatesGui::Colour::get_white(), // dummy colour argument
				results_symbol);

	d_result_layer_ptr->add_rendered_geometry(pick_results);
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
			GPlatesGui::Symbol results_symbol = GPlatesGui::Symbol(GPlatesGui::Symbol::CROSS, DEFAULT_SYMBOL_SIZE, true);
			GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
						GPlatesMaths::LatLonPoint(lat,lon));
			GPlatesViewOperations::RenderedGeometry pick_results =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
						point.get_non_null_pointer(),
						GPlatesGui::Colour::get_red(),
						2, /* point thickness */
						2, /* line thickness */
						false, /* fill polygon */
						false, /* fill polyline */
						GPlatesGui::Colour::get_white(), // dummy colour argument
						results_symbol);
			d_result_layer_ptr->add_rendered_geometry(pick_results);
		}
	}

}


void
GPlatesQtWidgets::HellingerDialog::update_tree_from_model()
{    
	QObject::disconnect(tree_widget_picks->selectionModel(), SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
					 this, SLOT(handle_selection_changed(const QItemSelection &, const QItemSelection &)));

	tree_widget_picks->clear();

	d_geometry_to_tree_item_map.clear();
	hellinger_model_type::const_iterator
			iter = d_hellinger_model->begin(),
			end = d_hellinger_model->end();

	for (; iter != end ; ++iter)
	{
		bool set_as_selected_pick =(d_selected_pick && (*d_selected_pick == iter));
		add_pick_to_tree(
					iter->first,
					iter->second,
					tree_widget_picks,
					d_geometry_to_tree_item_map,
					set_as_selected_pick);

	}

	update_canvas();
	update_buttons();

	QObject::connect(tree_widget_picks->selectionModel(), SIGNAL(selectionChanged (const QItemSelection &, const QItemSelection &)),
					 this, SLOT(handle_selection_changed(const QItemSelection &, const QItemSelection &)));
}

void
GPlatesQtWidgets::HellingerDialog::create_feature_collection()
{

}

void
GPlatesQtWidgets::HellingerDialog::handle_close()
{
	clear_rendered_geometries();
	activate_layers(false);
	d_hellinger_edit_point_dialog->close();
	d_hellinger_new_point_dialog->close();
}

void
GPlatesQtWidgets::HellingerDialog::update_canvas()
{
	qDebug() << "update_canvas";
	clear_rendered_geometries();
	draw_picks();
	update_result();
	draw_error_ellipse();
	update_hovered_item();
	update_selected_geometries();

}

void
GPlatesQtWidgets::HellingerDialog::update_edit_layer(
		const GPlatesMaths::PointOnSphere &pos_)
{
	// TODO: see if we can use a single instance of HellingerEditPointDialog and
	// so prevent having to check here (and elsewhere) for the operation type.
	GPlatesMaths::LatLonPoint llp = GPlatesMaths::make_lat_lon_point(pos_);
	if (is_in_edit_point_state())
	{
		d_hellinger_edit_point_dialog->update_pick_coords(llp);
	}
	if (is_in_new_point_state())
	{
		qDebug() << "HD::UEL new_point_state";
		d_hellinger_new_point_dialog->update_pick_coords(llp);
	}

}

void GPlatesQtWidgets::HellingerDialog::set_enlarged_edit_geometry(bool enlarged)
{
	d_edit_point_is_enlarged = enlarged;
	handle_update_point_editing();
}

bool GPlatesQtWidgets::HellingerDialog::is_in_edit_point_state()
{
	return ((d_canvas_operation_type == EDIT_POINT_OPERATION) ||
			(d_canvas_operation_type == EDIT_SEGMENT_OPERATION));
}

bool GPlatesQtWidgets::HellingerDialog::is_in_new_point_state()
{
	return ((d_canvas_operation_type == NEW_POINT_OPERATION) ||
			(d_canvas_operation_type == NEW_SEGMENT_OPERATION));
}

void GPlatesQtWidgets::HellingerDialog::set_feature_highlight(
		const GPlatesMaths::PointOnSphere &point)
{
	GPlatesViewOperations::RenderedGeometry highlight_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
				point.get_non_null_pointer(),
				GPlatesGui::Colour::get_yellow(),
				ENLARGED_POINT_SIZE,
				DEFAULT_LINE_THICKNESS,
				false, /* fill polygon */
				false /* fill polyline */ );

	d_feature_highlight_layer_ptr->add_rendered_geometry(highlight_geometry);
}

void GPlatesQtWidgets::HellingerDialog::update_selected_geometries()
{
	if (d_selected_pick)
	{
		highlight_selected_pick((*d_selected_pick)->second);
	}
	else if (d_selected_segment)
	{
		highlight_selected_segment(*d_selected_segment);
	}
}


const GPlatesGui::Colour &
GPlatesQtWidgets::HellingerDialog::get_segment_colour(
		int num_colour)
{
	num_colour = num_colour%7;
	switch (num_colour)
	{
	case 0:
		return GPlatesGui::Colour::get_green();
		break;
	case 1:
		return GPlatesGui::Colour::get_blue();
		break;
	case 2:
		return GPlatesGui::Colour::get_maroon();
		break;
	case 3:
		return GPlatesGui::Colour::get_purple();
		break;
	case 4:
		return GPlatesGui::Colour::get_fuchsia();
		break;
	case 5:
		return GPlatesGui::Colour::get_olive();
		break;
	case 6:
		return GPlatesGui::Colour::get_navy();
		break;
	default:
		return GPlatesGui::Colour::get_navy();
	}
}

void GPlatesQtWidgets::HellingerDialog::handle_clear()
{
	QMessageBox message_box;
	message_box.setIcon(QMessageBox::Warning);
	message_box.setWindowTitle(tr("Clear all picks"));
	message_box.setText(
				tr("Are you sure you want to remove all the picks?"));
	message_box.setStandardButtons(QMessageBox::Cancel | QMessageBox::Ok);
	message_box.setDefaultButton(QMessageBox::Ok);
	int ret = message_box.exec();

	if (ret == QMessageBox::Cancel)
	{
		return;
	}
	else
	{
		d_hellinger_model->clear_all_picks();
		update_tree_from_model();
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

			if (it->second.d_segment_type == FIXED_PICK_TYPE)
			{
				GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
							GPlatesMaths::LatLonPoint(it->second.d_lat,it->second.d_lon));

				GPlatesViewOperations::RenderedGeometry pick_geometry =
						GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
							point.get_non_null_pointer(),
							get_segment_colour(num_colour),
							2, /* point thickness */
							2, /* line thickness */
							false, /* fill polygon */
							false, /* fill polyline */
							GPlatesGui::Colour::get_white(), // dummy colour argument
							it->second.d_segment_type == MOVING_PICK_TYPE ? d_moving_symbol : d_fixed_symbol);

				d_result_layer_ptr->add_rendered_geometry(pick_geometry);
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

			if (it->second.d_segment_type == MOVING_PICK_TYPE)
			{
				GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
							GPlatesMaths::LatLonPoint(it->second.d_lat,it->second.d_lon));

				GPlatesViewOperations::RenderedGeometry pick_geometry =
						GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
							point.get_non_null_pointer(),
							get_segment_colour(num_colour),
							2, /* point thickness */
							2, /* line thickness */
							false, /* fill polygon */
							false, /* fill polyline */
							GPlatesGui::Colour::get_white(), // dummy colour argument
							it->second.d_segment_type == MOVING_PICK_TYPE ? d_moving_symbol : d_fixed_symbol);

				d_pick_layer_ptr->add_rendered_geometry(pick_geometry);
			}
		}
	}

}

void GPlatesQtWidgets::HellingerDialog::draw_picks()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_pick_layer_ptr->clear_rendered_geometries();

	hellinger_model_type::const_iterator it = d_hellinger_model->begin();
	int num_segment = 0;
	int num_colour = 0;
	d_geometry_to_model_map.clear();
	for (; it != d_hellinger_model->end(); ++it)
	{
		if (it->second.d_is_enabled)
		{
			if (num_segment != it->first)
			{
				++num_colour;
				++num_segment;
			}

			GPlatesMaths::PointOnSphere point = GPlatesMaths::make_point_on_sphere(
						GPlatesMaths::LatLonPoint(it->second.d_lat,it->second.d_lon));

			GPlatesViewOperations::RenderedGeometry pick_geometry =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
						point.get_non_null_pointer(),
						get_segment_colour(num_colour),
						2, /* point thickness */
						2, /* line thickness */
						false, /* fill polygon */
						false, /* fill polyline */
						GPlatesGui::Colour::get_white(), // dummy colour argument
						it->second.d_segment_type == MOVING_PICK_TYPE ? d_moving_symbol : d_fixed_symbol);

			d_pick_layer_ptr->add_rendered_geometry(pick_geometry);
			d_geometry_to_model_map.push_back(it);
		}
	}
}

void
GPlatesQtWidgets::HellingerDialog::reconstruct_picks()
{

	clear_rendered_geometries();
	draw_fixed_picks();
	update_result();
	boost::optional<GPlatesQtWidgets::HellingerFitStructure> data_fit_struct = d_hellinger_model->get_fit();

	if (!data_fit_struct)
	{
		return;
	}

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
								get_segment_colour(num_colour),
								2, /* point thickness */
								2, /* line thickness */
								false, /* fill polygon */
								false, /* fill polyline */
								GPlatesGui::Colour::get_white(), // dummy colour argument
								it->second.d_segment_type == MOVING_PICK_TYPE ? d_moving_symbol : d_fixed_symbol);

					d_result_layer_ptr->add_rendered_geometry(pick_geometry);
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
GPlatesQtWidgets::HellingerDialog::update_hovered_item(
		boost::optional<QTreeWidgetItem *> item,
		bool current_state)
{

	if (d_hovered_item)
	{
		reset_hovered_item(*d_hovered_item,d_hovered_item_original_state);
	}
	d_hovered_item = item;
	if (d_hovered_item)
	{
		set_hovered_item(*d_hovered_item);
		d_hovered_item_original_state = current_state;
	}
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
	clear_rendered_geometries();

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
GPlatesQtWidgets::HellingerDialog::renumber_segments()
{
	store_expanded_status();
	d_hellinger_model->renumber_segments();
	renumber_expanded_status_map(d_segment_expanded_status);
	tree_widget_picks->clear();
	update_tree_from_model();
	button_renumber->setEnabled(false);
	restore_expanded_status();
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
#if 0
	com_file_struct.d_estimate_kappa = checkbox_kappa->isChecked();
	com_file_struct.d_generate_output_files = checkbox_graphics->isChecked();
#endif
	com_file_struct.d_estimate_kappa = true;
	com_file_struct.d_generate_output_files = true;

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
	QObject::connect(button_new_pick, SIGNAL(clicked()), this, SLOT(handle_add_new_pick()));
	QObject::connect(button_export_pick_file, SIGNAL(clicked()), this, SLOT(handle_export_pick_file()));
	QObject::connect(button_export_com_file, SIGNAL(clicked()), this, SLOT(handle_export_com_file()));
	QObject::connect(button_expand_all, SIGNAL(clicked()), this, SLOT(handle_expand_all()));
	QObject::connect(button_collapse_all, SIGNAL(clicked()), this, SLOT(handle_collapse_all()));
	QObject::connect(button_edit_point, SIGNAL(clicked()), this, SLOT(handle_edit_pick()));
	QObject::connect(button_remove_point, SIGNAL(clicked()), this, SLOT(handle_remove_pick()));
	QObject::connect(button_remove_segment, SIGNAL(clicked()), this, SLOT(handle_remove_segment()));
	QObject::connect(button_new_segment, SIGNAL(clicked()), this, SLOT(handle_add_new_segment()));
	QObject::connect(button_edit_segment, SIGNAL(clicked()), this, SLOT(handle_edit_segment()));
	QObject::connect(button_stats, SIGNAL(clicked()), this, SLOT(handle_calculate_stats()));
	QObject::connect(button_activate_pick, SIGNAL(clicked()), this, SLOT(handle_pick_state_changed()));
	QObject::connect(button_deactivate_pick, SIGNAL(clicked()), this, SLOT(handle_pick_state_changed()));
	QObject::connect(button_renumber, SIGNAL(clicked()), this, SLOT(renumber_segments()));
	QObject::connect(button_close, SIGNAL(rejected()), this, SLOT(handle_close()));

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
	QObject::connect(button_clear,SIGNAL(clicked()),this,SLOT(handle_clear()));

	QObject::connect(d_hellinger_edit_point_dialog,SIGNAL(finished_editing()),this,SLOT(handle_finished_editing()));
	QObject::connect(d_hellinger_new_point_dialog,SIGNAL(finished_editing()),this,SLOT(handle_finished_editing()));

	QObject::connect(d_hellinger_edit_point_dialog,SIGNAL(update_editing()),this,SLOT(handle_update_point_editing()));
	QObject::connect(d_hellinger_new_point_dialog,SIGNAL(update_editing()),this,SLOT(handle_update_point_editing()));

	QObject::connect(d_hellinger_edit_segment_dialog,SIGNAL(finished_editing()),this,SLOT(handle_finished_editing()));

}

void GPlatesQtWidgets::HellingerDialog::set_up_child_layers()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without redrawing canvas after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Create a rendered layer to draw the picks.
	d_pick_layer_ptr =
		d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER);

	// Create a rendered layer to draw resultant pole and reconstructed picks
	d_result_layer_ptr =
		d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER);


	// Create a rendered layer to draw selected geometries
	d_selection_layer_ptr =
		d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER);

	// Create a rendered layer to draw highlighted geometries
	d_hover_layer_ptr =
		d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER);

	// Create a rendered layer to draw geometries undergoing editing.
	d_editing_layer_ptr =
		d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER);

	// Create a rendered layer to highlight feature geometries which can be selected.
	d_feature_highlight_layer_ptr =
			d_rendered_geom_collection_ptr->create_child_rendered_layer_and_transfer_ownership(
				GPlatesViewOperations::RenderedGeometryCollection::POLE_MANIPULATION_CANVAS_TOOL_WORKFLOW_LAYER);
}

void GPlatesQtWidgets::HellingerDialog::activate_layers(bool activate)
{
	d_pick_layer_ptr->set_active(activate);
	d_hover_layer_ptr->set_active(activate);
	d_result_layer_ptr->set_active(activate);
	d_selection_layer_ptr->set_active(activate);
}

void GPlatesQtWidgets::HellingerDialog::clear_rendered_geometries()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_pick_layer_ptr->clear_rendered_geometries();
	d_hover_layer_ptr->clear_rendered_geometries();
	d_result_layer_ptr->clear_rendered_geometries();
	d_selection_layer_ptr->clear_rendered_geometries();
	d_editing_layer_ptr->clear_rendered_geometries();
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

	d_segment_expanded_status.clear();
	for (int i = 0 ; i < count; ++i)
	{
		int segment = tree_widget_picks->topLevelItem(i)->text(0).toInt();
		d_segment_expanded_status.insert(std::make_pair<int,bool>(segment,tree_widget_picks->topLevelItem(i)->isExpanded()));
	}
}

void
GPlatesQtWidgets::HellingerDialog::restore_expanded_status()
{
	int top_level_items = tree_widget_picks->topLevelItemCount();
	QObject::disconnect(tree_widget_picks,SIGNAL(collapsed(QModelIndex)),this,SLOT(store_expanded_status()));
	QObject::disconnect(tree_widget_picks,SIGNAL(expanded(QModelIndex)),this,SLOT(store_expanded_status()));
	for (int i = 0; i < top_level_items ; ++i)
	{
		int segment = tree_widget_picks->topLevelItem(i)->text(0).toInt();
		expanded_status_map_type::const_iterator iter = d_segment_expanded_status.find(segment);
		if (iter != d_segment_expanded_status.end())
		{
			tree_widget_picks->topLevelItem(i)->setExpanded(iter->second);
		}

	}
	QObject::connect(tree_widget_picks,SIGNAL(collapsed(QModelIndex)),this,SLOT(store_expanded_status()));
	QObject::connect(tree_widget_picks,SIGNAL(expanded(QModelIndex)),this,SLOT(store_expanded_status()));
}

void GPlatesQtWidgets::HellingerDialog::expand_segment(
		const unsigned int segment_number)
{
	int top_level_items = tree_widget_picks->topLevelItemCount();
	for (int i = 0; i < top_level_items ; ++i)
	{
		const unsigned int segment = tree_widget_picks->topLevelItem(i)->text(0).toInt();

		if (segment == segment_number){
			tree_widget_picks->topLevelItem(i)->setExpanded(true);
			expanded_status_map_type::iterator iter = d_segment_expanded_status.find(segment);
			if (iter != d_segment_expanded_status.end())
			{
				iter->second = true;
			}
			return;
		}
	}
}

GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type GPlatesQtWidgets::HellingerDialog::get_pick_layer()
{
	return d_pick_layer_ptr;
}

GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type GPlatesQtWidgets::HellingerDialog::get_editing_layer()
{
	return d_editing_layer_ptr;
}

GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type GPlatesQtWidgets::HellingerDialog::get_feature_highlight_layer()
{
	return d_feature_highlight_layer_ptr;
}

void GPlatesQtWidgets::HellingerDialog::set_hovered_pick(
		const unsigned int index)
{

	if (index > d_geometry_to_model_map.size())
	{
		return;
	}

	hellinger_model_type::const_iterator it = d_geometry_to_model_map[index];
	const HellingerPick &pick = it->second;

	d_hover_layer_ptr->clear_rendered_geometries();

	add_pick_geometry_to_layer(pick,d_hover_layer_ptr,GPlatesGui::Colour::get_silver());

	if (index > d_geometry_to_tree_item_map.size())
	{
		return;
	}

	update_hovered_item(d_geometry_to_tree_item_map[index],pick.d_is_enabled);

}



void GPlatesQtWidgets::HellingerDialog::set_selected_pick(
		const unsigned int index)
{
	if (index > d_geometry_to_tree_item_map.size())
	{
		return;
	}
	update_hovered_item();

	d_selected_pick.reset(d_geometry_to_model_map[index]);
	d_selected_segment.reset();

	// This will trigger an update of the canvas.
	tree_widget_picks->setCurrentItem(d_geometry_to_tree_item_map[index]);
}

void GPlatesQtWidgets::HellingerDialog::set_selected_pick(
		const GPlatesQtWidgets::hellinger_model_type::const_iterator &it)
{
	d_selected_pick.reset(it);
	d_selected_segment.reset();
	update_canvas();
}

void GPlatesQtWidgets::HellingerDialog::clear_hovered_layer()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_hover_layer_ptr->clear_rendered_geometries();
	update_hovered_item();
}

void GPlatesQtWidgets::HellingerDialog::clear_selection_layer()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_selection_layer_ptr->clear_rendered_geometries();
}

void GPlatesQtWidgets::HellingerDialog::clear_editing_layer()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_editing_layer_ptr->clear_rendered_geometries();
}

void GPlatesQtWidgets::HellingerDialog::clear_feature_highlight_layer()
{
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;
	d_feature_highlight_layer_ptr->clear_rendered_geometries();
}

void GPlatesQtWidgets::HellingerDialog::edit_current_pick()
{
	handle_edit_pick();
}

