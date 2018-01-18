/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2015 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_SCALARCOVERAGEDEFORMATION_H
#define GPLATES_APP_LOGIC_SCALARCOVERAGEDEFORMATION_H

#include <vector>
#include <boost/optional.hpp>

#include "DeformationStrainRate.h"
#include "ScalarCoverageEvolution.h"
#include "TimeSpanUtils.h"
#include "TopologyReconstruct.h"

#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	namespace ScalarCoverageDeformation
	{
		/**
		 * Builds and keeps track of scalar values (associated with points in a geometry) over a time span.
		 */
		class ScalarCoverageTimeSpan :
				public GPlatesUtils::ReferenceCount<ScalarCoverageTimeSpan>
		{
		public:

			//! A convenience typedef for a shared pointer to a non-const @a ScalarCoverageTimeSpan.
			typedef GPlatesUtils::non_null_intrusive_ptr<ScalarCoverageTimeSpan> non_null_ptr_type;

			//! A convenience typedef for a shared pointer to a const @a ScalarCoverageTimeSpan.
			typedef GPlatesUtils::non_null_intrusive_ptr<const ScalarCoverageTimeSpan> non_null_ptr_to_const_type;


			/**
			 * Creates an *empty* scalar coverage time span containing only the specified scalar values.
			 *
			 * Subsequently calling @a get_scalar_values will always return the specified scalar values
			 * regardless of reconstruction time specified.
			 */
			static
			non_null_ptr_type
			create(
					const std::vector<double> &scalar_values)
			{
				return non_null_ptr_type(new ScalarCoverageTimeSpan(scalar_values));
			}


			/**
			 * Creates a scalar coverage time span containing scalars.
			 *
			 * The time span of reconstructed/deformed feature geometries, @a geometry_time_span,
			 * supply the domain points associated with the scalar values. It contains deformation info
			 * such as strain rates that evolve our scalar values (eg, crustal thickness) and also
			 * contains deactivated points (eg, subducted/consumed).
			 *
			 * If @a scalar_evolution_function is specified then the scalar values are evolved over time
			 * (provided the geometry time span contains non-zero strain rates - ie, passed through a
			 * deforming network). Otherwise the geometry time span is only used to deactivate points
			 * (and their associated scalar values) over time.
			 *
			 * If @a scalar_values represent the scalar values at the geometry import time of the
			 * geometry time span - see 'TopologyReconstruct::create_geometry_time_span()' for more details.
			 * Those scalar values are then evolved forward and/or backward in time as necessary to fill the
			 * time range of the specified geometry time span.
			 */
			static
			non_null_ptr_type
			create(
					const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &geometry_time_span,
					const std::vector<double> &scalar_values,
					boost::optional<scalar_evolution_function_type> scalar_evolution_function = boost::none)
			{
				return non_null_ptr_type(
						new ScalarCoverageTimeSpan(
								geometry_time_span,
								scalar_values,
								scalar_evolution_function));
			}


			/**
			 * Returns true if the scalar values are active at the specified reconstruction time.
			 *
			 * If all geometry points (associated with the scalar values) subduct (going forward in time)
			 * or are consumed by mid-ocean ridges (going backward in time) or both, then the time range
			 * in which the geometry is valid will be reduced from the normal unlimited range (ie, [-inf, inf]).
			 * Note that this is different than the *feature* time of appearance/disappearance which
			 * is outside the scope of this class (and must be handled by the caller).
			 */
			bool
			is_valid(
					const double &reconstruction_time) const;

			/**
			 * Returns the scalar values at the specified time.
			 *
			 * Note: Only scalar values at *active* points are returned.
			 * And the order of scalar values matches the order of associated points
			 * returned by 'TopologyReconstruct::GeometryTimeSpan::get_geometry_data()'.
			 *
			 * Returns false if @a is_valid returns false.
			 */
			bool
			get_scalar_values(
					const double &reconstruction_time,
					std::vector<double> &scalar_values) const;

			/**
			 * Returns the scalar values at *all* points at the specified time (including inactive points).
			 *
			 * Note: Inactive points will store 'none'.
			 * And the order of scalar values matches the order of associated points
			 * returned by 'TopologyReconstruct::GeometryTimeSpan::get_all_geometry_data()'.
			 *
			 * Returns false if @a is_valid returns false.
			 */
			bool
			get_all_scalar_values(
					const double &reconstruction_time,
					std::vector< boost::optional<double> > &scalar_values) const;


			/**
			 * The time range of this scalar coverage time span.
			 */
			TimeSpanUtils::TimeRange
			get_time_range() const
			{
				return d_scalar_values_time_span->get_time_range();
			}

			/**
			 * The time that we started topology reconstruction of the initial scalar values from.
			 */
			const double &
			get_scalar_import_time() const
			{
				return d_scalar_import_time;
			}

		private:

			/**
			 * Typedef for a sequence of (per-point) scalar values.
			 *
			 * Inactive points/scalars are none.
			 */
			typedef std::vector< boost::optional<double> > scalar_value_seq_type;

			//! Typedef for a time span of scalar value sequences.
			typedef TimeSpanUtils::TimeWindowSpan<scalar_value_seq_type> scalar_values_time_span_type;


			scalar_values_time_span_type::non_null_ptr_type d_scalar_values_time_span;
			double d_scalar_import_time;

			//! The first time slot that the geometry becomes active (if was even de-activated going backward in time).
			boost::optional<unsigned int> d_time_slot_of_appearance;
			//! The last time slot that the geometry remains active (if was even de-activated going forward in time).
			boost::optional<unsigned int> d_time_slot_of_disappearance;


			static
			scalar_value_seq_type
			create_import_sample(
					const std::vector<double> &scalar_values,
					boost::optional<TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type> geometry_time_span = boost::none);

			explicit
			ScalarCoverageTimeSpan(
					const std::vector<double> &present_day_scalar_values);

			explicit
			ScalarCoverageTimeSpan(
					const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &geometry_time_span,
					const std::vector<double> &scalar_values,
					boost::optional<scalar_evolution_function_type> scalar_evolution_function);

			void
			initialise_time_span(
					const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &geometry_time_span,
					const boost::optional<scalar_evolution_function_type> &scalar_evolution_function);

			bool
			evolve_time_steps(
					scalar_value_seq_type &current_scalar_values,
					unsigned int start_time_slot,
					unsigned int end_time_slot,
					const TopologyReconstruct::GeometryTimeSpan::non_null_ptr_type &geometry_time_span,
					const boost::optional<scalar_evolution_function_type> &scalar_evolution_function);

			scalar_value_seq_type
			create_rigid_scalar_values_sample(
					const double &reconstruction_time,
					const double &closest_younger_sample_time,
					const scalar_value_seq_type &closest_younger_sample);

			scalar_value_seq_type
			interpolate_scalar_values_sample(
					const double &interpolate_position,
					const double &first_geometry_time,
					const double &second_geometry_time,
					const scalar_value_seq_type &first_sample,
					const scalar_value_seq_type &second_sample);
		};
	}
}

#endif // GPLATES_APP_LOGIC_SCALARCOVERAGEDEFORMATION_H
