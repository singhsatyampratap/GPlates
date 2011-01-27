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

#include "ReconstructLayerOptionsWidget.h"

#include "SetVGPVisibilityDialog.h"
#include "ViewportWindow.h"


GPlatesQtWidgets::ReconstructLayerOptionsWidget::ReconstructLayerOptionsWidget(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_) :
	LayerOptionsWidget(parent_),
	d_application_state(application_state),
	d_viewport_window(viewport_window),
	d_set_vgp_visibility_dialog(NULL)
{
	setupUi(this);

	QObject::connect(
			set_vgp_visibility_button,
			SIGNAL(clicked()),
			this,
			SLOT(open_vgp_visibility_dialog()));
}


GPlatesQtWidgets::LayerOptionsWidget *
GPlatesQtWidgets::ReconstructLayerOptionsWidget::create(
		GPlatesAppLogic::ApplicationState &application_state,
		GPlatesPresentation::ViewState &view_state,
		ViewportWindow *viewport_window,
		QWidget *parent_)
{
	return new ReconstructLayerOptionsWidget(
			application_state,
			view_state,
			viewport_window,
			parent_);
}


void
GPlatesQtWidgets::ReconstructLayerOptionsWidget::set_data(
		const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer)
{
	d_current_visual_layer = visual_layer;
}


const QString &
GPlatesQtWidgets::ReconstructLayerOptionsWidget::get_title()
{
	static const QString TITLE = "Reconstruction options";
	return TITLE;
}


void
GPlatesQtWidgets::ReconstructLayerOptionsWidget::open_vgp_visibility_dialog()
{
	if (!d_set_vgp_visibility_dialog)
	{
		d_set_vgp_visibility_dialog = new SetVGPVisibilityDialog(
				d_application_state,
				&d_viewport_window->layers_dialog());
	}

	d_set_vgp_visibility_dialog->populate(d_current_visual_layer);

	// This dialog is shown modally.
	d_set_vgp_visibility_dialog->exec();
}
