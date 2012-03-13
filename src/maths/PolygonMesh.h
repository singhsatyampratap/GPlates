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

#ifndef GPLATES_MATHS_POLYGONMESH_H
#define GPLATES_MATHS_POLYGONMESH_H

#include <boost/optional.hpp>

#include "MultiPointOnSphere.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"
#include "UnitVector3D.h"

#include "utils/ReferenceCount.h"


namespace GPlatesMaths
{
	/**
	 * A triangular mesh of the interior region of a polygon.
	 *
	 * Can also be generated from a polyline by closing the gap between the first and last vertices.
	 *
	 * Can also be generated from a multipoint by considering the order of points to form the
	 * concave circumference of a polygon.
	 *
	 * TODO: Need to handle self-intersections (ie, generate mesh in presense of self-intersections).
	 * UPDATE: Currently polygon stenciling of a polygon fan mesh (ie, a mesh not fully contained
	 * within the interior of the polygon) is used exclusively for rendering filled polygons and
	 * used for self-intersecting polygons when reconstructing rasters - polygon stenciling is a
	 * raster based technique that masks away the regions of the fan mesh outside the polygon - so
	 * this technique is only useful for raster based applications.
	 */
	class PolygonMesh :
			public GPlatesUtils::ReferenceCount<PolygonMesh>
	{
	public:
		//! A convenience typedef for a shared pointer to a non-const @a PolygonMesh.
		typedef GPlatesUtils::non_null_intrusive_ptr<PolygonMesh> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a PolygonMesh.
		typedef GPlatesUtils::non_null_intrusive_ptr<const PolygonMesh> non_null_ptr_to_const_type;


		/**
		 * A mesh triangle.
		 *
		 * Contains three vertex indices into the vertex array returned by 
		 */
		class Triangle
		{
		public:
			/**
			 * Returns the index into the array of mesh vertices.
			 *
			 * The returned index is used to look up a vertex in the array returned
			 * by "PolygonMesh::get_vertices()".
			 *
			 * @param triangle_vertex_index is either 0, 1 or 2.
			 */
			unsigned int
			get_mesh_vertex_index(
					unsigned int triangle_vertex_index) const
			{
				return d_vertex_indices[triangle_vertex_index];
			}

		private:
			unsigned int d_vertex_indices[3];

			// NOTE: Constructor does not initialise.
			Triangle()
			{  }

			friend class PolygonMesh;
		};


		//! A mesh vertex.
		class Vertex
		{
		public:
			//! Returns the vertex.
			const UnitVector3D &
			get_vertex() const
			{
				return d_vertex;
			}

		private:
			UnitVector3D d_vertex;

			explicit
			Vertex(
					const UnitVector3D &vertex) :
				d_vertex(vertex)
			{  }

			friend class PolygonMesh;
		};


		/**
		 * Creates a @a PolygonMesh object from a @a PolygonOnSphere.
		 *
		 * Returns boost::none if failed to generate the mesh.
		 * Such as a self-intersecting polygon or polygon too large to project onto a 2D plane.
		 * TODO: Create polygon mesh under any situation (except too few unique vertices).
		 */
		static
		boost::optional<non_null_ptr_to_const_type>
		create(
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon);


		/**
		 * Creates a @a PolygonMesh object from a @a PolylineOnSphere.
		 *
		 * A polygon is formed from the polyline by joining the first and last vertices.
		 *
		 * Returns boost::none if failed to generate the mesh.
		 * Such as a self-intersecting polygon or polygon too large to project onto a 2D plane.
		 * TODO: Create polygon mesh under any situation (except too few unique vertices).
		 */
		static
		boost::optional<non_null_ptr_to_const_type>
		create(
				const PolylineOnSphere::non_null_ptr_to_const_type &polyline);


		/**
		 * Creates a @a PolygonMesh object from a @a MultiPointOnSphere.
		 *
		 * A polygon is formed from the multipoint by treating the order of points in the multipoint
		 * as the vertices of a polygon.
		 *
		 * Returns boost::none if failed to generate the mesh.
		 * Such as a self-intersecting polygon or polygon too large to project onto a 2D plane.
		 * TODO: Create polygon mesh under any situation (except too few unique vertices).
		 */
		static
		boost::optional<non_null_ptr_to_const_type>
		create(
				const MultiPointOnSphere::non_null_ptr_to_const_type &multi_point);


		/**
		 * Creates a @a PolygonMesh object from a @a GeometryOnSphere.
		 *
		 * Returns boost::none if failed to generate the mesh.
		 * Such as a self-intersecting polygon or polygon too large to project onto a 2D plane.
		 * TODO: Create polygon mesh under any situation (except too few unique vertices).
		 *
		 * Note that @a PointOnSphere is the only @a GeometryOnSphere derivation not handled.
		 * Because can't have a polygon mesh with only a single point.
		 */
		static
		boost::optional<non_null_ptr_to_const_type>
		create(
				const GeometryOnSphere::non_null_ptr_to_const_type &geometry_on_sphere);


		/**
		 * Returns the sequence of triangles that form the polygon mesh.
		 *
		 * The vertex indices in each triangles index into the vertex array returned by @a get_vertices.
		 */
		const std::vector<Triangle> &
		get_triangles() const
		{
			return d_triangles;
		}


		/**
		 * Returns the sequence of vertices indexed by the triangles in the polygon mesh.
		 */
		const std::vector<Vertex> &
		get_vertices() const
		{
			return d_vertices;
		}
		
	private:
		/**
		 * The mesh triangles.
		 */
		std::vector<Triangle> d_triangles;

		/**
		 * The mesh vertices.
		 */
		std::vector<Vertex> d_vertices;


		//! Default constructor starts off with no triangles or vertices.
		PolygonMesh()
		{  }


		/**
		 * Creates, and initialises, this polygon mesh using the specified range
		 * of points as the polygon boundary.
		 */
		template <typename PointOnSphereForwardIter>
		bool
		initialise(
				PointOnSphereForwardIter polygon_points_begin,
				PointOnSphereForwardIter polygon_points_end);
	};
}

#endif // GPLATES_MATHS_POLYGONMESH_H
