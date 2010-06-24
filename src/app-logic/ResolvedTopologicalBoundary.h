/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALBOUNDARY_H
#define GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALBOUNDARY_H

#include <cstddef>
#include <iterator>
#include <vector>
#include <boost/operators.hpp>
#include <boost/optional.hpp>

#include "ReconstructionGeometry.h"

#include "maths/PolygonOnSphere.h"

#include "model/FeatureHandle.h"
#include "model/types.h"
#include "model/WeakObserver.h"

#include "property-values/GeoTimeInstant.h"


namespace GPlatesAppLogic
{
	class ResolvedTopologicalBoundary:
			public ReconstructionGeometry,
			public GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle>
	{
	public:
		/**
		 * A convenience typedef for a shared pointer to a non-const @a ResolvedTopologicalBoundary.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ResolvedTopologicalBoundary>
				non_null_ptr_type;

		/**
		 * A convenience typedef for a shared pointer to a non-const @a ResolvedTopologicalBoundary.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ResolvedTopologicalBoundary>
				non_null_ptr_to_const_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<ResolvedTopologicalBoundary>.
		 */
		typedef boost::intrusive_ptr<ResolvedTopologicalBoundary> maybe_null_ptr_type;

		/**
		 * A convenience typedef for boost::intrusive_ptr<const ResolvedTopologicalBoundary>.
		 */
		typedef boost::intrusive_ptr<const ResolvedTopologicalBoundary> maybe_null_ptr_to_const_type;

		/**
		 * A convenience typedef for the WeakObserver base class of this class.
		 */
		typedef GPlatesModel::WeakObserver<GPlatesModel::FeatureHandle> WeakObserverType;

		/**
		 * A convenience typedef for the geometry of this @a ResolvedTopologicalBoundary.
		 */
		typedef GPlatesMaths::PolygonOnSphere::non_null_ptr_to_const_type
				resolved_topology_geometry_ptr_type;

		/**
		 * A convenience typedef for the geometry of subsegments of this RTB.
		 */
		typedef GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type sub_segment_geometry_ptr_type;


		/**
		 * Records the reconstructed geometry, and any other relevant information,
		 * of a subsegment.
		 *
		 * A subsegment is the subset of a reconstructed topological section's
		 * vertices that are used to form part of the boundary of the
		 * resolved topology geometry.
		 */
		class SubSegment
		{
		public:
			SubSegment(
					const sub_segment_geometry_ptr_type &sub_segment_geometry,
					const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref,
					bool use_reverse) :
				d_sub_segment_geometry(sub_segment_geometry),
				d_feature_ref(feature_ref),
				d_use_reverse(use_reverse)
			{  }

			/**
			 * The subset of vertices of topological section used in resolved topology geometry.
			 *
			 * NOTE: The vertices have already been reversed if this subsegment is reversed
			 * (as determined by @a get_use_reverse).
			 */
			sub_segment_geometry_ptr_type
			get_geometry() const
			{
				return d_sub_segment_geometry;
			}

			//! Reference to the feature referenced by the topological section.
			const GPlatesModel::FeatureHandle::const_weak_ref &
			get_feature_ref() const
			{
				return d_feature_ref;
			}

			bool
			get_use_reverse() const
			{
				return d_use_reverse;
			}

		private:
			//! The subsegment geometry.
			sub_segment_geometry_ptr_type d_sub_segment_geometry;

			//! Reference to the source feature handle of the topological section.
			GPlatesModel::FeatureHandle::const_weak_ref d_feature_ref;

			//! Indicates if geometry direction was reversed when assembling topology.
			bool d_use_reverse;
		};


		//! Typedef for a sequence of @a SubSegment objects.
		typedef std::vector<SubSegment> sub_segment_seq_type;


		/**
		 * Forward iterator over export template filename sequence.
		 * Dereferencing iterator returns a 'const SubSegment &'.
		 */
		class SubSegmentConstIterator :
				public std::iterator<std::bidirectional_iterator_tag, const SubSegment>,
				public boost::bidirectional_iteratable<SubSegmentConstIterator, const SubSegment *>
		{
		public:
			//! Create begin iterator.
			static
			SubSegmentConstIterator
			create_begin(
					const sub_segment_seq_type &sub_segment_seq)
			{
				return SubSegmentConstIterator(sub_segment_seq, 0);
			}


			//! Create end iterator.
			static
			SubSegmentConstIterator
			create_end(
					const sub_segment_seq_type &sub_segment_seq)
			{
				return SubSegmentConstIterator(sub_segment_seq, sub_segment_seq.size());
			}


			/**
			 * Dereference operator.
			 * Operator->() provided by class boost::bidirectional_iteratable.
			 */
			const SubSegment &
			operator*() const
			{
				return (*d_sub_segment_seq)[d_sequence_index];
			}


			/**
			 * Pre-increment operator.
			 * Post-increment operator provided by base class boost::bidirectional_iteratable.
			 */
			SubSegmentConstIterator &
			operator++()
			{
				++d_sequence_index;
				return *this;
			}


			/**
			 * Pre-decrement operator.
			 * Post-decrement operator provided by base class boost::bidirectional_iteratable.
			 */
			SubSegmentConstIterator &
			operator--()
			{
				--d_sequence_index;
				return *this;
			}


			/**
			 * Equality comparison.
			 * Inequality operator provided by base class boost::bidirectional_iteratable.
			 */
			friend
			bool
			operator==(
					const SubSegmentConstIterator &lhs,
					const SubSegmentConstIterator &rhs)
			{
				return lhs.d_sub_segment_seq == rhs.d_sub_segment_seq &&
					lhs.d_sequence_index == rhs.d_sequence_index;
			}

		private:
			const sub_segment_seq_type *d_sub_segment_seq;
			std::size_t d_sequence_index;


			SubSegmentConstIterator(
					const sub_segment_seq_type &sub_segment_seq,
					std::size_t sequence_index) :
				d_sub_segment_seq(&sub_segment_seq),
				d_sequence_index(sequence_index)
			{  }
		};


		/**
		 * The type used to const_iterate over the subsegments.
		 */
		typedef SubSegmentConstIterator sub_segment_const_iterator;


		/**
		 * Create a ResolvedTopologicalBoundary instance with an optional plate ID and an
		 * optional time of formation.
		 *
		 * For instance, a ResolvedTopologicalBoundary might be created without a plate ID
		 * if no plate ID is found amongst the properties of the feature whose topological
		 * geometry was resolved.
		 */
		template<typename SubSegmentForwardIter>
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree,
				resolved_topology_geometry_ptr_type resolved_topology_geometry_ptr,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				SubSegmentForwardIter sub_segment_sequence_begin,
				SubSegmentForwardIter sub_segment_sequence_end,
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id_ = boost::none,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_ = boost::none)
		{
			return non_null_ptr_type(
					new ResolvedTopologicalBoundary(
							reconstruction_tree,
							resolved_topology_geometry_ptr,
							feature_handle,
							property_iterator_,
							sub_segment_sequence_begin,
							sub_segment_sequence_end,
							plate_id_,
							time_of_formation_),
					GPlatesUtils::NullIntrusivePointerHandler());
		}

		virtual
		~ResolvedTopologicalBoundary()
		{  }

		/**
		 * Get a non-null pointer to a const ResolvedTopologicalBoundary which points to this
		 * instance.
		 *
		 * Since the ResolvedTopologicalBoundary constructors are private, it should never
		 * be the case that a ResolvedTopologicalBoundary instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_to_const_type
		get_non_null_pointer_to_const() const
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Get a non-null pointer to a ResolvedTopologicalBoundary which points to this
		 * instance.
		 *
		 * Since the ResolvedTopologicalBoundary constructors are private, it should never
		 * be the case that a ResolvedTopologicalBoundary instance has been constructed on
		 * the stack.
		 */
		const non_null_ptr_type
		get_non_null_pointer()
		{
			return GPlatesUtils::get_non_null_pointer(this);
		}

		/**
		 * Return whether this RTB references @a that_feature_handle.
		 *
		 * This function will not throw.
		 */
		bool
		references(
				const GPlatesModel::FeatureHandle &that_feature_handle) const
		{
			return (feature_handle_ptr() == &that_feature_handle);
		}

		/**
		 * Return the pointer to the FeatureHandle.
		 *
		 * The pointer returned will be NULL if this instance does not reference a
		 * FeatureHandle; non-NULL otherwise.
		 *
		 * This function will not throw.
		 */
		GPlatesModel::FeatureHandle *
		feature_handle_ptr() const
		{
			return WeakObserverType::publisher_ptr();
		}

		/**
		 * Return whether this pointer is valid to be dereferenced (to obtain a
		 * FeatureHandle).
		 *
		 * This function will not throw.
		 */
		bool
		is_valid() const
		{
			return (feature_handle_ptr() != NULL);
		}

		/**
		 * Return a weak-ref to the feature whose resolved topological geometry this RTB
		 * contains, or an invalid weak-ref, if this pointer is not valid to be
		 * dereferenced.
		 */
		const GPlatesModel::FeatureHandle::weak_ref
		get_feature_ref() const;

		/**
		 * Access the topological polygon feature property used to generate
		 * the resolved topological geometry.
		 */
		const GPlatesModel::FeatureHandle::iterator
		property() const
		{
			return d_property_iterator;
		}

		/**
		 * Access the resolved topology polygon geometry.
		 */
		const resolved_topology_geometry_ptr_type
		resolved_topology_geometry() const
		{
			return d_resolved_topology_geometry_ptr;
		}

		/**
		 * Access the cached plate ID, if it exists.
		 *
		 * Note that it's possible for a ResolvedTopologicalBoundary to be created without
		 * a plate ID -- for example, if no plate ID is found amongst the properties of the
		 * feature whose topological geometry was resolved.
		 */
		const boost::optional<GPlatesModel::integer_plate_id_type> &
		plate_id() const
		{
			return d_plate_id;
		}

		/**
		 * Return the cached time of formation of the feature.
		 */
		const boost::optional<GPlatesPropertyValues::GeoTimeInstant> &
		time_of_formation() const
		{
			return d_time_of_formation;
		}


		/**
		 * Returns const iterator to beginning of the internal sequence of @a SubSegment objects.
		 */
		sub_segment_const_iterator
		sub_segment_begin() const
		{
			return sub_segment_const_iterator::create_begin(d_sub_segment_seq);
		}


		/**
		 * Returns const iterator to end of the internal sequence of @a SubSegment objects.
		 */
		sub_segment_const_iterator
		sub_segment_end() const
		{
			return sub_segment_const_iterator::create_end(d_sub_segment_seq);
		}


		/**
		 * Accept a ConstReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ConstReconstructionGeometryVisitor &visitor) const;

		/**
		 * Accept a ReconstructionGeometryVisitor instance.
		 */
		virtual
		void
		accept_visitor(
				ReconstructionGeometryVisitor &visitor);

		/**
		 * Accept a WeakObserverVisitor instance.
		 */
		virtual
		void
		accept_weak_observer_visitor(
				GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor);

	private:
		/**
		 * The resolved topology geometry.
		 */
		resolved_topology_geometry_ptr_type d_resolved_topology_geometry_ptr;

		/**
		 * This is an iterator to the (topological-geometry-valued) property from which
		 * this RTB was derived.
		 */
		GPlatesModel::FeatureHandle::iterator d_property_iterator;

		/**
		 * The cached plate ID, if it exists.
		 *
		 * Note that it's possible for a ResolvedTopologicalBoundary to be created without
		 * a plate ID -- for example, if no plate ID is found amongst the properties of the
		 * feature whose topological geometry was resolved.
		 *
		 * The plate ID is used when colouring feature geometries by plate ID.  It's also
		 * of interest to a user who has clicked on the feature geometry.
		 */
		boost::optional<GPlatesModel::integer_plate_id_type> d_plate_id;

		/**
		 * The cached time of formation of the feature, if it exists.
		 *
		 * This is cached so that it can be used to calculate the age of the feature at any
		 * particular reconstruction time.  The age of the feature is used when colouring
		 * feature geometries by age.
		 */
		boost::optional<GPlatesPropertyValues::GeoTimeInstant> d_time_of_formation;

		/**
		 * The sequence of @a SubSegment objects that form the resolved topology geometry.
		 *
		 * This contains the subset of vertices of each reconstructed topological section
		 * used to generate the resolved topology geometry.
		 */
		sub_segment_seq_type d_sub_segment_seq;


		/**
		 * Instantiate a resolved topological geometry with an optional reconstruction
		 * plate ID and an optional time of formation.
		 *
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		template<typename SubSegmentForwardIter>
		ResolvedTopologicalBoundary(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_,
				resolved_topology_geometry_ptr_type resolved_topology_geometry_ptr,
				GPlatesModel::FeatureHandle &feature_handle,
				GPlatesModel::FeatureHandle::iterator property_iterator_,
				SubSegmentForwardIter sub_segment_sequence_begin,
				SubSegmentForwardIter sub_segment_sequence_end,
				boost::optional<GPlatesModel::integer_plate_id_type> plate_id_,
				boost::optional<GPlatesPropertyValues::GeoTimeInstant> time_of_formation_):
			ReconstructionGeometry(reconstruction_tree_),
			WeakObserverType(feature_handle),
			d_resolved_topology_geometry_ptr(resolved_topology_geometry_ptr),
			d_property_iterator(property_iterator_),
			d_plate_id(plate_id_),
			d_time_of_formation(time_of_formation_),
			d_sub_segment_seq(sub_segment_sequence_begin, sub_segment_sequence_end)
		{  }
	};
}

#endif  // GPLATES_APP_LOGIC_RESOLVEDTOPOLOGICALBOUNDARY_H