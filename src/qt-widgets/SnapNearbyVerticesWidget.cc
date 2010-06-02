/* $Id$ */
/**
* \file 
* $Revision$
* $Date$ 
* 
* Copyright (C) 2009 Geological Survey of Norway
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

#include "boost/optional.hpp"

#include "feature-visitors/PropertyValueFinder.h"
#include "gui/FeatureFocus.h"
#include "model/FeatureHandle.h"
#include "model/PropertyName.h"
#include "presentation/ViewState.h"
#include "property-values/GpmlPlateId.h"


#include "SnapNearbyVerticesWidget.h"
#include "TaskPanel.h"

static const double DEFAULT_THRESHOLD_DEGREES = 0.5;

namespace{
	
	boost::optional<GPlatesModel::integer_plate_id_type>
	get_conjugate_plate_id(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_handle)
	{
		if (!feature_handle->reference().is_valid())
		{
			return boost::none;
		}		
		static const GPlatesModel::PropertyName property_name =
			GPlatesModel::PropertyName::create_gpml("conjugatePlateId");

		const GPlatesPropertyValues::GpmlPlateId *plate_id = NULL;

		if (GPlatesFeatureVisitors::get_property_value(
				feature_handle, property_name, plate_id))
		{
			return boost::optional<GPlatesModel::integer_plate_id_type>(plate_id->value());
		}

		return boost::none;
	}
}

GPlatesQtWidgets::SnapNearbyVerticesWidget::SnapNearbyVerticesWidget(
	GPlatesQtWidgets::TaskPanel *task_panel_,
	GPlatesPresentation::ViewState &view_state,
	QWidget *parent_):
	QWidget(parent_),
	d_task_panel_ptr(task_panel_),
	d_feature_focus_ptr(&(view_state.get_feature_focus()))
{
	setupUi(this);
	setup_connections();
	set_default_widget_values();	
}

void
GPlatesQtWidgets::SnapNearbyVerticesWidget::set_default_widget_values()
{
	checkbox_vertices->setCheckState(Qt::Unchecked);
	frame_vertices->setEnabled(false);

	checkbox_plate_id->setCheckState(Qt::Unchecked);

	spinbox_plate_id->setEnabled(false);
	spinbox_plate_id->setValue(0);
	frame_plate_id->setEnabled(false);
	
	spinbox_threshold->setValue(DEFAULT_THRESHOLD_DEGREES);

	send_update_signal();
}



void
GPlatesQtWidgets::SnapNearbyVerticesWidget::setup_connections()
{
	QObject::connect(
		checkbox_vertices,
		SIGNAL(stateChanged(int)),
		this,
		SLOT(handle_vertex_checkbox_changed(int)));
		
	QObject::connect(
		checkbox_plate_id,
		SIGNAL(stateChanged(int)),
		this,
		SLOT(handle_plate_checkbox_changed(int)));
		
	QObject::connect(
		spinbox_threshold,
		SIGNAL(valueChanged(double)),
		this,
		SLOT(handle_spinbox_threshold_changed(double)));
		
	QObject::connect(
		spinbox_plate_id,
		SIGNAL(valueChanged(int)),
		this,
		SLOT(handle_spinbox_plate_id_changed(int)));

}

void
GPlatesQtWidgets::SnapNearbyVerticesWidget::handle_vertex_checkbox_changed(
	int state)
{
	frame_vertices->setEnabled(state);
	send_update_signal();
}

void
GPlatesQtWidgets::SnapNearbyVerticesWidget::handle_plate_checkbox_changed(
	int state)
{
	frame_plate_id->setEnabled(true);
	//spinbox_plate_id->setEnabled(radio_button_plate_id->isChecked());
	spinbox_plate_id->setEnabled(true);

	// Grab the actual plate-id value and emit the update signal.
	send_update_signal();
}

void
GPlatesQtWidgets::SnapNearbyVerticesWidget::send_update_signal()
{
	unsigned int plate_id_value = 0;

	if (checkbox_plate_id->isChecked())
	{
		plate_id_value = spinbox_plate_id->value();
	}
	d_task_panel_ptr->emit_vertex_data_changed_signal(
		checkbox_vertices->isChecked(),
		spinbox_threshold->value(),
		checkbox_plate_id->isChecked(),
		plate_id_value);
		
}

void
GPlatesQtWidgets::SnapNearbyVerticesWidget::handle_spinbox_threshold_changed(
	double threshold)
{
	send_update_signal();
}

void
GPlatesQtWidgets::SnapNearbyVerticesWidget::handle_spinbox_plate_id_changed(
	int plate_id)
{
	send_update_signal();
}



