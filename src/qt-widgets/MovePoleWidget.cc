/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2014 The University of Sydney, Australia
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

#include <utility>
#include <QDebug>
#include <QString>
#include <QtGlobal>

#include "MovePoleWidget.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructionGeometryUtils.h"
#include "app-logic/ReconstructionTree.h"
#include "app-logic/ReconstructionTreeCreator.h"
#include "app-logic/RotationUtils.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "gui/FeatureFocus.h"

#include "maths/FiniteRotation.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/UnitQuaternion3D.h"
#include "maths/UnitVector3D.h"

#include "presentation/ViewState.h"


GPlatesQtWidgets::MovePoleWidget::MovePoleWidget(
		GPlatesPresentation::ViewState &view_state,
		QWidget *parent_):
	TaskPanelWidget(parent_),
	d_feature_focus(view_state.get_feature_focus())
{
	setupUi(this);

	// Initialise the widget state based on the pole.

	enable_pole_checkbox->setChecked(d_pole);
	pole_widget->setEnabled(d_pole);
	// We can only get a stage pole if a feature is focused.
	constrain_to_stage_pole_pushbutton->setEnabled(
			bool(d_feature_focus.associated_reconstruction_geometry()));

	if (d_pole)
	{
		const GPlatesMaths::LatLonPoint lat_lon_pole = GPlatesMaths::make_lat_lon_point(d_pole.get());

		latitude_spinbox->setValue(lat_lon_pole.latitude());
		longitude_spinbox->setValue(lat_lon_pole.longitude());
	}
	else // set to the north pole...
	{
		latitude_spinbox->setValue(90);
		longitude_spinbox->setValue(0);
	}

	update_stage_pole_moving_fixed_plate_ids();

	make_signal_slot_connections(view_state);
}


GPlatesQtWidgets::MovePoleWidget::~MovePoleWidget()
{
	// boost::scoped_ptr destructor requires complete type.
}


void
GPlatesQtWidgets::MovePoleWidget::set_pole(
		boost::optional<GPlatesMaths::PointOnSphere> pole)
{
	// Return early if no state change.
	if (pole == d_pole)
	{
		return;
	}

	d_pole = pole;

	// Disconnect enable pole checkbox and lat/lon spinbox slots.
	// We only want to emit 'pole_changed' signal once.
	QObject::disconnect(
			enable_pole_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_pole_check_box_changed()));
	QObject::disconnect(
			latitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_latitude_spinbox_changed()));
	QObject::disconnect(
			longitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_longitude_spinbox_changed()));

	// Enable or disable pole.
	enable_pole_checkbox->setChecked(d_pole);

	if (d_pole)
	{
		const GPlatesMaths::LatLonPoint lat_lon_pole = GPlatesMaths::make_lat_lon_point(d_pole.get());

		latitude_spinbox->setValue(lat_lon_pole.latitude());
		longitude_spinbox->setValue(lat_lon_pole.longitude());
	}

	// Reconnect enable pole checkbox and lat/lon spinbox slots.
	QObject::connect(
			enable_pole_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_pole_check_box_changed()));
	QObject::connect(
			latitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_latitude_spinbox_changed()));
	QObject::connect(
			longitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_longitude_spinbox_changed()));

	Q_EMIT pole_changed(d_pole);
}


void
GPlatesQtWidgets::MovePoleWidget::activate()
{
	// Enable the task panel widget.
	setEnabled(true);
}


void
GPlatesQtWidgets::MovePoleWidget::deactivate()
{
	// Disable the task panel widget.
	setEnabled(false);
}


void
GPlatesQtWidgets::MovePoleWidget::set_focus()
{
	// We can only get a stage pole if a feature is focused.
	constrain_to_stage_pole_pushbutton->setEnabled(
			bool(d_feature_focus.associated_reconstruction_geometry()));

	update_stage_pole_moving_fixed_plate_ids();
}


void
GPlatesQtWidgets::MovePoleWidget::handle_reconstruction()
{
	update_stage_pole_moving_fixed_plate_ids();
}


void
GPlatesQtWidgets::MovePoleWidget::react_enable_pole_check_box_changed()
{
	// Return early if no state change.
	if (enable_pole_checkbox->isChecked() == bool(d_pole))
	{
		return;
	}

	if (enable_pole_checkbox->isChecked())
	{
		d_pole = GPlatesMaths::make_point_on_sphere(
				GPlatesMaths::LatLonPoint(latitude_spinbox->value(), longitude_spinbox->value()));
	}
	else
	{
		d_pole = boost::none;
	}

	// Enable/disable ability to modify the pole.
	pole_widget->setEnabled(d_pole);

	Q_EMIT pole_changed(d_pole);
}


void
GPlatesQtWidgets::MovePoleWidget::react_latitude_spinbox_changed()
{
	// Should only be able to change spinbox value if pole is enabled.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_pole,
			GPLATES_ASSERTION_SOURCE);

	const GPlatesMaths::LatLonPoint lat_lon_pole = GPlatesMaths::make_lat_lon_point(d_pole.get());

	// Return early if no state change.
	if (GPlatesMaths::are_almost_exactly_equal(latitude_spinbox->value(), lat_lon_pole.latitude()))
	{
		return;
	}

	// NOTE: Use longitude spinbox value instead of 'd_pole' longitude value because the later gets
	// reset by PointOnSphere to LatLonPoint conversion at the north/south pole.
	d_pole = GPlatesMaths::make_point_on_sphere(
			GPlatesMaths::LatLonPoint(latitude_spinbox->value(), longitude_spinbox->value()));

	Q_EMIT pole_changed(d_pole);
}


void
GPlatesQtWidgets::MovePoleWidget::react_longitude_spinbox_changed()
{
	// Should only be able to change spinbox value if pole is enabled.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_pole,
			GPLATES_ASSERTION_SOURCE);

	const GPlatesMaths::LatLonPoint lat_lon_pole = GPlatesMaths::make_lat_lon_point(d_pole.get());

	// Return early if no state change.
	if (GPlatesMaths::are_almost_exactly_equal(longitude_spinbox->value(), lat_lon_pole.longitude()))
	{
		return;
	}

	d_pole = GPlatesMaths::make_point_on_sphere(
			GPlatesMaths::LatLonPoint(lat_lon_pole.latitude(), longitude_spinbox->value()));

	Q_EMIT pole_changed(d_pole);
}


void
GPlatesQtWidgets::MovePoleWidget::react_north_pole_pushbutton_clicked()
{
	// Should only be able to set north pole if pole is enabled.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			d_pole,
			GPLATES_ASSERTION_SOURCE);

	const GPlatesMaths::LatLonPoint lat_lon_pole = GPlatesMaths::make_lat_lon_point(d_pole.get());

	// Return early if no state change.
	if (GPlatesMaths::are_almost_exactly_equal(90, lat_lon_pole.latitude()) &&
		GPlatesMaths::are_almost_exactly_equal(0, lat_lon_pole.longitude()))
	{
		return;
	}

	set_pole(GPlatesMaths::make_point_on_sphere(GPlatesMaths::LatLonPoint(90, 0)));
}


void
GPlatesQtWidgets::MovePoleWidget::react_stage_pole_pushbutton_clicked()
{
	// Get stage pole axis.
	boost::optional<GPlatesMaths::PointOnSphere> stage_pole_location = get_stage_pole_location();

	// Use north pole if no stage pole or represents identity rotation.
	// This is a visual indicator to the user that the stage pole is not available.
	if (!stage_pole_location)
	{
		stage_pole_location = GPlatesMaths::PointOnSphere(GPlatesMaths::UnitVector3D::zBasis());
	}

	const GPlatesMaths::LatLonPoint stage_pole_axis_lat_lon =
			GPlatesMaths::make_lat_lon_point(stage_pole_location.get());

	set_pole(stage_pole_location);
}




void
GPlatesQtWidgets::MovePoleWidget::react_keep_stage_pole_constrained_checkbox_changed()
{
}


void
GPlatesQtWidgets::MovePoleWidget::make_signal_slot_connections(
		GPlatesPresentation::ViewState &view_state)
{
	QObject::connect(
			&view_state.get_feature_focus(), SIGNAL(focus_changed(GPlatesGui::FeatureFocus &)),
			this, SLOT(set_focus()));
	QObject::connect(
			&view_state.get_application_state(), SIGNAL(reconstructed(GPlatesAppLogic::ApplicationState &)),
			this, SLOT(handle_reconstruction()));
	QObject::connect(
			enable_pole_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(react_enable_pole_check_box_changed()));
	QObject::connect(
			latitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_latitude_spinbox_changed()));
	QObject::connect(
			longitude_spinbox, SIGNAL(valueChanged(double)),
			this, SLOT(react_longitude_spinbox_changed()));
	QObject::connect(
			north_pole_pushbutton, SIGNAL(clicked(bool)),
			this, SLOT(react_north_pole_pushbutton_clicked()));
	QObject::connect(
			constrain_to_stage_pole_pushbutton, SIGNAL(clicked(bool)),
			this, SLOT(react_stage_pole_pushbutton_clicked()));
	QObject::connect(
			keep_stage_pole_constrained_checkbox, SIGNAL(stateChanged(int)),
			this, SLOT(react_keep_stage_pole_constrained_checkbox_changed()));
}


void
GPlatesQtWidgets::MovePoleWidget::update_stage_pole_moving_fixed_plate_ids()
{
	boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type>
			rfg = get_focused_feature_geometry();

	// Clear the stage pole moving/fixed plate IDs if there's no focused feature geometry.
	if (!rfg)
	{
		stage_pole_moving_plate_lineedit->clear();
		stage_pole_fixed_plate_lineedit->clear();
		return;
	}

	boost::optional< std::pair<GPlatesModel::integer_plate_id_type, GPlatesModel::integer_plate_id_type> >
			moving_fixed_plate_ids = get_stage_pole_plate_pair(*rfg.get());

	// Clear the stage pole moving/fixed plate IDs if there's no rotation file, for example.
	if (!moving_fixed_plate_ids)
	{
		stage_pole_moving_plate_lineedit->clear();
		stage_pole_fixed_plate_lineedit->clear();
		return;
	}

	// Update the moving/fixed plate IDs.
	QString moving_plate_id_string;
	moving_plate_id_string.setNum(moving_fixed_plate_ids->first);
	stage_pole_moving_plate_lineedit->setText(moving_plate_id_string);
	QString fixed_plate_id_string;
	fixed_plate_id_string.setNum(moving_fixed_plate_ids->second);
	stage_pole_fixed_plate_lineedit->setText(fixed_plate_id_string);
}


boost::optional<GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type>
GPlatesQtWidgets::MovePoleWidget::get_focused_feature_geometry() const
{
	GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_geometry =
			d_feature_focus.associated_reconstruction_geometry();
	if (!focused_geometry)
	{
		return boost::none;
	}

	// Like ModifyReconstructionPoleWidget we're only interested in ReconstructedFeatureGeometry's.
	boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> rfg =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ReconstructedFeatureGeometry>(focused_geometry);
	if (!rfg)
	{
		return boost::none;
	}

	return GPlatesAppLogic::ReconstructedFeatureGeometry::non_null_ptr_to_const_type(rfg.get());
}


boost::optional< std::pair<GPlatesModel::integer_plate_id_type, GPlatesModel::integer_plate_id_type> >
GPlatesQtWidgets::MovePoleWidget::get_stage_pole_plate_pair(
		const GPlatesAppLogic::ReconstructedFeatureGeometry &rfg) const
{
	boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id =
			rfg.reconstruction_plate_id();
	if (!reconstruction_plate_id)
	{
		return boost::none;
	}

	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			rfg.get_reconstruction_tree();

	GPlatesAppLogic::ReconstructionTree::edge_refs_by_plate_id_map_const_range_type edges =
			reconstruction_tree->find_edges_whose_moving_plate_id_match(
					reconstruction_plate_id.get());

	if (edges.first == edges.second)
	{
		// We haven't found any edges - might not have a rotation file loaded.
		return boost::none;
	}

	// We shouldn't have more than one edge - even in a cross-over situation, one
	// of the edges will already have been selected for use in the tree	
	if (std::distance(edges.first, edges.second) > 1)
	{
		qDebug() << "More than one edge found for reconstruction plate id " << reconstruction_plate_id.get();
		return boost::none;
	}

	GPlatesAppLogic::ReconstructionTree::edge_ref_type edge = edges.first->second;

	return std::make_pair(edge->moving_plate(), edge->fixed_plate());
}


boost::optional<GPlatesMaths::PointOnSphere>
GPlatesQtWidgets::MovePoleWidget::get_stage_pole_location() const
{
	const GPlatesAppLogic::ReconstructionGeometry::maybe_null_ptr_to_const_type focused_geometry =
			d_feature_focus.associated_reconstruction_geometry();
	if (!focused_geometry)
	{
		return boost::none;
	}

	// Like ModifyReconstructionPoleWidget we're only interested in ReconstructedFeatureGeometry's.
	boost::optional<const GPlatesAppLogic::ReconstructedFeatureGeometry *> rfg =
			GPlatesAppLogic::ReconstructionGeometryUtils::get_reconstruction_geometry_derived_type<
					const GPlatesAppLogic::ReconstructedFeatureGeometry>(focused_geometry);
	if (!rfg)
	{
		return boost::none;
	}

	boost::optional<GPlatesModel::integer_plate_id_type> reconstruction_plate_id =
			rfg.get()->reconstruction_plate_id();
	if (!reconstruction_plate_id)
	{
		return boost::none;
	}

	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree =
			rfg.get()->get_reconstruction_tree();

	GPlatesAppLogic::ReconstructionTree::edge_refs_by_plate_id_map_const_range_type edges =
			reconstruction_tree->find_edges_whose_moving_plate_id_match(
					reconstruction_plate_id.get());

	if (edges.first == edges.second)
	{
		// We haven't found any edges - might not have a rotation file loaded.
		return boost::none;
	}

	// We shouldn't have more than one edge - even in a cross-over situation, one
	// of the edges will already have been selected for use in the tree	
	if (std::distance(edges.first, edges.second) > 1)
	{
		qDebug() << "More than one edge found for reconstruction plate id " << reconstruction_plate_id.get();
		return boost::none;
	}

	GPlatesAppLogic::ReconstructionTree::edge_ref_type edge = edges.first->second;

	GPlatesAppLogic::ReconstructionTree::non_null_ptr_to_const_type reconstruction_tree_2 =
			rfg.get()->get_reconstruction_tree_creator().get_reconstruction_tree(
					rfg.get()->get_reconstruction_time() + 1);

	// Get stage pole.
	const GPlatesMaths::FiniteRotation stage_pole =
			GPlatesAppLogic::RotationUtils::get_stage_pole(
					*reconstruction_tree,
					*reconstruction_tree_2,
					edge->moving_plate(),
					edge->fixed_plate());

	// Get stage pole axis.
	if (GPlatesMaths::represents_identity_rotation(stage_pole.unit_quat()))
	{
		return boost::none;
	}

	// Return the stage pole axis.
	return GPlatesMaths::PointOnSphere(stage_pole.unit_quat().get_rotation_params(boost::none).axis);
}