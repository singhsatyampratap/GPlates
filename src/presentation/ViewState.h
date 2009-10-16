/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_PRESENTATION_VIEWSTATE_H
#define GPLATES_PRESENTATION_VIEWSTATE_H

#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <QObject>

////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: Please use forward declarations (and boost::scoped_ptr) instead of including headers
// where possible.
// This header gets included in a lot of other files and we want to reduce compile times.
////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
// FIXME remove this header
#include "view-operations/RenderedGeometryCollection.h"


namespace GPlatesAppLogic
{
	class ApplicationState;
	class FeatureCollectionFileIO;
	class PlateVelocities;
	class Reconstruct;
}

namespace GPlatesGui
{
	class Colour;
	class ColourTable;
	class ColourTableDelegator;
	class GeometryFocusHighlight;
	class FeatureFocus;
	class ViewportZoom;
}

namespace GPlatesModel
{
	class Reconstruction;
}

namespace GPlatesQtWidgets
{
	class ViewportWindow;
}

namespace GPlatesViewOperations
{
	class ReconstructView;
	class RenderedGeometryCollection;
	class ViewportProjection;
}

namespace GPlatesPresentation
{
	class ViewState :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT
		
	public:
		ViewState(
				GPlatesAppLogic::ApplicationState &application_state);

		~ViewState();


		GPlatesAppLogic::ApplicationState &
		get_application_state();


		///////////////////////////////////
		// FIXME: temporary horrible hack - remove this method when all state
		// accessed by this method has been moved in this class.
		/**
		 * Only use this method if you are accessing state that will ultimately
		 * be moved into this class (@a ViewState) - ie, stuff that does not involve
		 * Qt widgets.
		 * If you need to access Qt widget related stuff then pass a reference to
		 * ViewportWindow into your class/function - the idea is that when all
		 * relevant state has been moved over then this method will be removed.
		 */
		GPlatesQtWidgets::ViewportWindow &
		get_other_view_state()
		{
			return *d_other_view_state;
		}

		void
		set_other_view_state(
				GPlatesQtWidgets::ViewportWindow &viewport_window)
		{
			d_other_view_state = &viewport_window;
		}
		//
		///////////////////////////////////


		GPlatesAppLogic::Reconstruct &
		get_reconstruct();


		GPlatesViewOperations::RenderedGeometryCollection &
		get_rendered_geometry_collection();


		GPlatesGui::FeatureFocus &
		get_feature_focus();


		GPlatesGui::ViewportZoom &
		get_viewport_zoom();


		GPlatesViewOperations::ViewportProjection &
		get_viewport_projection();


		const GPlatesAppLogic::PlateVelocities &
		get_plate_velocities() const;


		//! Colour reconstruction geometry by plate id.
		void
		choose_colour_by_plate_id();

		//! Colour reconstruction geometry with a single colour.
		void
		choose_colour_by_single_colour(
				const GPlatesGui::Colour &colour);

		//! Colour reconstruction geometry by feature type.
		void
		choose_colour_by_feature_type();

		//! Colour reconstruction geometry by age.
		void
		choose_colour_by_age();

		/**
		 * Returns the colour table.
		 *
		 * When performing colour lookup with the returned colour table
		 * it will always refer to the latest colour table selected with
		 * @a choose_colour_by_age for example.
		 * This is because the returned colour table delegates colour lookup to an
		 * actual colour table implementation which itself can be switched inside the
		 * delegate.
		 * 
		 */
		GPlatesGui::ColourTable *
		get_colour_table();

	private slots:
		void
		handle_zoom_change();

	private:
		//
		// NOTE: Most of these are boost::scoped_ptr's to avoid having to include header files.
		//

		GPlatesAppLogic::ApplicationState &d_application_state;

		// FIXME: remove this when refactored
		GPlatesQtWidgets::ViewportWindow *d_other_view_state;

		//! Performs the reconstructions.
		boost::scoped_ptr<GPlatesAppLogic::Reconstruct> d_reconstruct;

		//! Contains all rendered geometries for this view state.
		boost::scoped_ptr<GPlatesViewOperations::RenderedGeometryCollection> d_rendered_geometry_collection;

		//! Keeps track of the currently selected colour table.
		boost::scoped_ptr<GPlatesGui::ColourTableDelegator> d_colour_table;

		//! The viewport zoom state.
		boost::scoped_ptr<GPlatesGui::ViewportZoom> d_viewport_zoom;

		//! The viewport projection state.
		boost::scoped_ptr<GPlatesViewOperations::ViewportProjection> d_viewport_projection;

		//! Renders the focused geometry highlighted.
		boost::scoped_ptr<GPlatesGui::GeometryFocusHighlight> d_geometry_focus_highlight;

		//! Tracks the currently focused feature (if any).
		boost::scoped_ptr<GPlatesGui::FeatureFocus> d_feature_focus;

		// FIXME: remove these 
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_comp_mesh_point_layer;
		GPlatesViewOperations::RenderedGeometryCollection::child_layer_owner_ptr_type
			d_comp_mesh_arrow_layer;
		boost::scoped_ptr<GPlatesAppLogic::PlateVelocities> d_plate_velocities;

		//! Performs tasks each time a reconstruction is generated.
		boost::scoped_ptr<GPlatesViewOperations::ReconstructView> d_reconstruct_view;


		void
		connect_to_viewport_zoom();

		void
		connect_to_file_state();

		void
		connect_to_file_io();

		void
		connect_to_feature_focus();

		void
		setup_rendered_geometry_collection();
	};
}

#endif // GPLATES_PRESENTATION_VIEWSTATE_H