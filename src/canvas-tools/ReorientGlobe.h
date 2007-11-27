/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2007 The University of Sydney, Australia
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

#ifndef GPLATES_CANVASTOOLS_REORIENTGLOBE_H
#define GPLATES_CANVASTOOLS_REORIENTGLOBE_H

#include "gui/CanvasTool.h"


namespace GPlatesCanvasTools
{
	/**
	 * This is the canvas tool used to re-orient the globe by dragging.
	 */
	class ReorientGlobe:
			public GPlatesGui::CanvasTool
	{
	public:
		/**
		 * A convenience typedef for GPlatesUtils::non_null_intrusive_ptr<ReorientGlobe>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReorientGlobe> non_null_ptr_type;

		virtual
		~ReorientGlobe()
		{  }

		/**
		 * Create a ReorientGlobe instance.
		 */
		static
		const non_null_ptr_type
		create(
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_)
		{
			ReorientGlobe::non_null_ptr_type ptr(*(new ReorientGlobe(globe_, globe_canvas_)));
			return ptr;
		}

		virtual
		void
		handle_left_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe)
		{
			reorient_globe_by_drag_update(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe, is_on_globe);
		}

		virtual
		void
		handle_left_release_after_drag(
				const GPlatesMaths::PointOnSphere &initial_pos_on_globe,
				const GPlatesMaths::PointOnSphere &oriented_initial_pos_on_globe,
				bool was_on_globe,
				const GPlatesMaths::PointOnSphere &current_pos_on_globe,
				bool is_on_globe)
		{
			reorient_globe_by_drag_release(initial_pos_on_globe,
					oriented_initial_pos_on_globe, was_on_globe,
					current_pos_on_globe, is_on_globe);
		}

	protected:
		// This constructor should not be public, because we don't want to allow
		// instantiation of this type on the stack.
		explicit
		ReorientGlobe(
				GPlatesGui::Globe &globe_,
				GPlatesQtWidgets::GlobeCanvas &globe_canvas_):
			CanvasTool(globe_, globe_canvas_)
		{  }

	private:
		// This constructor should never be defined, because we don't want/need to allow
		// copy-construction.
		ReorientGlobe(
				const ReorientGlobe &);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment.
		ReorientGlobe &
		operator=(
				const ReorientGlobe &);
	};
}

#endif  // GPLATES_CANVASTOOLS_REORIENTGLOBE_H