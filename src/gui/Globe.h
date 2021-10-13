/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_GLOBE_H
#define GPLATES_GUI_GLOBE_H

#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "Colour.h"
#include "ColourScheme.h"
#include "GlobeRenderedGeometryCollectionPainter.h"
#include "OpaqueSphere.h"
#include "PersistentOpenGLObjects.h"
#include "SphericalGrid.h"
#include "SimpleGlobeOrientation.h"
#include "Stars.h"
#include "TextRenderer.h"
#include "RenderSettings.h"

#include "maths/UnitVector3D.h"
#include "maths/PointOnSphere.h"
#include "maths/Rotation.h"

#include "opengl/GLContext.h"
#include "opengl/GLMatrix.h"

#include "presentation/VisualLayers.h"

#include "utils/VirtualProxy.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
}

namespace GPlatesPresentation
{
	class ViewState;
}

namespace GPlatesViewOperations
{
	class RenderedGeometryCollection;
}

namespace GPlatesGui
{
	class GlobeVisibilityTester;

	class Globe
	{
	public:
		/**
		 * Typedef for an opaque object that caches a particular painting.
		 */
		typedef boost::shared_ptr<void> cache_handle_type;


		Globe(
				GPlatesPresentation::ViewState &view_state,
				const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
				GPlatesViewOperations::RenderedGeometryCollection &rendered_geom_collection,
				const GPlatesPresentation::VisualLayers &visual_layers,
				RenderSettings &render_settings,
				const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
				const GlobeVisibilityTester &visibility_tester,
				ColourScheme::non_null_ptr_type colour_scheme);

		//! To clone a Globe
		Globe(
				Globe &existing_globe,
				const PersistentOpenGLObjects::non_null_ptr_type &persistent_opengl_objects,
				const TextRenderer::non_null_ptr_to_const_type &text_renderer_ptr,
				const GlobeVisibilityTester &visibility_tester,
				ColourScheme::non_null_ptr_type colour_scheme);

		~Globe()
		{  }

		/**
		 * Initialise any OpenGL state.
		 *
		 * This method is called when the OpenGL context is first bound (and hence we can make OpenGL calls).
		 */
		void
		initialiseGL(
				GPlatesOpenGL::GLRenderer &renderer);

		SimpleGlobeOrientation &
		orientation() const
		{
			return *d_globe_orientation_ptr;
		}

		void
		set_new_handle_pos(
				const GPlatesMaths::PointOnSphere &pos);

		void
		update_handle_pos(
				const GPlatesMaths::PointOnSphere &pos);

		const GPlatesMaths::PointOnSphere
		orient(
				const GPlatesMaths::PointOnSphere &pos) const;

		/**
		 * Paint the globe and all the visible features and rasters on it.
		 *
		 * The three projection transforms differ only in their far clip plane distance.
		 * One includes only the front half of the globe, another includes the full globe and
		 * another is long enough to allow rendering of the stars.
		 *
		 * @param viewport_zoom_factor The magnification of the globe in the viewport window.
		 *        Value should be one when earth fills viewport and proportionately greater
		 *        than one when viewport shows only part of the globe.
		 */
		cache_handle_type
		paint(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &viewport_zoom_factor,
				float scale,
				const GPlatesOpenGL::GLMatrix &projection_transform_include_half_globe,
				const GPlatesOpenGL::GLMatrix &projection_transform_include_full_globe,
				const GPlatesOpenGL::GLMatrix &projection_transform_include_stars);

		/*
		 * A special version of the globe's paint() method more suitable for vector output.
		 *
		 * NOTE: Unlike @a paint the caller must have pushed the projection transform onto @a renderer.
		 *
		 * @param viewport_zoom_factor The magnification of the globe in the viewport window.
		 *        Value should be one when earth fills viewport and proportionately greater
		 *        than one when viewport shows only part of the globe.
		 */
		void
		paint_vector_output(
				GPlatesOpenGL::GLRenderer &renderer,
				const double &viewport_zoom_factor,
				float scale);

	private:

		GPlatesPresentation::ViewState &d_view_state;

		/**
		 * Keeps track of OpenGL-related objects that persist from one render to the next.
		 */
		const PersistentOpenGLObjects::non_null_ptr_type d_persistent_opengl_objects;
			
		//! Flags to determine what data to show
		RenderSettings &d_render_settings;
		
		//! The collection of @a RenderedGeometry objects we need to paint.
		GPlatesViewOperations::RenderedGeometryCollection &d_rendered_geom_collection;

		const GPlatesPresentation::VisualLayers &d_visual_layers;

		/**
		 * Stars in the background, behind the Earth.
		 *
		 * It's optional since it can't be constructed until @a initialiseGL is called (valid OpenGL context).
		 */
		boost::optional<Stars> d_stars;

		/**
		 * The solid earth.
		 *
		 * It's optional since it can't be constructed until @a initialiseGL is called (valid OpenGL context).
		 */
		boost::optional<OpaqueSphere> d_sphere;

		/**
		 * Assists with rendering when @a d_sphere is translucent.
		 *
		 * It's optional since it can't be constructed until @a initialiseGL is called (valid OpenGL context).
		 */
		boost::optional<OpaqueSphere> d_black_sphere;

		/**
		 * Lines of lat and lon on surface of earth.
		 *
		 * It's optional since it can't be constructed until @a initialiseGL is called (valid OpenGL context).
		 */
		boost::optional<SphericalGrid> d_grid;

		/**
		 * The accumulated orientation of the globe.
		 */
		boost::shared_ptr<SimpleGlobeOrientation> d_globe_orientation_ptr;

		/**
		 * Painter used to draw @a RenderedGeometry objects on the globe.
		 */
		GlobeRenderedGeometryCollectionPainter d_rendered_geom_collection_painter;

		/**
		 * Calculate tranform to ransform the view according to the current globe orientation.
		 */
		void
		get_globe_orientation_transform(
				GPlatesOpenGL::GLMatrix &transform) const;
	};
}

#endif  // GPLATES_GUI_GLOBE_H
