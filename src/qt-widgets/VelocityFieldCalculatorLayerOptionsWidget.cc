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

#include <boost/static_assert.hpp>

#include "VelocityFieldCalculatorLayerOptionsWidget.h"

#include "TotalReconstructionPolesDialog.h"
#include "ViewportWindow.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/VelocityFieldCalculatorLayerTask.h"

#include "global/GPlatesAssert.h"

#include "presentation/VelocityFieldCalculatorVisualLayerParams.h"


namespace
{
	const QString HELP_SOLVE_VELOCITIES_METHOD_DIALOG_TITLE = QObject::tr("Calculating velocities");
	const QString HELP_SOLVE_VELOCITIES_METHOD_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<h3>Select the method used to calculate velocities</h3>"
			"A velocity is calculated at each point in <i>velocity domain</i> features using a plate id.\n"
			"The <i>velocity domain</i> features can contain point, multi-point, polyline or polygon geometries.\n"
			"<p>The options for obtaining the plate id at each point in these geometries are:</p>"
			"<h4>Calculate velocities on surfaces:</h4>"
			"<ul>"
			"<li>At each time step the static/dynamic polygons/networks connected to "
			"<i>velocity surfaces</i> are used to get plate ids for the points in the "
			"<i>velocity domain</i> features based on which surfaces they intersect.<li>"
			"<li>The <i>velocity surfaces</i> can be static polygons or topological plate polygons/networks.</li>"
			"<li>This option <b>ignores</b> plate ids in the <i>velocity domain</i> features.<li>"
			"<li>To use this option, layers containing the surfaces should be connected to the <i>velocity surfaces</i> layer input.<li>"
			"</ul>"
			"<h4>Calculate velocities by plate id:</h4>\n"
			"<ul>"
			"<li>The plate id stored in each <i>velocity domain</i> feature is used.<li>"
			"<li>Any layers currently connected to the <i>velocity surfaces</i> layer input are <b>ignored</b>.<li>"
			"</ul>"
			"</body></html>\n");

	const QString HELP_ARROW_SPACING_DIALOG_TITLE = QObject::tr("Spacing between arrows");
	const QString HELP_ARROW_SPACING_DIALOG_TEXT = QObject::tr(
			"<html><body>\n"
			"<p>This parameter limits the number of velocity arrows that can be displayed on the screen or monitor.</p>"
			"<p>This is achieved by dividing the globe into equal area regions where the area of each region "
			"is controlled by this parameter. If there are too many arrows in a region then only the arrow closest to "
			"the centre of the region is displayed and this rule is repeated for each region. "
			"In this way only a limited number of arrows are rendered and they are distributed evenly across the globe.</p>"
			"<p>The density of arrows on the screen is <i>independent</i> of the zoom level. "
			"That is, the number of arrows per unit screen area remains constant across the zoom levels.</p>"
			"<p>Select the 'X' button to remove any limit to the number of arrows on the screen.</p>"
			"</body></html>\n");
}


GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::VelocityFieldCalculatorLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_view_state(view_state),
	d_viewport_window(viewport_window),
	d_help_solve_velocities_method_dialog(
			new InformationDialog(
					HELP_SOLVE_VELOCITIES_METHOD_DIALOG_TEXT,
					HELP_SOLVE_VELOCITIES_METHOD_DIALOG_TITLE,
					viewport_window)),
	d_help_arrow_spacing_dialog(
			new InformationDialog(
					HELP_ARROW_SPACING_DIALOG_TEXT,
					HELP_ARROW_SPACING_DIALOG_TITLE,
					viewport_window))
{
	setupUi(this);
	triangulation_checkbox->setCursor(QCursor(Qt::ArrowCursor));
	solve_velocities_method_combobox->setCursor(QCursor(Qt::ArrowCursor));
	push_button_help_solve_velocities_method->setCursor(QCursor(Qt::ArrowCursor));
	arrow_spacing_spinbox->setCursor(QCursor(Qt::ArrowCursor));
	push_button_help_arrow_spacing->setCursor(QCursor(Qt::ArrowCursor));
	push_button_unlimited_arrow_spacing->setCursor(QCursor(Qt::ArrowCursor));

#if 0
	QObject::connect(
			constrained_checkbox,
			SIGNAL(clicked()),
			this,
			SLOT(handle_constrained_clicked()));
#endif

	QObject::connect(
			triangulation_checkbox, SIGNAL(clicked()),
			this, SLOT(handle_triangulation_clicked()));
	QObject::connect(
			solve_velocities_method_combobox, SIGNAL(activated(int)),
			this, SLOT(handle_solve_velocity_method_combobox_activated(int)));
	QObject::connect(
			arrow_spacing_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(handle_arrow_spacing_value_changed(double)));
	QObject::connect(
			push_button_unlimited_arrow_spacing, SIGNAL(clicked()),
			this, SLOT(handle_unlimited_arrow_spacing_clicked()));

	// Connect the help dialogs.
	QObject::connect(
			push_button_help_solve_velocities_method, SIGNAL(clicked()),
			d_help_solve_velocities_method_dialog, SLOT(show()));
	QObject::connect(
			push_button_help_arrow_spacing, SIGNAL(clicked()),
			d_help_arrow_spacing_dialog, SLOT(show()));
}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new VelocityFieldCalculatorLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;

	// Set the state of the checkboxes.
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (layer_task_params)
		{
			typedef GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params layer_task_params_type;
			// Update this source code if more 'solve velocities' enumeration values have been added (or removed).
			BOOST_STATIC_ASSERT(layer_task_params_type::NUM_SOLVE_VELOCITY_METHODS == 2);

			// Populate the 'solve velocities' combobox.
			solve_velocities_method_combobox->clear();
			for (int i = 0; i < layer_task_params_type::NUM_SOLVE_VELOCITY_METHODS; ++i)
			{
				const layer_task_params_type::SolveVelocitiesMethodType solve_velocities_method =
						static_cast<layer_task_params_type::SolveVelocitiesMethodType>(i);

				// "Calculate velocities "....
				switch (solve_velocities_method)
				{
				case layer_task_params_type::SOLVE_VELOCITIES_ON_SURFACES:
					solve_velocities_method_combobox->addItem("on surfaces");
					break;
				case layer_task_params_type::SOLVE_VELOCITIES_BY_PLATE_ID:
					solve_velocities_method_combobox->addItem("by plate id");
					break;
				default:
					GPlatesGlobal::Abort(GPLATES_ASSERTION_SOURCE);
					break;
				}
			}

			solve_velocities_method_combobox->setCurrentIndex(layer_task_params->get_solve_velocities_method());
		}

		GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (visual_layer_params)
		{
			//constrained_checkbox->setChecked(visual_layer_params->show_constrained_vectors());
			triangulation_checkbox->setChecked(visual_layer_params->show_delaunay_vectors());

			// Setting values in a spin box will emit signals if the value changes
			// which can lead to an infinitely recursive decent.
			// To avoid this we temporarily disconnect the signals.
			QObject::disconnect(
					arrow_spacing_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_arrow_spacing_value_changed(double)));
			arrow_spacing_spinbox->setValue(visual_layer_params->get_arrow_spacing());
			QObject::connect(
					arrow_spacing_spinbox, SIGNAL(valueChanged(double)),
					this, SLOT(handle_arrow_spacing_value_changed(double)));
		}
	}
}


const QString &
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::get_title()
{
	static const QString TITLE = tr("Velocity & Interpolation options");
	return TITLE;
}



void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_constrained_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			//params->set_show_constrained_vectors(constrained_checkbox->isChecked());
		}
	}
}

void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_triangulation_clicked()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_show_delaunay_vectors(triangulation_checkbox->isChecked());
		}
	}
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_solve_velocity_method_combobox_activated(
		int index)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer =
			d_current_visual_layer.lock())
	{
		// Set the band name in the app-logic layer params.
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (layer_task_params)
		{
			if (index < 0 || index >= GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params::NUM_SOLVE_VELOCITY_METHODS)
			{
				return;
			}

			// If the combobox choice has not changed then return early.
			if (index == layer_task_params->get_solve_velocities_method())
			{
				return;
			}

			layer_task_params->set_solve_velocities_method(
					static_cast<GPlatesAppLogic::VelocityFieldCalculatorLayerTask::Params::SolveVelocitiesMethodType>(
							index));
		}
	}
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_arrow_spacing_value_changed(
		double arrow_spacing)
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
		GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *params =
			dynamic_cast<GPlatesPresentation::VelocityFieldCalculatorVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (params)
		{
			params->set_arrow_spacing(arrow_spacing);
		}
	}
}


void
GPlatesQtWidgets::VelocityFieldCalculatorLayerOptionsWidget::handle_unlimited_arrow_spacing_clicked()
{
	// Set to the minimum value.
	// This will also display the special value text of "Not limited".
	arrow_spacing_spinbox->setValue(0);
}
