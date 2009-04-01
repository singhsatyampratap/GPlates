/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006 The University of Sydney, Australia
 *  (under the name "PolyLineOnSphere.h")
 * Copyright (C) 2006, 2007, 2008 The University of Sydney, Australia
 *  (under the name "PolylineOnSphere.h")
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

#ifndef GPLATES_MATHS_POLYLINEONSPHERE_H
#define GPLATES_MATHS_POLYLINEONSPHERE_H

#include <vector>
#include <iterator>  // std::iterator, std::bidirectional_iterator_tag, std::distance
#include <algorithm>  // std::swap
#include <utility>  // std::pair

#include "GeometryOnSphere.h"
#include "GreatCircleArc.h"
#include "global/PreconditionViolationError.h"


namespace GPlatesMaths
{
	/** 
	 * Represents a polyline on the surface of a sphere. 
	 *
	 * Internally, this is stored as a sequence of GreatCircleArc.  You
	 * can iterate over this sequence of GreatCircleArc in the usual manner
	 * using the const_iterators returned by the functions @a begin and
	 * @a end.
	 *
	 * You can also iterate over the @em vertices of the polyline using the
	 * vertex_const_iterators returned by the functions @a vertex_begin and
	 * @a vertex_end.  For instance, to copy all the vertices of a polyline
	 * into a list of PointOnSphere, you would use the code snippet:
	 * @code
	 * std::list< PointOnSphere > the_list(polyline.vertex_begin(),
	 *                                     polyline.vertex_end());
	 * @endcode
	 *
	 * You can create a polyline by invoking the static member function
	 * @a PolylineOnSphere::create_on_heap, passing it a sequential STL
	 * container (list, vector, ...) of PointOnSphere to define the
	 * vertices of the polyline.  The sequence of points must contain at
	 * least two distinct elements, enabling the creation of a polyline
	 * composed of at least one well-defined segment.  The requirements
	 * upon the sequence of points are described in greater detail in the
	 * comment for the static member function
	 * @a PolylineOnSphere::evaluate_construction_parameter_validity.
	 *
	 * Say you have a sequence of PointOnSphere: [A, B, C, D].  If you pass
	 * this sequence to the @a PolylineOnSphere::create_on_heap function, it
	 * will create a polyline composed of 3 segments: A->B, B->C, C->D.  If
	 * you subsequently iterate through the vertices of this polyline, you
	 * will get the same sequence of points back again: A, B, C, D.
	 *
	 * Note that PolylineOnSphere does have mutators (non-const member functions
	 * which enable the modification of the class internals), in particular
	 * the copy-assignment operator.
	 */
	class PolylineOnSphere:
			public GeometryOnSphere
	{
		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<PolylineOnSphere,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 * 
		 * Note that this typedef is indeed meant to be private.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<PolylineOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;

	public:

		/**
		 * A convenience typedef for
		 * GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere,
		 * GPlatesUtils::NullIntrusivePointerHandler>.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const PolylineOnSphere,
				GPlatesUtils::NullIntrusivePointerHandler>
				non_null_ptr_to_const_type;


		/**
		 * The type of the sequence of great circle arcs.
		 *
		 * Implementation detail:  We are using 'std::vector' as the
		 * sequence type (rather than, say, 'std::list') to provide a
		 * speed-up in memory-allocation (we use 'std::vector::reserve'
		 * at creation time to avoid expensive reallocations as arcs
		 * are appended one-by-one; after that, because the contents of
		 * the sequence are never altered, the size of the vector will
		 * never change), a speed-up in iteration (for what it's worth,
		 * a pointer-increment rather than a 'node = node->next'-style
		 * operation) and a decrease (hopefully) in memory-usage (by
		 * avoiding a whole bunch of unnecessary links).  (We should,
		 * however, be able to get away without relying upon the
		 * "random-access"ness of vector iterators; forward iterators
		 * should be enough.)
		 */
		typedef std::vector<GreatCircleArc> seq_type;


		/**
		 * The type used to const_iterate over the sequence of arcs.
		 */
		typedef seq_type::const_iterator const_iterator;


		/**
		 * The type used to describe collection sizes.
		 */
		typedef seq_type::size_type size_type;


		/**
		 * This class enables const_iteration over the vertices of a
		 * PolylineOnSphere.
		 *
		 * An instance of this class @em actually iterates over the
		 * sequence of GreatCircleArc by which a PolylineOnSphere is
		 * implemented, but it pretends it's iterating over a sequence
		 * of PointOnSphere by additionally keeping track of whether
		 * it's pointing at the "start-point" or "end-point" of the
		 * current GreatCircleArc.
		 *
		 * It is assumed that the sequence of GreatCircleArc over
		 * which this iterator is iterating will always contain at
		 * least one element (and thus, at least two vertices).  This
		 * assumption should be fulfilled by the PolylineOnSphere
		 * invariant.
		 */
		class VertexConstIterator:
				public std::iterator<std::bidirectional_iterator_tag, PointOnSphere>
		{

			enum StartOrEnd {
				START,
				END
			};

			typedef PolylineOnSphere::const_iterator gca_const_iterator;

		public:

			/**
			 * Create the "begin" vertex iterator for @a poly.
			 *
			 * Note that it's intentional that the instance returned is non-const: If
			 * the instance were const, it would not be possible to write an expression
			 * like '++(polyline.vertex_begin())' to access the second vertex of the
			 * polyline.
			 */
			static
			VertexConstIterator
			create_begin(
					const PolylineOnSphere &poly);


			/**
			 * Create the "end" vertex iterator for @a poly.
			 *
			 * Note that it's intentional that the instance returned is non-const: If
			 * the instance were const, it would not be possible to write an expression
			 * like '--(polyline.vertex_end())' to access the last vertex of the
			 * polyline.
			 */
			static
			VertexConstIterator
			create_end(
					const PolylineOnSphere &poly);


			/**
			 * Default-construct a vertex iterator.
			 *
			 * A default-constructed iterator will be
			 * uninitialised.  (I don't @em like providing a
			 * constructor which leaves an object in an
			 * uninitialised state, but the presence of a default
			 * constructor is mandated by the
			 * bidirectional_iterator interface.)
			 *
			 * If you attempt to dereference an uninitialised
			 * iterator or access the members of a PointOnSphere
			 * through an uninitialised iterator you will get a
			 * GPlatesGlobal::UninitialisedIteratorException thrown
			 * back at you.  (Hey, it's better than a segfault...)
			 *
			 * The following operations are OK for an uninitialised
			 * iterator:
			 *  - comparison for equality to another iterator;
			 *  - comparison for inequality to another iterator;
			 *  - being assigned-to by another iterator.
			 *
			 * The following operations are no-ops for an
			 * uninitialised iterator:
			 *  - increment;
			 *  - decrement.
			 */
			VertexConstIterator():
				d_poly_ptr(NULL), 
				d_curr_gca(), 
				d_gca_start_or_end(END)
			{  }


			/**
			 * Copy-construct a vertex iterator.
			 */
			VertexConstIterator(
					const VertexConstIterator &other):
				d_poly_ptr(other.d_poly_ptr),
				d_curr_gca(other.d_curr_gca),
				d_gca_start_or_end(other.d_gca_start_or_end)
			{  }


			/**
			 * Return the gca_const_iterator which points to the
			 * current GreatCircleArc.
			 */
			gca_const_iterator
			curr_gca() const
			{
				return d_curr_gca;
			}


			/**
			 * Return whether this iterator is pointing at the
			 * "start-point" or "end-point" of the current
			 * GreatCircleArc.
			 */
			StartOrEnd
			gca_start_or_end() const
			{
				return d_gca_start_or_end;
			}


			/**
			 * Assign @a other to this.
			 */
			VertexConstIterator &
			operator=(
					const VertexConstIterator &other)
			{
				d_poly_ptr = other.d_poly_ptr;
				d_curr_gca = other.d_curr_gca;
				d_gca_start_or_end = other.d_gca_start_or_end;

				return *this;
			}


			/**
			 * Dereference this iterator to obtain the
			 * currently-pointed-at PointOnSphere.
			 */
			const PointOnSphere &
			operator*() const
			{
				return current_point();
			}


			/**
			 * Access a member of the PointOnSphere which is
			 * currently being pointed-at by this iterator.
			 */
			const PointOnSphere *
			operator->() const
			{
				return &(current_point());
			}


			/**
			 * Pre-increment this iterator.
			 */
			VertexConstIterator &
			operator++()
			{
				increment();
				return *this;
			}


			/**
			 * Post-increment this iterator.
			 */
			const VertexConstIterator
			operator++(int)
			{
				VertexConstIterator old = *this;
				increment();
				return old;
			}


			/**
			 * Pre-decrement this iterator.
			 */
			VertexConstIterator &
			operator--()
			{
				decrement();
				return *this;
			}


			/**
			 * Post-decrement this iterator.
			 */
			const VertexConstIterator
			operator--(int)
			{
				VertexConstIterator old = *this;
				decrement();
				return old;
			}

		private:

			/**
			 * Construct a VertexConstIterator instance to iterate over the vertices of
			 * @a poly.
			 *
			 * The current position of the iterator is described by @a curr_gca_ and
			 * @a gca_start_or_end_.  Note that not all combinations of @a curr_gca_
			 * and @a gca_start_or_end_ are valid.
			 *
			 * This constructor should only be invoked by the @a create_begin and
			 * @a create_end static member functions.
			 */
			VertexConstIterator(
					const PolylineOnSphere &poly,
					gca_const_iterator curr_gca_,
					StartOrEnd gca_start_or_end_) :
				d_poly_ptr(&poly),
				d_curr_gca(curr_gca_),
				d_gca_start_or_end(gca_start_or_end_)
			{  }


			/**
			 * This function performs the magic which is used to
			 * obtain the currently-pointed-at PointOnSphere.
			 */
			const PointOnSphere &
			current_point() const;


			/**
			 * This function performs the magic which is used to
			 * increment this iterator.
			 *
			 * If this iterator is uninitialised (ie, it was either
			 * default-constructed or assigned-to by an iterator
			 * which was default-constructed) this function will be
			 * a no-op.
			 */
			void
			increment();


			/**
			 * This function performs the magic which is used to
			 * decrement this iterator.
			 *
			 * If this iterator is uninitialised (ie, it was either
			 * default-constructed or assigned-to by an iterator
			 * which was default-constructed) this function will be
			 * a no-op.
			 */
			void
			decrement();


			/**
			 * This is the PolylineOnSphere instance which is being
			 * traversed by this iterator.
			 *
			 * This pointer will be NULL in an uninitialised
			 * (default-constructed) iterator.
			 */
			const PolylineOnSphere *d_poly_ptr;


			/**
			 * This points to the current GreatCircleArc in the
			 * PolylineOnSphere.
			 */
			gca_const_iterator d_curr_gca;


			/**
			 * This keeps track of whether this iterator is
			 * pointing at the "start-point" or "end-point" of the
			 * current GreatCircleArc.
			 */
			StartOrEnd d_gca_start_or_end;

		};


		/**
		 * The type used to const_iterate over the vertices.
		 */
		typedef VertexConstIterator vertex_const_iterator;


		/**
		 * The possible return values from the construction-parameter
		 * validation functions
		 * @a evaluate_construction_parameter_validity and
		 * @a evaluate_segment_endpoint_validity.
		 */
		enum ConstructionParameterValidity
		{
			VALID,
			INVALID_INSUFFICIENT_DISTINCT_POINTS,
			INVALID_ANTIPODAL_SEGMENT_ENDPOINTS
		};


		/**
		 * Evaluate the validity of the construction-parameters.
		 *
		 * What this actually means in plain(er) English is that you
		 * can use this function to check whether you would be able to
		 * construct a polyline instance from a given set of parameters
		 * (ie, your collection of points in the range @a begin / @a end).
		 *
		 * If you pass this function what turns out to be invalid
		 * construction-parameters, it will politely return an error
		 * diagnostic.  If you were to pass these same invalid
		 * parameters to the creation functions down below, you would
		 * get an exception thrown back at you.
		 *
		 * It's not terribly difficult to obtain a collection which
		 * qualifias as valid parameters (no antipodal adjacent points;
		 * at least two distinct points in the collection -- nothing
		 * particularly unreasonable) but the creation functions are
		 * fairly unsympathetic if your parameters @em do turn out to
		 * be invalid.
		 *
		 * @a invalid_points is a return-parameter; if the
		 * construction-parameters are found to be invalid due to
		 * antipodal adjacent points, the value of this return-parameter
		 * will be set to the pair of iterator of type ForwardIter which
		 * point to the guilty points.  If no adjacent points are found
		 * to be antipodal, this parameter will not be modified.
		 */
		template<typename ForwardIter>
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
				ForwardIter begin,
				ForwardIter end,
				std::pair<ForwardIter, ForwardIter> &invalid_points);

		/**
		 * Evaluate the validity of the construction-parameters.
		 *
		 * What this actually means in plain(er) English is that you
		 * can use this function to check whether you would be able to
		 * construct a polyline instance from a given set of parameters
		 * (ie, your collection of points in @a coll).
		 *
		 * If you pass this function what turns out to be invalid
		 * construction-parameters, it will politely return an error
		 * diagnostic.  If you were to pass these same invalid
		 * parameters to the creation functions down below, you would
		 * get an exception thrown back at you.
		 *
		 * It's not terribly difficult to obtain a collection which
		 * qualifias as valid parameters (no antipodal adjacent points;
		 * at least two distinct points in the collection -- nothing
		 * particularly unreasonable) but the creation functions are
		 * fairly unsympathetic if your parameters @em do turn out to
		 * be invalid.
		 *
		 * @a coll should be a sequential STL container (list, vector,
		 * ...) of PointOnSphere.
		 *
		 * @a invalid_points is a return-parameter; if the
		 * construction-parameters are found to be invalid due to
		 * antipodal adjacent points, the value of this return-parameter
		 * will be set to the pair of const_iterators of @a coll which
		 * point to the guilty points.  If no adjacent points are found
		 * to be antipodal, this parameter will not be modified.
		 */
		template<typename C>
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
				const C &coll,
				std::pair<typename C::const_iterator, typename C::const_iterator> &
						invalid_points)
		{
			return evaluate_construction_parameter_validity(
					coll.begin(), coll.end(), invalid_points);
		}


		/**
		 * Evaluate the validity of the points @a p1 and @a p2 for use
		 * in the creation of a polyline line-segment.
		 *
		 * You won't ever @em need to call this function
		 * (@a evaluate_construction_parameter_validity will do all the
		 * calling for you), but it's here in case you ever, you know,
		 * @em want to...
		 */
		static
		ConstructionParameterValidity
		evaluate_segment_endpoint_validity(
				const PointOnSphere &p1,
				const PointOnSphere &p2);


		/**
		 * Create a new PolylineOnSphere instance on the heap from the sequence of points
		 * delimited by the forward iterators @a begin and @a end and return
		 * an intrusive_ptr which points to the newly-created instance.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template<typename ForwardIter>
		static
		const non_null_ptr_to_const_type
		create_on_heap(
				ForwardIter begin, ForwardIter end);

		/**
		 * Create a new PolylineOnSphere instance on the heap from the sequence of points
		 * @a coll, and return an intrusive_ptr which points to the newly-created instance.
		 *
		 * @a coll should be a sequential STL container (list, vector, ...) of
		 * PointOnSphere.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template<typename C>
		static
		const non_null_ptr_to_const_type
		create_on_heap(
				const C &coll)
		{
			return create_on_heap(coll.begin(), coll.end());
		}


		/**
		 * Clone this PolylineOnSphere instance, to create a duplicate instance on the
		 * heap.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		const GeometryOnSphere::non_null_ptr_to_const_type
		clone_as_geometry() const
		{
			GeometryOnSphere::non_null_ptr_to_const_type dup(
					new PolylineOnSphere(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}


		/**
		 * Clone this PolylineOnSphere instance, to create a duplicate instance on the
		 * heap.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		const non_null_ptr_to_const_type
		clone_as_polyline() const
		{
			non_null_ptr_to_const_type dup(
					new PolylineOnSphere(*this),
					GPlatesUtils::NullIntrusivePointerHandler());
			return dup;
		}


		/**
		 * Get a non-null pointer to a const PolylineOnSphere which points to this instance
		 * (or a clone of this instance).
		 *
		 * (Since geometries are treated as immutable literals in GPlates, a geometry can
		 * never be modified through a pointer, so there is no reason why it would be
		 * inappropriate to return a pointer to a clone of this instance rather than a
		 * pointer to this instance.)
		 *
		 * This function will behave correctly regardless of whether this instance is on
		 * the stack or the heap.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer() const;


		virtual
		ProximityHitDetail::maybe_null_ptr_type
		test_proximity(
				const ProximityCriteria &criteria) const;


		/**
		 * Accept a ConstGeometryOnSphereVisitor instance.
		 *
		 * See the Visitor pattern (p.331) in Gamma95 for information on the purpose of
		 * this function.
		 */
		virtual
		void
		accept_visitor(
				ConstGeometryOnSphereVisitor &visitor) const;


		/**
		 * Copy-assign the value of @a other to this.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 *
		 * This copy-assignment operator should act exactly the same as the default
		 * (auto-generated) copy-assignment operator would, except that it should not
		 * assign the ref-count of @a other to this.
		 */
		PolylineOnSphere &
		operator=(
				const PolylineOnSphere &other)
		{
			// Use the copy+swap idiom to enable strong exception safety.
			PolylineOnSphere dup(other);
			this->swap(dup);
			return *this;
		}


		/**
		 * Return the "begin" const_iterator to iterate over the
		 * sequence of GreatCircleArc which defines this polyline.
		 */
		const_iterator
		begin() const
		{
			return d_seq.begin();
		}


		/**
		 * Return the "end" const_iterator to iterate over the
		 * sequence of GreatCircleArc which defines this polyline.
		 */
		const_iterator
		end() const
		{
			return d_seq.end();
		}


		/**
		 * Return the number of segments in this polyline.
		 */
		size_type
		number_of_segments() const
		{
			return d_seq.size();
		}


		/**
		 * Return the "begin" const_iterator to iterate over the vertices of this polyline.
		 *
		 * Note that it's intentional that the instance returned is non-const: If the
		 * instance were const, it would not be possible to write an expression like
		 * '++(polyline.vertex_begin())' to access the second vertex of the polyline.
		 */
		vertex_const_iterator
		vertex_begin() const
		{
			return vertex_const_iterator::create_begin(*this);
		}


		/**
		 * Return the "end" const_iterator to iterate over the vertices of this polyline.
		 *
		 * Note that it's intentional that the instance returned is non-const: If the
		 * instance were const, it would not be possible to write an expression like
		 * '--(polyline.vertex_begin())' to access the last vertex of the polyline.
		 */
		vertex_const_iterator
		vertex_end() const
		{
			return vertex_const_iterator::create_end(*this);
		}


		/**
		 * Return the number of vertices in this polyline.
		 */
		size_type
		number_of_vertices() const
		{
			return d_seq.size() + 1;
		}


		/**
		 * Return the start-point of this polyline.
		 */
		const PointOnSphere &
		start_point() const
		{
			const GreatCircleArc &first_gca = *(begin());
			return first_gca.start_point();
		}


		/**
		 * Return the end-point of this polyline.
		 */
		const PointOnSphere &
		end_point() const
		{
			const GreatCircleArc &last_gca = *(--(end()));
			return last_gca.end_point();
		}


		/**
		 * Swap the contents of this polyline with @a other.
		 *
		 * This function does not throw.
		 */
		void
		swap(
				PolylineOnSphere &other)
		{
			// Obviously, we should not swap the ref-counts of the instances.
			d_seq.swap(other.d_seq);
		}


		/**
		 * Evaluate whether @a test_point is "close" to this polyline.
		 *
		 * The measure of what is "close" is provided by
		 * @a closeness_inclusion_threshold.
		 *
		 * If @a test_point is "close", the function will calculate
		 * exactly @em how close, and store that value in @a closeness.
		 *
		 * The value of @a latitude_exclusion_threshold should be equal
		 * to \f$\sqrt{1 - {t_c}^2}\f$ (where \f$t_c\f$ is the
		 * closeness inclusion threshold).  This parameter is designed
		 * to enable a quick elimination of "no-hopers" (test-points
		 * which can easily be determined to have no chance of being
		 * "close"), leaving only plausible test-points to proceed to
		 * the more expensive proximity tests.  If you imagine a
		 * line-segment of this polyline as an arc along the equator,
		 * then there will be a threshold latitude above and below the
		 * equator, beyond which there is no chance of a test-point
		 * being "close" to that segment.
		 *
		 * For more information, read the comment before
		 * @a GPlatesGui::ProximityTests::find_close_rfgs.
		 */
		bool
		is_close_to(
				const PointOnSphere &test_point,
				const real_t &closeness_inclusion_threshold,
				const real_t &latitude_exclusion_threshold,
				real_t &closeness) const;

	private:

		/**
		 * Create an empty PolylineOnSphere instance.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of a polyline without any vertices.
		 *
		 * This constructor should never be invoked directly by client code; only through
		 * the static 'create_on_heap' function.
		 *
		 * This constructor should act exactly the same as the default (auto-generated)
		 * default-constructor would, except that it should initialise the ref-count to
		 * zero.
		 */
		PolylineOnSphere():
			GeometryOnSphere()
		{  }


		/**
		 * Create a copy-constructed PolylineOnSphere instance.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 *
		 * This constructor should never be invoked directly by client code; only through
		 * the 'clone_as_geometry' or 'clone_as_polyline' function.
		 *
		 * This constructor should act exactly the same as the default (auto-generated)
		 * copy-constructor would, except that it should initialise the ref-count to zero.
		 */
		PolylineOnSphere(
				const PolylineOnSphere &other):
			GeometryOnSphere(),
			d_seq(other.d_seq)
		{  }


		/**
		 * Generate a sequence of polyline segments from the sequence
		 * of points in the range @a begin / @a end, using the points to
		 * define the endpoints and vertices of the segments, then swap this
		 * new sequence of segments into the polyline @a poly, discarding
		 * any sequence of segments which may have been there before.
		 *
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		template<typename ForwardIter>
		static
		void
		generate_segments_and_swap(
				PolylineOnSphere &poly,
				ForwardIter begin,
				ForwardIter end);

		/**
		 * Attempt to create a line-segment defined by the points @a p1 and @a p2; append
		 * it to @a seq.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		static
		void
		create_segment_and_append_to_seq(
				seq_type &seq,
				const PointOnSphere &p1,
				const PointOnSphere &p2);


		/**
		 * This is the minimum number of (distinct) collection points to be passed into the
		 * 'create_on_heap' function to enable creation of a closed, well-defined polyline.
		 */
		static const unsigned s_min_num_collection_points;


		/**
		 * This is the sequence of polyline segments.
		 */
		seq_type d_seq;

	};


	inline
	PolylineOnSphere::VertexConstIterator
	PolylineOnSphere::VertexConstIterator::create_begin(
			const PolylineOnSphere &poly)
	{
		return VertexConstIterator(poly, poly.begin(), START);
	}
	

	inline
	PolylineOnSphere::VertexConstIterator
	PolylineOnSphere::VertexConstIterator::create_end(
			const PolylineOnSphere &poly)
	{
		return VertexConstIterator(poly, poly.end(), END);
	}


	/**
	 * Compare @a i1 and @a i2 for equality.
	 */
	inline
	bool
	operator==(
			const PolylineOnSphere::VertexConstIterator &i1,
			const PolylineOnSphere::VertexConstIterator &i2)
	{
		return (i1.curr_gca() == i2.curr_gca() &&
				i1.gca_start_or_end() == i2.gca_start_or_end());
	}


	/**
	 * Compare @a i1 and @a i2 for inequality.
	 */
	inline
	bool
	operator!=(
			const PolylineOnSphere::VertexConstIterator &i1,
			const PolylineOnSphere::VertexConstIterator &i2)
	{
		return (i1.curr_gca() != i2.curr_gca() ||
				i1.gca_start_or_end() != i2.gca_start_or_end());
	}


	/**
	 * Determine whether the two polylines @a poly1 and @a poly2 are
	 * equivalent when the directedness of the polyline segments is taken
	 * into account.
	 */
	bool
	polylines_are_directed_equivalent(
			const PolylineOnSphere &poly1,
			const PolylineOnSphere &poly2);


	/**
	 * Determine whether the two polylines @a poly1 and @a poly2 are
	 * equivalent when the directedness of the polyline segments is
	 * ignored.
	 *
	 * By this test, a polyline whose segments are [A, B, C, D] would be
	 * equivalent to polylines [A, B, C, D] or [D', C', B', A'], where 
	 * A' is the reverse of A, B' the reverse of B, and so on.
	 */
	bool
	polylines_are_undirected_equivalent(
			const PolylineOnSphere &poly1,
			const PolylineOnSphere &poly2);


	template<typename ForwardIter>
	PolylineOnSphere::ConstructionParameterValidity
	PolylineOnSphere::evaluate_construction_parameter_validity(
			ForwardIter begin,
			ForwardIter end,
			std::pair<ForwardIter, ForwardIter> &invalid_points)
	{
		const unsigned num_points = count_distinct_adjacent_points(begin, end);
		if (num_points < s_min_num_collection_points) {
			// The collection does not contain enough distinct points to create even
			// one line-segment.
			return INVALID_INSUFFICIENT_DISTINCT_POINTS;
		}

		ForwardIter prev;
		ForwardIter iter = begin;
		for (prev = iter++ ; iter != end; prev = iter++) {
			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;

			ConstructionParameterValidity v = evaluate_segment_endpoint_validity(p1, p2);

			// Using a switch-statement, along with GCC's "-Wswitch" option (implicitly
			// enabled by "-Wall"), will help to ensure that no cases are missed.
			switch (v) {

			case VALID:

				// Keep looping.
				break;

			case INVALID_INSUFFICIENT_DISTINCT_POINTS:

				// This value should never be returned, since it's not related to
				// segments.
				//
				// FIXME:  This sucks.  We should separate "global" errors (like
				// "insufficient distinct points") from "segment" errors (like
				// "antipodal segment endpoints").
				break;

			case INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:

				invalid_points.first = prev;
				invalid_points.second = iter;
				return v;
			}
		}

		// If we got this far, we couldn't find anything wrong with the
		// construction parameters.
		return VALID;
	}

	template<typename ForwardIter>
	const PolylineOnSphere::non_null_ptr_to_const_type
	PolylineOnSphere::create_on_heap(
			ForwardIter begin, ForwardIter end)
	{
		PolylineOnSphere::non_null_ptr_type ptr(new PolylineOnSphere(),
				GPlatesUtils::NullIntrusivePointerHandler());
		generate_segments_and_swap(*ptr, begin, end);
		return ptr;
	}


	/**
	 * The exception thrown when an attempt is made to create a polyline using invalid points.
	 */
	class InvalidPointsForPolylineConstructionError:
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		/**
		 * Instantiate the exception.
		 *
		 * @param cpv is the polyline's construction parameter validity value, which
		 * presumably describes why the points are invalid.
		 */
		InvalidPointsForPolylineConstructionError(
				PolylineOnSphere::ConstructionParameterValidity cpv,
				const char *filename,
				int line_num):
			d_cpv(cpv),
			d_filename(filename),
			d_line_num(line_num)
		{  }

		virtual
		~InvalidPointsForPolylineConstructionError()
		{  }

	protected:
		virtual
		const char *
		ExceptionName() const
		{
			return "InvalidPointsForPolylineConstructionError";
		}

		// FIXME: This would be better as a 'const std::string'.
		virtual
		std::string
		Message() const
		{
			switch (d_cpv) {
			case PolylineOnSphere::VALID:
				return "valid";
			case PolylineOnSphere::INVALID_INSUFFICIENT_DISTINCT_POINTS:
				return "insufficient distinct points";
			case PolylineOnSphere::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
				return "antipodal segment endpoints";
			}
			// Control-flow should never reach the end of this function.
			// FIXME:  We should assert this.
			// We'll return an empty string to placate the compiler, which is
			// complaining about control reaching end of non-void function.
			return std::string();
		}

	private:
		PolylineOnSphere::ConstructionParameterValidity d_cpv;
		const char *d_filename;
		int d_line_num;
	};


	template<typename ForwardIter>
	void
	PolylineOnSphere::generate_segments_and_swap(
			PolylineOnSphere &poly,
			ForwardIter begin,
			ForwardIter end)
	{
		std::pair<ForwardIter, ForwardIter> invalid_points;
		ConstructionParameterValidity v =
				evaluate_construction_parameter_validity(begin, end, invalid_points);
		if (v != VALID) {
			throw InvalidPointsForPolylineConstructionError(v, __FILE__, __LINE__);
		}

		// Make it easier to provide strong exception safety by appending the new segments
		// to a temporary sequence (rather than putting them directly into 'd_seq').
		seq_type tmp_seq;
		// Observe that the number of points used to define a polyline (which will become
		// the number of vertices in the polyline, counting the begin-point and end-point
		// of the polyline as vertices) is one greater than the number of segments in the
		// polyline.
		const seq_type::size_type num_points = std::distance(begin, end);
		tmp_seq.reserve(num_points - 1);

		ForwardIter prev;
		ForwardIter iter = begin;
		for (prev = iter++ ; iter != end; prev = iter++) {
			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;
			create_segment_and_append_to_seq(tmp_seq, p1, p2);
		}
		poly.d_seq.swap(tmp_seq);
	}


	/**
	 * This routine exports the Python wrapper class and associated functionality
	 */
	void export_PolylineOnSphere();

}

namespace std
{
	/**
	 * This is a template specialisation of the standard function @a swap.
	 *
	 * See Josuttis, section 4.4.2, "Swapping Two Values", for more information.
	 */
	template<>
	inline
	void
	swap<GPlatesMaths::PolylineOnSphere>(
			GPlatesMaths::PolylineOnSphere &p1,
			GPlatesMaths::PolylineOnSphere &p2)
	{
		p1.swap(p2);
	}
}

#endif  // GPLATES_MATHS_POLYLINEONSPHERE_H
