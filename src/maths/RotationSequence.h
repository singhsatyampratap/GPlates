/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
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
 * Authors:
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_ROTATIONSEQUENCE_H_
#define _GPLATES_MATHS_ROTATIONSEQUENCE_H_

#include <list>

#include "global/types.h"  /* rid_t */
#include "types.h"  /* real_t */
#include "FiniteRotation.h"


namespace GPlatesMaths
{
	using namespace GPlatesGlobal;


	/**
	 * This class represents a continuous sequence of finite rotations
	 * which describe the motion of a moving plate relative to a given
	 * fixed plate.
	 *
	 * If the sequence consists of a single finite rotation, it will
	 * be defined at a single point in time.  If it consists of two
	 * or more finite rotations, the sequence will span a period of
	 * time.  By interpolating between the specified finite rotations,
	 * it is possible to calculate a finite rotation for any point in
	 * time within this period.
	 *
	 * Note that if a finite rotation corresponding to 0 Ma (ie, the
	 * present-day) is not explicitly specified, the rotation sequence
	 * will not extend to the present-day.  There is no <em>automatic,
	 * implicit inclusion</em> of a 0 Ma finite rotation into a rotation
	 * sequence.
	 *
	 * If a rotation sequence consists of two or more finite rotations,
	 * one of which corresponds to 0 Ma, then the sequence will span
	 * a period of time which extends to the present-day.  Making the
	 * assumption that the motion of the plate does not experience any
	 * sudden change at the present-day, it is possible to extrapolate
	 * the motion represented by the most recent segment of the sequence
	 * to calculate the probable finite rotation of a point in time in
	 * the future.
	 */
	class RotationSequence
	{
		private:

			typedef std::list< FiniteRotation > seq_type;

			/**
			 * Since FiniteRotation instances are quite large,
			 * and there might be "several" (multiple tens of)
			 * FiniteRotations stored in a single rotation
			 * sequence, lessen the impact of copying a
			 * RotationSequence instance by sharing the actual
			 * sequence of FiniteRotation objects.
			 *
			 * This sharing is done with reference counting.
			 */
			class SharedSequence
			{
					/*
					 * Since the members of this class
					 * are always being altered, don't
					 * bother declaring any 'const'
					 * methods.
					 */

				private:

					seq_type _seq;

					/*
					 * Whether the sequence of finite
					 * rotations has been modified since
					 * it was last sorted -- if it has
					 * even been sorted yet at all.
					 */
					bool _is_modified;

				public:

					/*
					 * The number of references to an
					 * instance of this object.
					 */
					size_t _ref_count;


					SharedSequence()
					 : _is_modified(true),
					   _ref_count(1) {  }


					seq_type::const_iterator
					begin() {

						ensureSeqSorted();
						return _seq.begin();
					}


					seq_type::const_iterator
					end() {

						ensureSeqSorted();
						return _seq.end();
					}


					void
					insert(const FiniteRotation &frot) {

						_seq.push_back(frot);
						_is_modified = true;
					}

				private:

					void
					ensureSeqSorted() {

						if (_is_modified) {

							/*
							 * The sequence has
							 * been modified since
							 * last sort (or has
							 * not yet been sorted
							 * at all).
							 */
							_seq.sort();
							_is_modified = false;
						}
					}
			};

		public:

			/**
			 * Create a rotation sequence for motion of the given
			 * moving plate relative to the given fixed plate,
			 * initialising the sequence with a finite rotation.
			 *
			 * Since a finite rotation must be provided to this
			 * constructor, it will be assumed that a rotation
			 * sequence can never be empty.
			 */
			RotationSequence(const rid_t &fixed_plate,
			                 const rid_t &moving_plate,
			                 const FiniteRotation &frot);


			/**
			 * Explicitly define a copy-constructor, since
			 * we're doing ref-counting magic.
			 */
			RotationSequence(const RotationSequence &other)
			 : _fixed_plate(other._fixed_plate),
			   _moving_plate(other._moving_plate),
			   _most_recent_time(other._most_recent_time),
			   _most_distant_time(other._most_distant_time) {

				shareOthersSharedSeq(other);
			}


			/**
			 * Explicitly define a destructor, since
			 * we're doing ref-counting magic.
			 */
			~RotationSequence() {

				decrementSharedSeqRefCount(_shared_seq);
			}


			/**
			 * Explicitly define an assignment operator, since
			 * we're doing ref-counting magic.
			 */
			RotationSequence &
			operator=(const RotationSequence &other);


			real_t
			mostRecentTime() const { return _most_recent_time; }


			real_t
			mostDistantTime() const { return _most_distant_time; }


			rid_t
			fixedPlate() const { return _fixed_plate; }


			rid_t
			movingPlate() const { return _moving_plate; }


			/**
			 * Returns whether this rotation sequence is "defined"
			 * at a particular point in time.
			 *
			 * A rotation sequence is a continuous sequence which
			 * spans a certain period of time.  It is "defined"
			 * at all points in time which lie within this period.
			 */
			bool
			isDefinedAtTime(real_t t) const {

				// First, deal with times in the future.
				if (t < 0.0) {

					// It's a time in the future.
					return isDefinedInFuture();
				}
				return (_most_recent_time <= t &&
				        t <= _most_distant_time);
			}


			/**
			 * Returns whether this rotation sequence is "defined"
			 * in the future.
			 *
			 * For this to occur:
			 * - the rotation sequence must consist of two or more
			 *    finite rotations.
			 * - the most recent finite rotation must correspond
			 *    to the present-day.
			 */
			bool
			isDefinedInFuture() const {

				return (_most_recent_time == 0.0 &&
				       _most_distant_time != 0.0);
			}


			/**
			 * Insert another finite rotation into the sequence.
			 */
			void insert(const FiniteRotation &frot);


			/**
			 * If this rotation sequence is defined at time 't',
			 * calculate the finite rotation for time 't'.
			 *
			 * @throws InvalidOperationException if @a t is negative
			 *   (i.e. in the future) and @p isDefinedInFuture()
			 *   returns false.
			 * @throws InvalidOperationException if @a t is outside
			 *   the time-span of the rotation sequence.
			 * @throws ControlFlowException if, against all reason and
			 *   good taste, the flow of the execution makes it's way
			 *   into Never-Never Land.
			 */
			FiniteRotation finiteRotationAtTime(real_t t) const;

		private:

			rid_t  _fixed_plate;
			rid_t  _moving_plate;
			real_t _most_recent_time;  // Millions of years ago
			real_t _most_distant_time;  // Millions of years ago

			// shared using reference counting
			SharedSequence *_shared_seq;


			void
			shareOthersSharedSeq(const RotationSequence &other) {

				_shared_seq = other._shared_seq;
				++(_shared_seq->_ref_count);
			}


			void
			decrementSharedSeqRefCount(SharedSequence *ss) {

				if (--(ss->_ref_count) == 0) {

					/*
					 * There are no other references
					 * to the shared sequence.
					 */
					delete ss;
				}
			}
	};


	/**
	 * Although this operation doesn't strictly make sense for a
	 * RotationSequence, it is provided to enable RotationSequences to be
	 * sorted by STL algorithms.
	 */
	inline bool
	operator<(const RotationSequence &rs1, const RotationSequence &rs2) {

		return (rs1.mostRecentTime() < rs2.mostRecentTime());
	}
}

#endif  // _GPLATES_MATHS_ROTATIONSEQUENCE_H_
