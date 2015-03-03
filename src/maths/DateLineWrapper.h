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

#ifndef GPLATES_MATHS_DATELINEWRAPPER_H
#define GPLATES_MATHS_DATELINEWRAPPER_H

#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/shared_ptr.hpp>

#include "LatLonPoint.h"
#include "PolygonOnSphere.h"
#include "PolylineOnSphere.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"
#include "utils/SmartNodeLinkedList.h"


namespace GPlatesMaths
{
	class BoundingSmallCircle;

	/**
	 * Clips polyline/polygon geometries to the dateline (at -180, or +180, degrees longitude) and
	 * wraps them to the opposite longitude so that they display correctly over a (-180,180)
	 * rectangular (lat/lon) projection.
	 *
	 * The motivation for this class is to remove horizontal lines across the display in ArcGIS
	 * for geometries that intersect the dateline.
	 *
	 * NOTE: This is a class instead of global functions in case certain things can be re-used
	 * such as vertex memory pools, etc.
	 *
	 * The algorithms and data structures used here are based on the following paper:
	 *
	 *   Greiner, G., Hormann, K., 1998.
	 *   Efficient clipping of arbitrary polygons.
	 *   Association for Computing Machinery—Transactions on Graphics 17 (2), 71–83.
	 *
	 * ...and, to a lesser extent,...
	 *
	 *   Liu, Y.K.,Wang,X.Q.,Bao,S.Z.,Gombosi, M.,Zalik, B.,2007.
	 *   An algorithm for polygon clipping, and for determining polygon intersections and unions.
	 *   Computers&Geosciences33,589–598.
	 */
	class DateLineWrapper :
			public GPlatesUtils::ReferenceCount<DateLineWrapper>
	{
	public:
		typedef GPlatesUtils::non_null_intrusive_ptr<DateLineWrapper> non_null_ptr_type;
		typedef GPlatesUtils::non_null_intrusive_ptr<const DateLineWrapper> non_null_ptr_to_const_type;


		//! Typedef for a sequence of lat/lon points.
		typedef std::vector<LatLonPoint> lat_lon_points_seq_type;

		//! Typedef for a polyline containing lat/lon points.
		typedef boost::shared_ptr<lat_lon_points_seq_type> lat_lon_polyline_type;

		/**
		 * Typedef for a polygon containing lat/lon points.
		 *
		 * NOTE: This mirrors @a PolygonOnSphere in that the start and end points are *not* the same.
		 * So you may need to explicitly close the polygon by appending the start point (eg, OGR library).
		 */
		typedef boost::shared_ptr<lat_lon_points_seq_type> lat_lon_polygon_type;


		/**
		 * Creates a @a DateLineWrapper object.
		 */
		static
		non_null_ptr_type
		create()
		{
			return non_null_ptr_type(new DateLineWrapper());
		}


		/**
		 * Clips the specified polyline-on-sphere to the dateline while converting from cartesian (xyz)
		 * coordinates to latitude/longitude coordinates.
		 *
		 * The clipped polylines are appended to @a output_polylines.
		 *
		 * The dateline is the great circle arc from north pole to south pole at longitude -180 degrees.
		 * The dateline splits the entire globe into the longitude range (-180,180).
		 * So longitudes greater than 180 wrap to -180 and longitudes less than -180 wrap to 180.
		 *
		 * NOTE: If @a input_polyline does *not* intersect the dateline then it is not clipped and
		 * hence only one polyline is output to @a output_polylines (it is just the unclipped polyline
		 * converted to lat/lon coordinates).
		 *
		 * NOTE: In some rare cases it's possible that the entire input polyline got swallowed by the dateline.
		 * This can happen if the original polyline is entirely *on* the dateline which is considered
		 * to be *outside* the dateline polygon (which covers the entire globe and 'effectively' excludes
		 * a very thin area of size epsilon around the dateline arc).
		 * In this situation the input polyline is added to @a output_polylines.
		 */
		void
		wrap_to_dateline(
				const PolylineOnSphere::non_null_ptr_to_const_type &input_polyline,
				std::vector<lat_lon_polyline_type> &output_polylines);


		/**
		 * Clips the specified polygon-on-sphere to the dateline while converting from cartesian (xyz)
		 * coordinates to latitude/longitude coordinates.
		 *
		 * The clipped polygons are appended to @a output_polygons.
		 *
		 * The dateline is the great circle arc from north pole to south pole at longitude -180 degrees.
		 * The dateline splits the entire globe into the longitude range (-180,180).
		 * So longitudes greater than 180 wrap to -180 and longitudes less than -180 wrap to 180.
		 *
		 * NOTE: If @a input_polygon does *not* intersect the dateline then it is not clipped and
		 * hence only one polygon is output to @a output_polygons (it is just the unclipped polygon
		 * converted to lat/lon coordinates).
		 *
		 * NOTE: In some rare cases it's possible that the entire input polygon got swallowed by the dateline.
		 * This can happen if the original polygon is entirely *on* the dateline which is considered
		 * to be *outside* the dateline polygon (which covers the entire globe and 'effectively' excludes
		 * a very thin area of size epsilon around the dateline arc).
		 * In this situation the input polygon is added to @a output_polygons.
		 */
		void
		wrap_to_dateline(
				const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon,
				std::vector<lat_lon_polygon_type> &output_polygons);

		//
		// The following early reject methods are not needed and only provided for convenience...
		//

		/**
		 * Returns true if the specified bounding small circle intersects the dateline arc.
		 *
		 * This can be used as a quick test to see if a geometry (bounded by the small circle)
		 * can *possibly* intersect the dateline.
		 *
		 * NOTE: A return value of true does *not* necessarily mean a geometry bounded by the
		 * small circle intersects the dateline.
		 * However a return value of false does mean a bounded geometry does *not* intersect the dateline.
		 */
		bool
		intersects_dateline(
				const BoundingSmallCircle &geometry_bounding_small_circle) const;

	private:
		/**
		 * Classification of a vertex based on its position relative to the dateline.
		 */
		enum VertexClassification
		{
			// In front of the plane containing the dateline arc.
			CLASSIFY_FRONT,

			// Behind the plane containing the dateline arc.
			CLASSIFY_BACK,

			// On the 'thick' (epsilon) plane containing the dateline arc, but off the dateline
			// arc itself (ie, roughly longitude of zero degrees).
			CLASSIFY_OFF_DATELINE_ARC_ON_PLANE,

			// On the 'thick' (epsilon) plane containing the dateline arc and on the dateline
			// arc itself (ie, roughly longitude of -/+ 1800 degrees).
			CLASSIFY_ON_DATELINE_ARC,

			// Within the 'thick' point (epsilon circle) at the north pole.
			// Basically within epsilon from the north pole.
			CLASSIFY_ON_NORTH_POLE,

			// Within the 'thick' point (epsilon circle) at the south pole.
			// Basically within epsilon from the south pole.
			CLASSIFY_ON_SOUTH_POLE
		};


		/**
		 * The type of intersection when intersecting a line segment of a geometry with the dateline.
		 */
		enum IntersectionType
		{
			INTERSECTED_DATELINE,
			INTERSECTED_NORTH_POLE,
			INTERSECTED_SOUTH_POLE
		};


		/**
		 * A vertex in the graph.
		 */
		struct Vertex
		{
			explicit
			Vertex(
					const LatLonPoint &point_,
					bool is_intersection_ = false,
					bool exits_other_polygon_ = false) :
				point(point_),
				intersection_neighbour(NULL),
				is_intersection(is_intersection_),
				exits_other_polygon(exits_other_polygon_),
				used_to_output_polygon(false)
			{  }


			LatLonPoint point;

			/**
			 * References the other polygon if this is an intersection vertex, otherwise NULL.
			 *
			 * Since the dateline is treated as a polygon it can reference the geometry (polygon) and vice versa.
			 *
			 * Note that for polylines this is always NULL since traversal of the dateline is not needed.
			 *
			 * NOTE: Due to cyclic dependencies this is a 'void *' instead of a 'vertex_list_type::Node *'.
			 */
			void/*vertex_list_type::Node*/ *intersection_neighbour;

			/**
			 * Is true if this vertex represents an intersection of geometry with the dateline.
			 *
			 * NOTE: This can be true when @a intersection_neighbour is NULL if geometry is a *polyline*.
			 * This is because polylines don't use, and hence don't initialise, @a intersection_neighbour.
			 */
			bool is_intersection;

			/**
			 * Is true if this vertex is an intersection that exits the other polygon.
			 *
			 * For a polygon geometry the other polygon is the dateline polygon.
			 * And for the dateline polygon the other polygon is the geometry polygon.
			 * Note that this is not used when the geometry is a polyline.
			 */
			bool exits_other_polygon;

			/**
			 * Is true if this vertex has been used to output a polygon.
			 *
			 * This is used to track which output polygons have been output.
			 */
			bool used_to_output_polygon;
		};

		//! Typedef for a double-linked list of vertices.
		typedef GPlatesUtils::SmartNodeLinkedList<Vertex> vertex_list_type;

		//! Typedef for a pool allocator of vertex list nodes.
		typedef boost::object_pool<vertex_list_type::Node> vertex_node_pool_type;


		/**
		 * A graph of a geometry potentially intersecting the dateline.
		 *
		 * The dateline is treated as an infinitesimally thin polygon so that traditional
		 * polygon-polygon clipping can be performed (when the geometry is a polygon and not a polyline).
		 */
		class IntersectionGraph :
				boost::noncopyable
		{
		public:
			explicit
			IntersectionGraph(
					// Is this graph used to clip a polygon ?
					bool is_polygon_graph_);

			/**
			 * Traverses geometry vertices (and intersection vertices) in graph and generates polylines.
			 *
			 * Returns false if no output polylines were generated (if input polyline entirely on dateline).
			 */
			bool
			generate_polylines(
					std::vector<lat_lon_polyline_type> &lat_lon_polylines);

			/**
			 * Traverses geometry/dateline vertices (and intersection vertices) in graph and generates polygons.
			 *
			 * Returns false if no output polygons were generated (if input polygon entirely on dateline *or*
			 * there were no intersections with the dateline).
			 */
			bool
			generate_polygons(
					std::vector<lat_lon_polygon_type> &lat_lon_polygons,
					const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon);

			//! Adds a regular vertex that is *not* the result of intersecting or touching the dateline.
			void
			add_vertex(
						const PointOnSphere &point);

			//! Adds an intersection of geometry with the dateline on the front side of dateline.
			void
			add_intersection_vertex_on_front_dateline(
					const PointOnSphere &point,
					bool exiting_dateline_polygon);

			//! Adds an intersection of geometry with the dateline on the back side of dateline.
			void
			add_intersection_vertex_on_back_dateline(
					const PointOnSphere &point,
					bool exiting_dateline_polygon);

			/**
			 * Adds an intersection of geometry with the north pole.
			 */
			void
			add_intersection_vertex_on_north_pole(
					const PointOnSphere &point,
					bool exiting_dateline_polygon);

			/**
			 * Adds an intersection of geometry with the south pole.
			 */
			void
			add_intersection_vertex_on_south_pole(
					const PointOnSphere &point,
					bool exiting_dateline_polygon);

			/**
			 * Record the fact that the geometry intersected the north pole.
			 */
			void
			intersected_north_pole()
			{
				d_geometry_intersected_north_pole = true;
			}

			/**
			 * Record the fact that the geometry intersected the south pole.
			 */
			void
			intersected_south_pole()
			{
				d_geometry_intersected_south_pole = true;
			}

		private:
			//! Allocates vertex list nodes - all nodes are freed when 'this' graph is destroyed.
			vertex_node_pool_type d_vertex_node_pool;

			//! The geometry vertices (and intersection point copies).
			vertex_list_type d_geometry_vertices;

			/**
			 * Contains the four lat/lon corner dateline vertices (and any intersection point copies).
			 *
			 * NOTE: This list is empty if this graph is *not* a polygon graph (ie, if it's a polyline graph).
			 */
			vertex_list_type d_dateline_vertices;

			/*
			 * The four corner vertices represent the corners of the rectangular projection:
			 *   (-90,  180)
			 *   ( 90,  180)
			 *   ( 90, -180)
			 *   (-90, -180)
			 *
			 * The order is a clockwise order when looking at the dateline arc from above the globe.
			 *
			 *   NF -> NB
			 *   /\    |
			 *   |     \/
			 *   SF <- SB
			 *
			 *            dateline
			 *               |
			 *   <---   +180 | -180   --->
			 *               |
			 *
			 * NOTE: These are NULL if this graph is *not* a polygon graph (ie, if it's a polyline graph).
			 */
			vertex_list_type::Node *d_dateline_corner_south_front;
			vertex_list_type::Node *d_dateline_corner_north_front;
			vertex_list_type::Node *d_dateline_corner_north_back;
			vertex_list_type::Node *d_dateline_corner_south_back;

			//! If generating a graph for a polygon then @a dateline_vertices will be created at intersections.
			bool d_is_polygon_graph;

			//! Is true if geometry intersected or touched (within epsilon) the north pole.
			bool d_geometry_intersected_north_pole;
			//! Is true if geometry intersected or touched (within epsilon) the south pole.
			bool d_geometry_intersected_south_pole;

			//! The sentinel node in the vertex lists constructs an unused dummy vertex - the value doesn't matter.
			static const Vertex LISTS_SENTINEL;


			void
			link_intersection_vertices(
						vertex_list_type::Node *intersection_vertex_node1,
						vertex_list_type::Node *intersection_vertex_node2)
			{
				intersection_vertex_node1->element().intersection_neighbour = intersection_vertex_node2;
				intersection_vertex_node2->element().intersection_neighbour = intersection_vertex_node1;
			}

			void
			generate_entry_exit_flags_for_dateline_polygon(
					const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon);

			void
			generate_entry_exit_flags_for_dateline_polygon(
					const vertex_list_type::iterator initial_dateline_vertex_iter,
					const bool initial_dateline_vertex_is_inside_geometry_polygon);

			void
			output_intersecting_polygons(
					std::vector<lat_lon_polygon_type> &lat_lon_polygons);
		};


		//! Constructor.
		DateLineWrapper()
		{  }

		bool
		possibly_intersects_dateline(
				const PolylineOnSphere::non_null_ptr_to_const_type &polyline) const;

		bool
		possibly_intersects_dateline(
				const PolygonOnSphere::non_null_ptr_to_const_type &polygon) const;

		//! Generates a graph from the geometry and the dateline and any intersections.
		template <typename LineSegmentForwardIter>
		void
		generate_intersection_graph(
				IntersectionGraph &graph,
				LineSegmentForwardIter const line_segments_begin,
				LineSegmentForwardIter const line_segments_end,
				bool is_polygon);

		//! Adds a line segment to the intersection graph based on its vertex classifications.
		void
		add_line_segment_to_intersection_graph(
				IntersectionGraph &graph,
				const GreatCircleArc &line_segment,
				VertexClassification line_segment_start_vertex_classification,
				VertexClassification line_segment_end_vertex_classification);

		/**
		 * Returns intersection of line segment (line_segment_start_point, line_segment_end_point) with dateline, if any.
		 *
		 * If an intersection was found then type of intersection is returned in @a intersection_type.
		 *
		 * NOTE: The line segment endpoints must be on opposite sides of the 'thick' dateline plane,
		 * otherwise the result is not numerically robust.
		 */
		boost::optional<PointOnSphere>
		intersect_line_segment(
				IntersectionType &intersection_type,
				const GreatCircleArc &line_segment,
				bool line_segment_start_point_in_dateline_front_half_space,
				IntersectionGraph &graph) const;

		//! Calculates intersection point of the specified line segment and the dateline.
		boost::optional<GPlatesMaths::PointOnSphere>
		calculate_intersection(
				const GreatCircleArc &line_segment) const;

		//! Classifies the specified point relative to the dateline.
		VertexClassification
		classify_vertex(
				const UnitVector3D &vertex,
				IntersectionGraph &graph) const;

		//! Output the un-intersected input polyline.
		void
		output_input_polyline(
				const PolylineOnSphere::non_null_ptr_to_const_type &input_polyline,
				std::vector<lat_lon_polyline_type> &output_polylines,
				bool on_dateline_arc);

		//! Output the un-intersected input polygon.
		void
		output_input_polygon(
				const PolygonOnSphere::non_null_ptr_to_const_type &input_polygon,
				std::vector<lat_lon_polygon_type> &output_polygons,
				bool on_dateline_arc);
	};
}

#endif // GPLATES_MATHS_DATELINEWRAPPER_H
