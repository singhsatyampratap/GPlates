/* $Id$ */

/**
 * \file 
 * Interface for visiting derived @a RenderedGeometryImpl classes.
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

#ifndef GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYVISITOR_H
#define GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYVISITOR_H

namespace GPlatesViewOperations
{
	class RenderedPointOnSphere;
	class RenderedMultiPointOnSphere;
	class RenderedPolylineOnSphere;
	class RenderedPolygonOnSphere;

	/**
	 * Interface for visiting a derived @a RenderedGeometryImpl object.
	 *
	 * This visits const objects only because @a RenderedGeometry objects
	 * are immutable so we should respect this when accessing their
	 * implementation types.
	 */
	class ConstRenderedGeometryVisitor
	{
	public:
		virtual
		~ConstRenderedGeometryVisitor()
		{  }

		virtual
		void
		visit_rendered_point_on_sphere(
				const GPlatesViewOperations::RenderedPointOnSphere &)
		{  }

		virtual
		void
		visit_rendered_multi_point_on_sphere(
				const RenderedMultiPointOnSphere &)
		{  }

		virtual
		void
		visit_rendered_polyline_on_sphere(
				const RenderedPolylineOnSphere &)
		{  }

		virtual
		void
		visit_rendered_polygon_on_sphere(
				const RenderedPolygonOnSphere &)
		{  }
	};
}

#endif // GPLATES_VIEWOPERATIONS_RENDEREDGEOMETRYVISITOR_H