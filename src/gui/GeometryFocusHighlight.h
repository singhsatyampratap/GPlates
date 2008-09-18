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

#ifndef GPLATES_GUI_GEOMETRYFOCUSHIGHLIGHT_H
#define GPLATES_GUI_GEOMETRYFOCUSHIGHLIGHT_H

#include <QObject>
#include <vector>
#include <boost/optional.hpp>

#include "gui/RenderedGeometryLayers.h"
#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"


namespace GPlatesGui
{
	/**
	 * This class is used to control the highlighting of the currently-focused reconstruction
	 * geometry.
	 */
	class GeometryFocusHighlight:
			public QObject
	{
		Q_OBJECT
	public:

		GeometryFocusHighlight(
				RenderedGeometryLayers::rendered_geometry_layer_type &highlight_layer):
			d_highlight_layer_ptr(&highlight_layer)
		{  }

		virtual
		~GeometryFocusHighlight()
		{  }

	public slots:

		/**
		 * Change which reconstruction geometry is currently focused, also specifying an
		 * (optional) weak-ref to the feature which contains the geometry whose RFG is the
		 * currently-focused reconstruction geometry.
		 *
		 * The counter-intuitive ordering of the arguments (feature first, followed by the
		 * reconstruction geometry, when the geometry is the "main" argument) is so that
		 * this slot matches the @a focus_changed signal emitted by FeatureFocus.
		 */
		void
		set_focus(
				GPlatesModel::FeatureHandle::weak_ref feature_ref,
				GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type focused_geometry);

		/**
		 * Draw the focused geometry (if there is one) on the screen.
		 */
		void
		draw_focused_geometry();

	signals:

		/**
		 * Emitted when the canvas should update (re-draw) itself.
		 */
		void
		canvas_should_update();

	private:

		/**
		 * The layer of rendered geometries which is used for highlighting.
		 */
		RenderedGeometryLayers::rendered_geometry_layer_type *d_highlight_layer_ptr;

		/**
		 * The feature which contains the geometry whose RFG is the currently-focused
		 * reconstruction geometry.
		 *
		 * Note that there might not be any such feature, in which case this would be an
		 * invalid weak-ref.
		 */
		GPlatesModel::FeatureHandle::weak_ref d_feature;

		/**
		 * The reconstruction geometry which is focused.
		 *
		 * Note that there may not be a focused reconstruction geometry, in which case this
		 * would be a null pointer.
		 */
		GPlatesModel::ReconstructionGeometry::maybe_null_ptr_type d_focused_geometry;

	};
}

#endif // GPLATES_GUI_GEOMETRYFOCUSHIGHLIGHT_H