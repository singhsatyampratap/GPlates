/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010, 2011 The University of Sydney, Australia
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

#include <boost/shared_ptr.hpp>
#include <QDir>
#include <QFileInfo>

#include "RasterLayerOptionsWidget.h"

#include "QtWidgetUtils.h"
#include "ReadErrorAccumulationDialog.h"
#include "RemappedColourPaletteWidget.h"
#include "ViewportWindow.h"

#include "app-logic/Layer.h"
#include "app-logic/RasterLayerTask.h"

#include "file-io/ReadErrorAccumulation.h"

#include "gui/ColourPaletteUtils.h"
#include "gui/CptColourPalette.h"
#include "gui/Dialogs.h"

#include "presentation/RasterVisualLayerParams.h"
#include "presentation/VisualLayer.h"

#include "property-values/XsString.h"


namespace GPlatesQtWidgets
{
	namespace
	{
		//! The age CPT file (user can manually select when they load an age grid raster).
		const QString AGE_CPT_FILE_NAME = ":/age.cpt";
	}
}


GPlatesQtWidgets::RasterLayerOptionsWidget::RasterLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_open_file_dialog(
			this,
			tr("Open CPT File"),
			tr("Regular CPT file (*.cpt);;All files (*)"),
			view_state),
	d_use_age_palette_button(new QToolButton(this)),
	d_colour_palette_widget(
			new RemappedColourPaletteWidget(view_state, viewport_window, this, d_use_age_palette_button))
{
	setupUi(this);


	band_combobox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			band_combobox, SIGNAL(activated(const QString &)),
			this, SLOT(handle_band_combobox_activated(const QString &)));

	d_use_age_palette_button->setCursor(QCursor(Qt::ArrowCursor));
	d_use_age_palette_button->setText(QObject::tr("Age"));
	QObject::connect(
			d_use_age_palette_button, SIGNAL(clicked()),
			this, SLOT(handle_use_age_palette_button_clicked()));

	opacity_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			opacity_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_opacity_spinbox_changed(double)));

	intensity_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			intensity_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_intensity_spinbox_changed(double)));

	surface_relief_scale_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	QObject::connect(
			surface_relief_scale_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_surface_relief_scale_spinbox_changed(double)));

	//
	// Colour palette.
	//

	QtWidgetUtils::add_widget_to_placeholder(
			d_colour_palette_widget,
			palette_placeholder_widget);
	d_colour_palette_widget->setCursor(QCursor(Qt::ArrowCursor));

	QObject::connect(
			d_colour_palette_widget, SIGNAL(select_palette_filename_button_clicked()),
			this, SLOT(handle_select_palette_filename_button_clicked()));
	QObject::connect(
			d_colour_palette_widget, SIGNAL(use_default_palette_button_clicked()),
			this, SLOT(handle_use_default_palette_button_clicked()));

	QObject::connect(
			d_colour_palette_widget, SIGNAL(range_check_box_changed(int)),
			this, SLOT(handle_palette_range_check_box_changed(int)));

	QObject::connect(
			d_colour_palette_widget, SIGNAL(min_line_editing_finished(double)),
			this, SLOT(handle_palette_min_line_editing_finished(double)));
	QObject::connect(
			d_colour_palette_widget, SIGNAL(max_line_editing_finished(double)),
			this, SLOT(handle_palette_max_line_editing_finished(double)));

	QObject::connect(
			d_colour_palette_widget, SIGNAL(range_restore_min_max_button_clicked()),
			this, SLOT(handle_palette_range_restore_min_max_button_clicked()));
	QObject::connect(
			d_colour_palette_widget, SIGNAL(range_restore_mean_deviation_button_clicked()),
			this, SLOT(handle_palette_range_restore_mean_deviation_button_clicked()));
	QObject::connect(
			d_colour_palette_widget, SIGNAL(range_restore_mean_deviation_spinbox_changed(double)),
			this, SLOT(handle_palette_range_restore_mean_deviation_spinbox_changed(double)));
}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::RasterLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new RasterLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::RasterLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::RasterLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (layer_task_params)
		{
			// Populate the band combobox with the list of band names, and ensure that
			// the correct one is selected.
			int band_name_index = -1;
			const GPlatesPropertyValues::TextContent &selected_band_name = layer_task_params->get_band_name();

			band_combobox->clear();
			const GPlatesPropertyValues::GpmlRasterBandNames::band_names_list_type &band_names =
				layer_task_params->get_band_names();
			for (int i = 0; i != static_cast<int>(band_names.size()); ++i)
			{
				const GPlatesPropertyValues::TextContent &curr_band_name = band_names[i]->value();
				if (curr_band_name == selected_band_name)
				{
					band_name_index = i;
				}
				band_combobox->addItem(curr_band_name.get().qstring());
			}

			band_combobox->setCurrentIndex(band_name_index);
		}

		GPlatesPresentation::RasterVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			// Hide colour palette-related widgets if raster type is RGBA8.
			if (visual_layer_params->get_raster_type() == GPlatesPropertyValues::RasterType::RGBA8)
			{
				palette_placeholder_widget->setVisible(false);
			}
			else
			{
				palette_placeholder_widget->setVisible(true);

				// Set the colour palette.
				d_colour_palette_widget->set_parameters(
						visual_layer_params->get_colour_palette_parameters());
			}

			// Setting the values in the spin boxes will emit signals if the value changes
			// which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect their signals.

			QObject::disconnect(
					opacity_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_opacity_spinbox_changed(double)));
			opacity_spinbox->setValue(visual_layer_params->get_opacity());
			QObject::connect(
					opacity_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_opacity_spinbox_changed(double)));

			QObject::disconnect(
					intensity_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_intensity_spinbox_changed(double)));
			intensity_spinbox->setValue(visual_layer_params->get_intensity());
			QObject::connect(
					intensity_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_intensity_spinbox_changed(double)));

			QObject::disconnect(
					surface_relief_scale_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_surface_relief_scale_spinbox_changed(double)));
			surface_relief_scale_spinbox->setValue(visual_layer_params->get_surface_relief_scale());
			QObject::connect(
					surface_relief_scale_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_surface_relief_scale_spinbox_changed(double)));
		}
	}
}


const QString &
GPlatesQtWidgets::RasterLayerOptionsWidget::get_title()
{
	static const QString TITLE = tr("Raster options");
	return TITLE;
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_band_combobox_activated(
		const QString &text)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		// Set the band name in the app-logic layer params.
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::RasterLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::RasterLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (layer_task_params)
		{
			layer_task_params->set_band_name(GPlatesUtils::UnicodeString(text));
		}
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_select_palette_filename_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!params)
		{
			return;
		}

		QString palette_file_name = d_open_file_dialog.get_open_file_name();
		if (palette_file_name.isEmpty())
		{
			return;
		}

		d_view_state.get_last_open_directory() = QFileInfo(palette_file_name).path();

		std::pair<double, double> colour_palette_range;
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type colour_palette =
				load_colour_palette(palette_file_name, colour_palette_range, *params);
		if (GPlatesGui::RasterColourPaletteType::get_type(*colour_palette) ==
			GPlatesGui::RasterColourPaletteType::INVALID)
		{
			return;
		}

		// Update the colour palette in the layer params.
		GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
				params->get_colour_palette_parameters();
		colour_palette_parameters.set_colour_palette(
				palette_file_name,
				colour_palette,
				colour_palette_range);
		params->set_colour_palette_parameters(colour_palette_parameters);
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_use_default_palette_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					params->get_colour_palette_parameters();
			colour_palette_parameters.use_default_colour_palette();
			params->set_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_use_age_palette_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!params)
		{
			return;
		}

		std::pair<double, double> age_colour_palette_range;
		GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type age_colour_palette =
				load_colour_palette(AGE_CPT_FILE_NAME, age_colour_palette_range, *params);
		if (GPlatesGui::RasterColourPaletteType::get_type(*age_colour_palette) ==
			GPlatesGui::RasterColourPaletteType::INVALID)
		{
			return;
		}

		// Update the colour palette in the layer params.
		GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
				params->get_colour_palette_parameters();
		colour_palette_parameters.set_colour_palette(
				AGE_CPT_FILE_NAME,
				age_colour_palette,
				age_colour_palette_range);
		params->set_colour_palette_parameters(colour_palette_parameters);
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_palette_range_check_box_changed(
		int state)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					params->get_colour_palette_parameters();
			// Map or unmap the colour palette range.
			if (state == Qt::Checked)
			{
				colour_palette_parameters.map_palette_range(
						colour_palette_parameters.get_mapped_palette_range().first,
						colour_palette_parameters.get_mapped_palette_range().second);
			}
			else
			{
				colour_palette_parameters.unmap_palette_range();
			}
			params->set_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_palette_min_line_editing_finished(
		double min_value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> raster_scalar_min_max = get_raster_scalar_min_max(layer);
			const double min_range_delta = 0.0001 * (raster_scalar_min_max.second - raster_scalar_min_max.first);

			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					visual_layer_params->get_colour_palette_parameters();

			// Ensure min not greater than max.
			if (min_value > colour_palette_parameters.get_palette_range().second/*max*/ - min_range_delta)
			{
				min_value = colour_palette_parameters.get_palette_range().second/*max*/ - min_range_delta;
			}

			colour_palette_parameters.map_palette_range(
					min_value/*min*/,
					colour_palette_parameters.get_palette_range().second/*max*/);

			visual_layer_params->set_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_palette_max_line_editing_finished(
		double max_value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> raster_scalar_min_max = get_raster_scalar_min_max(layer);
			const double min_range_delta = 0.0001 * (raster_scalar_min_max.second - raster_scalar_min_max.first);

			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					visual_layer_params->get_colour_palette_parameters();

			// Ensure max not less than min.
			if (max_value < colour_palette_parameters.get_palette_range().first/*min*/ + min_range_delta)
			{
				max_value = colour_palette_parameters.get_palette_range().first/*min*/ + min_range_delta;
			}

			colour_palette_parameters.map_palette_range(
					colour_palette_parameters.get_palette_range().first/*min*/,
					max_value/*max*/);

			visual_layer_params->set_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_palette_range_restore_min_max_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> raster_scalar_min_max = get_raster_scalar_min_max(layer);

			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					params->get_colour_palette_parameters();

			colour_palette_parameters.map_palette_range(
					raster_scalar_min_max.first,
					raster_scalar_min_max.second);

			params->set_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_palette_range_restore_mean_deviation_button_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
			const std::pair<double, double> raster_scalar_mean_std_dev = get_raster_scalar_mean_std_dev(layer);

			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					params->get_colour_palette_parameters();

			double range_min = raster_scalar_mean_std_dev.first -
					colour_palette_parameters.get_deviation_from_mean() * raster_scalar_mean_std_dev.second;
			double range_max = raster_scalar_mean_std_dev.first +
					colour_palette_parameters.get_deviation_from_mean() * raster_scalar_mean_std_dev.second;

			colour_palette_parameters.map_palette_range(range_min, range_max);

			params->set_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_palette_range_restore_mean_deviation_spinbox_changed(
		double deviation_from_mean)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			GPlatesPresentation::RemappedColourPaletteParameters colour_palette_parameters =
					visual_layer_params->get_colour_palette_parameters();

			colour_palette_parameters.set_deviation_from_mean(deviation_from_mean);

			visual_layer_params->set_colour_palette_parameters(colour_palette_parameters);
		}
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_opacity_spinbox_changed(
		double value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_opacity(value);
		}
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_intensity_spinbox_changed(
		double value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_intensity(value);
		}
	}
}


void
GPlatesQtWidgets::RasterLayerOptionsWidget::handle_surface_relief_scale_spinbox_changed(
		double value)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		GPlatesPresentation::RasterVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::RasterVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_surface_relief_scale(value);
		}
	}
}


std::pair<double, double>
GPlatesQtWidgets::RasterLayerOptionsWidget::get_raster_scalar_min_max(
		GPlatesAppLogic::Layer &layer) const
{
	const GPlatesAppLogic::RasterLayerTask::Params *layer_task_params =
		dynamic_cast<const GPlatesAppLogic::RasterLayerTask::Params *>(
				&layer.get_layer_task_params());

	// Default values result in clearing the colour scale widget.
	double raster_scalar_min = 0;
	double raster_scalar_max = 0;
	if (layer_task_params)
	{
		const GPlatesPropertyValues::RasterStatistics raster_statistic =
				layer_task_params->get_band_statistic();
		if (raster_statistic.minimum &&
			raster_statistic.maximum)
		{
			raster_scalar_min = raster_statistic.minimum.get();
			raster_scalar_max = raster_statistic.maximum.get();
		}
	}

	return std::make_pair(raster_scalar_min, raster_scalar_max);
}


std::pair<double, double>
GPlatesQtWidgets::RasterLayerOptionsWidget::get_raster_scalar_mean_std_dev(
		GPlatesAppLogic::Layer &layer) const
{
	const GPlatesAppLogic::RasterLayerTask::Params *layer_task_params =
		dynamic_cast<const GPlatesAppLogic::RasterLayerTask::Params *>(
				&layer.get_layer_task_params());

	// Default values result in clearing the colour scale widget.
	double raster_scalar_mean = 0;
	double raster_scalar_std_dev = 0;
	if (layer_task_params)
	{
		const GPlatesPropertyValues::RasterStatistics raster_statistic =
				layer_task_params->get_band_statistic();
		if (raster_statistic.mean &&
			raster_statistic.standard_deviation)
		{
			raster_scalar_mean = raster_statistic.mean.get();
			raster_scalar_std_dev = raster_statistic.standard_deviation.get();
		}
	}

	return std::make_pair(raster_scalar_mean, raster_scalar_std_dev);
}


GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type
GPlatesQtWidgets::RasterLayerOptionsWidget::load_colour_palette(
		const QString &palette_file_name,
		std::pair<double, double> &colour_palette_range,
		const GPlatesPresentation::RasterVisualLayerParams &params)
{
	// Only allow loading an integer colour palette if the raster is integer-valued and
	// the user is not remapping the colour palette.
	bool allow_integer_colour_palette = false;
	if (GPlatesPropertyValues::RasterType::is_integer(params.get_raster_type()) &&
		!params.get_colour_palette_parameters().is_palette_range_mapped())
	{
		allow_integer_colour_palette = true;
	}

	GPlatesFileIO::ReadErrorAccumulation cpt_read_errors;
	GPlatesGui::RasterColourPalette::non_null_ptr_to_const_type raster_colour_palette =
			GPlatesGui::ColourPaletteUtils::read_cpt_raster_colour_palette(
					palette_file_name,
					allow_integer_colour_palette,
					cpt_read_errors);

	// Copy the input value range to the caller.
	boost::optional< std::pair<double, double> > range =
			GPlatesGui::ColourPaletteUtils::get_range(*raster_colour_palette);
	if (range)
	{
		colour_palette_range = std::make_pair(range->first, range->second);
	}
	else
	{
		// Null range for empty colour palette.
		colour_palette_range = std::pair<double, double>(0,0);
	}

	// Show any read errors.
	if (cpt_read_errors.size() > 0)
	{
		ReadErrorAccumulationDialog &read_errors_dialog =
				d_viewport_window->dialogs().read_error_accumulation_dialog();

		read_errors_dialog.read_errors().accumulate(cpt_read_errors);
		read_errors_dialog.update();
		read_errors_dialog.show();
	}

	return raster_colour_palette;
}
