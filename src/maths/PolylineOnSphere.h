/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef GPLATES_MATHS_POLYLINEONSPHERE_H
#define GPLATES_MATHS_POLYLINEONSPHERE_H

#include <vector>
#include <iterator>  /* iterator, bidirectional_iterator_tag */
#include <utility>  /* pair */
#include <boost/intrusive_ptr.hpp>
#include "GreatCircleArc.h"
#include "InvalidPolylineException.h"

namespace GPlatesMaths {

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
	 * You can create a polyline by invoking either the static member
	 * function @a PolylineOnSphere::create or the static member function
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
	 * this sequence to the @a PolylineOnSphere::create function, it will
	 * create a polyline composed of 3 segments: A->B, B->C, C->D.  If you
	 * subsequently iterate through the vertices of this polyline, you will
	 * get the same sequence of points back again: A, B, C, D.
	 */
	class PolylineOnSphere {

	 public:

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
		typedef std::vector< GreatCircleArc > seq_type;


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
		class VertexConstIterator: public std::iterator<
		 std::bidirectional_iterator_tag, PointOnSphere > {
		
			enum StartOrEnd {

				START,
				END
			};

			typedef PolylineOnSphere::const_iterator
			 gca_const_iterator;

		  public:

			/**
			 * Create the "begin" vertex iterator for @a poly.
			 */
			static
			VertexConstIterator
			create_begin(
			 const PolylineOnSphere &poly);


			/**
			 * Create the "end" vertex iterator for @a poly.
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
			 *  - increment
			 *  - decrement
			 */
			VertexConstIterator() :
			 d_poly_ptr(NULL), 
			 d_curr_gca(), 
			 d_gca_start_or_end(END) {  }


			/**
			 * Copy-construct a vertex iterator.
			 */
			VertexConstIterator(
			 const VertexConstIterator &other) :
			 d_poly_ptr(other.d_poly_ptr),
			 d_curr_gca(other.d_curr_gca),
			 d_gca_start_or_end(other.d_gca_start_or_end) {  }


			/**
			 * Return the gca_const_iterator which points to the
			 * current GreatCircleArc.
			 */
			gca_const_iterator
			curr_gca() const {

				return d_curr_gca;
			}


			/**
			 * Return whether this iterator is pointing at the
			 * "start-point" or "end-point" of the current
			 * GreatCircleArc.
			 */
			StartOrEnd
			gca_start_or_end() const {

				return d_gca_start_or_end;
			}


			/**
			 * Assign @a other to this.
			 */
			VertexConstIterator &
			operator=(
			 const VertexConstIterator &other) {

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
			operator*() const {

				return current_point();
			}


			/**
			 * Access a member of the PointOnSphere which is
			 * currently being pointed-at by this iterator.
			 */
			const PointOnSphere *
			operator->() const {

				return &(current_point());
			}


			/**
			 * Pre-increment this iterator.
			 */
			VertexConstIterator &
			operator++() {

				increment();

				return *this;
			}


			/**
			 * Post-increment this iterator.
			 */
			const VertexConstIterator
			operator++(int) {

				VertexConstIterator old = *this;
				increment();

				return old;
			}


			/**
			 * Pre-decrement this iterator.
			 */
			VertexConstIterator &
			operator--() {

				decrement();

				return *this;
			}


			/**
			 * Post-decrement this iterator.
			 */
			const VertexConstIterator
			operator--(int) {

				VertexConstIterator old = *this;
				decrement();

				return old;
			}

		  private:

			/**
			 * Gee, why do you think this is private?
			 */
			VertexConstIterator(
			 const PolylineOnSphere &poly,
			 gca_const_iterator curr_gca_,
			 StartOrEnd gca_start_or_end_) :
			 d_poly_ptr(&poly),
			 d_curr_gca(curr_gca_),
			 d_gca_start_or_end(gca_start_or_end_) {  }


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

			const PolylineOnSphere *d_poly_ptr;

			gca_const_iterator d_curr_gca;

			StartOrEnd d_gca_start_or_end;

		};


		/**
		 * The type used to const_iterate over the vertices.
		 */
		typedef VertexConstIterator vertex_const_iterator;


		/**
		 * The type used for the reference-count.
		 */
		typedef long ref_count_type;


		/**
		 * The possible return values from the construction-parameter
		 * validation functions
		 * @a evaluate_construction_parameter_validity and
		 * @a evaluate_segment_endpoint_validity.
		 */
		enum ConstructionParameterValidity {

			VALID,
			INVALID_INSUFFICIENT_POINTS,
			INVALID_INSUFFICIENT_UNIQUE_POINTS,
			INVALID_DUPLICATE_SEGMENT_ENDPOINTS,
			INVALID_ANTIPODAL_SEGMENT_ENDPOINTS
		};


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
		 * qualifias as valid parameters (no duplicate or antipodal
		 * adjacent points; at least two distinct points in the
		 * collection -- nothing particularly unreasonable) but the
		 * creation functions are fairly unsympathetic if your
		 * parameters @em do turn out to be invalid.
		 *
		 * @a coll should be a sequential STL container (list, vector,
		 * ...) of PointOnSphere.
		 *
		 * @a invalid_points is a return-parameter; if the
		 * construction-parameters are found to be invalid due to
		 * duplicate or antipodal adjacent points, the value of this
		 * return-parameter will be set to the pair of const_iterators
		 * of @a coll which point to the guilty points.  If no adjacent
		 * points are found to be duplicate or antipodal, this
		 * parameter will not be modified.
		 *
		 * The optional argument @a should_silently_drop_dups controls
		 * whether or not duplicate adjacent points should silently be
		 * dropped instead of causing an exception to be thrown.  (Dup
		 * adjacent points are a not-uncommon occurrence when reading
		 * PLATES data files.  All Hail PLATES!)
		 */
		template< typename C >
		static
		ConstructionParameterValidity
		evaluate_construction_parameter_validity(
		 const C &coll,
		 std::pair< typename C::const_iterator, typename
		  C::const_iterator > &invalid_points,
		 bool should_silently_drop_dups = true);


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
		 * Create a new PolylineOnSphere instance from the sequence of
		 * points @a coll.
		 *
		 * @a coll should be a sequential STL container (list, vector,
		 * ...) of PointOnSphere.
		 *
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		template< typename C >
		static
		PolylineOnSphere
		create(
		 const C &coll);


		/**
		 * Create a new PolylineOnSphere instance on the heap from the sequence of points
		 * @a coll, and return an intrusive_ptr which points to the newly-created instance.
		 *
		 * @a coll should be a sequential STL container (list, vector, ...) of
		 * PointOnSphere.
		 *
		 * This function is strongly exception-safe and exception-neutral.
		 */
		template< typename C >
		static
		boost::intrusive_ptr< PolylineOnSphere >
		create_on_heap(
		 const C &coll);


		/**
		 * Return the "begin" const_iterator to iterate over the
		 * sequence of GreatCircleArc which defines this polyline.
		 */
		const_iterator
		begin() const {
			
			return d_seq.begin();
		}


		/**
		 * Return the "end" const_iterator to iterate over the
		 * sequence of GreatCircleArc which defines this polyline.
		 */
		const_iterator
		end() const {
			
			return d_seq.end();
		}


		/**
		 * Return the number of segments in this polyline.
		 */
		size_type
		number_of_segments() const {

			return d_seq.size();
		}


		/**
		 * Return the "begin" const_iterator to iterate over the
		 * vertices of this polyline.
		 */
		vertex_const_iterator
		vertex_begin() const {

			return vertex_const_iterator::create_begin(*this);
		}


		/**
		 * Return the "end" const_iterator to iterate over the
		 * vertices of this polyline.
		 */
		vertex_const_iterator
		vertex_end() const {

			return vertex_const_iterator::create_end(*this);
		}


		/**
		 * Return the number of vertices in this polyline.
		 */
		size_type
		number_of_vertices() const {

			return (d_seq.size() + 1);
		}


		/**
		 * Return the start-point of this polyline.
		 */
		const PointOnSphere &
		start_point() const {

			const GreatCircleArc &first_gca = *(begin());

			return first_gca.start_point();
		}


		/**
		 * Return the end-point of this polyline.
		 */
		const PointOnSphere &
		end_point() const {

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
		 PolylineOnSphere &other) {

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
		 * @a GPlatesState::Layout::find_close_data.
		 */
		bool
		is_close_to(
		 const PointOnSphere &test_point,
		 const real_t &closeness_inclusion_threshold,
		 const real_t &latitude_exclusion_threshold,
		 real_t &closeness) const;

		/**
		 * Increment the reference-count of this instance.
		 */
		void
		increment_ref_count() {
			++d_ref_count;
		}


		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 */
		ref_count_type
		decrement_ref_count() {
			return --d_ref_count;
		}

	 private:

		PolylineOnSphere() {  }


		/**
		 * Generate a sequence of polyline segments from the collection
		 * of points @a coll, using the points to define the endpoints
		 * and vertices of the segments, then swap this new sequence of
		 * segments into the polyline @a poly, discarding any sequence
		 * of segments which may have been there before.
		 *
		 * This function is strongly exception-safe and
		 * exception-neutral.
		 */
		template< typename C >
		static
		void
		generate_segments_and_swap(
		 PolylineOnSphere &poly,
		 const C &coll);


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
		 const PointOnSphere &p2,
		 bool should_silently_drop_dups = true);

		/**
		 * This is the reference-count used by boost::intrusive_ptr.
		 */
		ref_count_type d_ref_count;

		/**
		 * This is the sequence of polyline segments.
		 */
		seq_type d_seq;

	};


	inline
	PolylineOnSphere::VertexConstIterator
	PolylineOnSphere::VertexConstIterator::create_begin(
	 const PolylineOnSphere &poly) {

		return VertexConstIterator(poly, poly.begin(), START);
	}
	

	inline
	PolylineOnSphere::VertexConstIterator
	PolylineOnSphere::VertexConstIterator::create_end(
	 const PolylineOnSphere &poly) {

		return VertexConstIterator(poly, poly.end(), END);
	}


	/**
	 * Compare @a i1 and @a i2 for equality.
	 */
	inline
	bool
	operator==(
	 const PolylineOnSphere::VertexConstIterator &i1,
	 const PolylineOnSphere::VertexConstIterator &i2) {

		return
		 (i1.curr_gca() == i2.curr_gca() &&
		  i1.gca_start_or_end() == i2.gca_start_or_end());
	}


	/**
	 * Compare @a i1 and @a i2 for inequality.
	 */
	inline
	bool
	operator!=(
	 const PolylineOnSphere::VertexConstIterator &i1,
	 const PolylineOnSphere::VertexConstIterator &i2) {

		return
		 (i1.curr_gca() != i2.curr_gca() ||
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


	template< typename C >
	PolylineOnSphere::ConstructionParameterValidity
	PolylineOnSphere::evaluate_construction_parameter_validity(
	 const C &coll,
	 std::pair< typename C::const_iterator, typename
	  C::const_iterator > &invalid_points,
	 bool should_silently_drop_dups) {

		typename C::size_type num_points = coll.size();
		if (num_points < 2) {

			// The collection does not contain enough points to
			// create even one line-segment.
			return INVALID_INSUFFICIENT_POINTS;
		}

		typename C::const_iterator
		 prev,
		 iter = coll.begin(),
		 end = coll.end();
		for (prev = iter++ ; iter != end; prev = iter++) {

			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;

			ConstructionParameterValidity v =
			 evaluate_segment_endpoint_validity(p1, p2);

			// Using a switch-statement, along with GCC's
			// "-Wswitch" option (implicitly enabled by "-Wall"),
			// will help to ensure that no cases are missed.
			switch (v) {

			 case VALID:

				// Keep looping.
				break;

			 case INVALID_INSUFFICIENT_POINTS:
			 case INVALID_INSUFFICIENT_UNIQUE_POINTS:

				// These values shouldn't be returned.
				// FIXME:  Can this be checked at compile-time?
				// (Perhaps with use of template magic, to
				// avoid the need to check at run-time and
				// throw an exception if the assertion fails.)
				break;

			 case INVALID_DUPLICATE_SEGMENT_ENDPOINTS:

				if (should_silently_drop_dups) {

					// You heard the man:  We should
					// silently drop duplicates.  But we
					// still need to keep track of the
					// number of (usable) points.
					--num_points;

				} else {

					invalid_points.first = prev;
					invalid_points.second = iter;

					return v;
				}
				// Keep looping.
				break;

			 case INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:

				invalid_points.first = prev;
				invalid_points.second = iter;

				return v;
			}
		}
		// Check the number of (usable) points again, now that we've
		// abjusted for duplicates.
		if (num_points < 2) {

			return INVALID_INSUFFICIENT_UNIQUE_POINTS;
		}

		// If we got this far, we couldn't find anything wrong with the
		// construction parameters.
		return VALID;
	}


	template< typename C >
	PolylineOnSphere
	PolylineOnSphere::create(
	 const C &coll) {

		PolylineOnSphere p;
		generate_segments_and_swap(p, coll);
		return p;
	}


	template< typename C >
	boost::intrusive_ptr< PolylineOnSphere >
	PolylineOnSphere::create_on_heap(
	 const C &coll) {

		boost::intrusive_ptr< PolylineOnSphere > ptr(new PolylineOnSphere());
		generate_segments_and_swap(*ptr, coll);
		return ptr;
	}


	template< typename C >
	void
	PolylineOnSphere::generate_segments_and_swap(
	 PolylineOnSphere &poly,
	 const C &coll) {

		if (coll.size() < 2) {

			// The collection does not contain enough points to
			// create even one line-segment.
			// FIXME:  I don't like throwing in a header-file.
			throw InvalidPolylineException("Attempted to create a "
			 "polyline from an insufficient number (ie, less than "
			 "2) of endpoints.");
		}

		// Make it easier to provide strong exception safety by
		// appending the new segments to a temporary sequence (rather
		// than putting them directly into 'd_seq').
		seq_type tmp_seq;
		tmp_seq.reserve(coll.size() - 1);

		typename C::const_iterator
		 prev,
		 iter = coll.begin(),
		 end = coll.end();
		for (prev = iter++ ; iter != end; prev = iter++) {

			const PointOnSphere &p1 = *prev;
			const PointOnSphere &p2 = *iter;

			create_segment_and_append_to_seq(tmp_seq, p1, p2);
		}

		if (tmp_seq.size() == 0) {

			// No line-segments were created, which must mean that
			// all points in the collection were identical.
			// FIXME:  I don't like throwing in a header-file.
			throw InvalidPolylineException("Attempted to create a "
			 "polyline from an insufficient number (ie, less than "
			 "2) of unique endpoints.");
		}
		poly.d_seq.swap(tmp_seq);
	}


	inline
	void
	intrusive_ptr_add_ref(
			PolylineOnSphere *p) {
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			PolylineOnSphere *p) {
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MATHS_POLYLINEONSPHERE_H