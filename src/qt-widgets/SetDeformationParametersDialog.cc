/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <boost/optional.hpp> 
#include <boost/shared_ptr.hpp>

#include "SetDeformationParametersDialog.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructLayerTask.h"
#include "app-logic/ReconstructParams.h"

#include "presentation/ReconstructVisualLayerParams.h"
#include "presentation/VisualLayer.h"


GPlatesQtWidgets::SetDeformationParametersDialog::SetDeformationParametersDialog(
		GPlatesAppLogic::ApplicationState &application_state,
		QWidget *parent_) :
	QDialog(parent_),
	d_application_state(application_state)
{
	setupUi(this);

	// Enable/disable strain accumulation controls if showing/hiding strain accumulation.
	strain_accumulation_widget->setEnabled(
			show_strain_accumulation_checkbox->isChecked());

	setup_connections();
}


bool
GPlatesQtWidgets::SetDeformationParametersDialog::populate(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	// Store pointer so we can write the settings back later.
	d_current_visual_layer = visual_layer;

	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = visual_layer.lock())
	{
		// Acquire a pointer to a @a ReconstructParams.
		// NOTE: Make sure we get a 'const' pointer to the reconstruct layer task params
		// otherwise it will think we are modifying it which will mean the reconstruct
		// layer will think it needs to regenerate its reconstructed feature geometries.
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		const GPlatesAppLogic::ReconstructLayerTask::Params *layer_task_params =
			dynamic_cast<const GPlatesAppLogic::ReconstructLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (!layer_task_params)
		{
			return false;
		}

		// Acquire a pointer to a @a ReconstructVisualLayerParams.
		const GPlatesPresentation::ReconstructVisualLayerParams *visual_layer_params =
			dynamic_cast<const GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!visual_layer_params)
		{
			return false;
		}

		// Handle delta t.
		spinbox_end_time->setValue(
			layer_task_params->get_reconstruct_params().get_deformation_end_time());
		spinbox_begin_time->setValue(
			layer_task_params->get_reconstruct_params().get_deformation_begin_time());
		spinbox_time_increment->setValue(
			layer_task_params->get_reconstruct_params().get_deformation_time_increment());

		// Show deformed feature geometries.
		show_deformed_feature_geometries_checkbox->setChecked(
				visual_layer_params->get_show_deformed_feature_geometries());

		// Show strain accumulation.
		show_strain_accumulation_checkbox->setChecked(
				visual_layer_params->get_show_strain_accumulation());
		// Set strain accumulation scale.
		strain_accumulation_scale_spinbox->setValue(
				visual_layer_params->get_strain_accumulation_scale());

		return true;
	}

	return false;
}


void
GPlatesQtWidgets::SetDeformationParametersDialog::setup_connections()
{
	QObject::connect(
			main_buttonbox,
			SIGNAL(accepted()),
			this,
			SLOT(handle_apply()));
	QObject::connect(
			main_buttonbox,
			SIGNAL(rejected()),
			this,
			SLOT(reject()));

	QObject::connect(
			show_strain_accumulation_checkbox,
			SIGNAL(stateChanged(int)),
			this,
			SLOT(react_show_strain_accumulation_changed(int)));
}


void
GPlatesQtWidgets::SetDeformationParametersDialog::react_show_strain_accumulation_changed(
		int state)
{
	// Enable/disable strain accumulation controls if showing/hiding strain accumulation.
	strain_accumulation_widget->setEnabled(
			show_strain_accumulation_checkbox->isChecked());
}


void
GPlatesQtWidgets::SetDeformationParametersDialog::handle_apply()
{
	if (boost::shared_ptr<GPlatesPresentation::VisualLayer> locked_visual_layer = d_current_visual_layer.lock())
	{
//qDebug() << "SetDeformationParametersDialog::handle_apply() : if(locked_visual_layer)";

		// Acquire a pointer to a @a ReconstructParams.
		GPlatesAppLogic::Layer layer = locked_visual_layer->get_reconstruct_graph_layer();
		GPlatesAppLogic::ReconstructLayerTask::Params *layer_task_params =
			dynamic_cast<GPlatesAppLogic::ReconstructLayerTask::Params *>(
					&layer.get_layer_task_params());
		if (!layer_task_params)
		{
//qDebug() << "SetDeformationParametersDialog::handle_apply() : if(!layer_task_params)";
			accept();
		}

		// Acquire a pointer to a @a ReconstructVisualLayerParams.
		GPlatesPresentation::ReconstructVisualLayerParams *visual_layer_params =
			dynamic_cast<GPlatesPresentation::ReconstructVisualLayerParams *>(
					locked_visual_layer->get_visual_layer_params().get());
		if (!visual_layer_params)
		{
			accept();
		}

		// Handle settings
		layer_task_params->get_reconstruct_params().set_deformation_end_time(
			spinbox_end_time->value());
		layer_task_params->get_reconstruct_params().set_deformation_begin_time(
			spinbox_begin_time->value());
		layer_task_params->get_reconstruct_params().set_deformation_time_increment(
			spinbox_time_increment->value());

		// Show deformed feature geometries.
		visual_layer_params->set_show_deformed_feature_geometries(
				show_deformed_feature_geometries_checkbox->isChecked());

		// Show strain accumulation.
		visual_layer_params->set_show_strain_accumulation(
				show_strain_accumulation_checkbox->isChecked());
		// Set strain accumulation scale.
		visual_layer_params->set_strain_accumulation_scale(
				strain_accumulation_scale_spinbox->value());

//qDebug() << "SetDeformationParametersDialog::handle_apply() : PRE d_application_state.reconstruct()";
		// Tell GPlates to reconstruct now so that the updated render settings are used. 
		d_application_state.reconstruct();	
	}

	accept();
//qDebug() << "SetDeformationParametersDialog::handle_apply() : POST accept;";
}