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

#ifndef GPLATES_APP_LOGIC_SCALARCOVERAGEEVOLUTION_H
#define GPLATES_APP_LOGIC_SCALARCOVERAGEEVOLUTION_H

#include <map>
#include <vector>
#include <boost/optional.hpp>

#include "DeformationStrainRate.h"
#include "ReconstructScalarCoverageParams.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "property-values/ValueObjectType.h"


namespace GPlatesAppLogic
{
	/**
	 * This namespace contains functions that evolve the scalar values (in a scalar coverage) over a time interval.
	 *
	 * For example a scalar coverage containing crustal thickness scalars will get evolved using
	 * the @a crustal_thinning function.
	 */
	class ScalarCoverageEvolution
	{
	public:

		enum EvolvedScalarType
		{
			// This can be absolute thickness (eg, in kms), or
			// a thickness ratio such as T/Ti (where T/Ti is current/initial thickness)...
			CRUSTAL_THICKNESS,
			// Stretching (beta) factor where 'beta = Ti/T' ...
			CRUSTAL_STRETCHING_FACTOR,
			// Thinning (gamma) factor where 'gamma = (1 - T/Ti)' ...
			CRUSTAL_THINNING_FACTOR,

			NUM_EVOLVED_SCALAR_TYPES
		};


		//! Typedef for scalar type.
		typedef GPlatesPropertyValues::ValueObjectType scalar_type_type;

		/**
		 * Typedef for a sequence of (per-point) scalar values.
		 *
		 * Inactive scalars are none.
		 */
		typedef std::vector< boost::optional<double> > scalar_value_seq_type;

		/**
		 * Initial scalar values (to be evolved).
		 */
		class InitialEvolvedScalarCoverage
		{
		public:

			//! Typedef for a map of evolved scalar types to initial scalar values (to be evolved).
			typedef std::map<EvolvedScalarType, std::vector<double>> initial_scalar_values_map_type;

			void
			add_scalar_values(
					EvolvedScalarType evolved_scalar_type,
					const std::vector<double> &initial_scalar_values);

			bool
			empty() const
			{
				return d_initial_evolved_scalar_values.empty();
			}

			const initial_scalar_values_map_type &
			get_scalar_values() const
			{
				return d_initial_evolved_scalar_values;
			}

			/**
			 * Returns number of scalar values (per scalar type).
			 *
			 * Throws exception if @a add_scalar_values has not yet been called at least once.
			 */
			unsigned int
			get_num_scalar_values() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_num_scalar_values,
						GPLATES_ASSERTION_SOURCE);

				return d_num_scalar_values.get();
			}

		private:
			//! The initial scalar values (to be evolved) associated with evolved scalar types.
			std::map<EvolvedScalarType, std::vector<double>> d_initial_evolved_scalar_values;

			//! Number of scalar values (per scalar type) - only initialised after first call to @a add_scalar_values.
			boost::optional<unsigned int> d_num_scalar_values;
		};


		/**
		 * Scalar values (associated with points in a geometry) for only those scalar types that
		 * evolve over time (due to deformation).
		 */
		class EvolvedScalarCoverage
		{
		public:

			/**
			 * Returns the scalar values at *all* points at the specified time (including inactive points).
			 *
			 * Note: Inactive points will store 'none' (such that the size of the returned
			 * scalar value sequence will always be @a get_num_all_scalar_values).
			 * And the order of scalar values matches the order of associated points
			 * returned by 'TopologyReconstruct::GeometryTimeSpan::get_all_geometry_data()'.
			 */
			const scalar_value_seq_type &
			get_evolved_scalar_values(
					EvolvedScalarType evolved_scalar_type) const
			{
				return d_evolved_scalar_values[evolved_scalar_type];
			}

			/**
			 * Returns number of scalar values (per scalar type).
			 */
			unsigned int
			get_num_scalar_values() const
			{
				return d_evolved_scalar_values[0].size();
			}

		private:
			scalar_value_seq_type d_evolved_scalar_values[NUM_EVOLVED_SCALAR_TYPES];

			// Only class ScalarCoverageEvolution can instantiate us.
			explicit
			EvolvedScalarCoverage(
					unsigned int num_scalar_values)
			{
				// Use default initial scalar values except for crustal thickness (leave as boost::none).
				// Crustal stretching and thinning factors are ratios so we can assume initial values for those.
				d_evolved_scalar_values[CRUSTAL_THICKNESS].resize(num_scalar_values);
				d_evolved_scalar_values[CRUSTAL_STRETCHING_FACTOR].resize(num_scalar_values, 1.0);  // Ti/T
				d_evolved_scalar_values[CRUSTAL_THINNING_FACTOR].resize(num_scalar_values, 0.0);  // (1 - T/Ti)
			}
			friend class ScalarCoverageEvolution;
		};


		/**
		 * Returns the scalar type associated with the specified evolved scalar type enumeration.
		 */
		static
		scalar_type_type
		get_scalar_type(
				EvolvedScalarType evolved_scalar_type);

		/**
		 * Returns the evolved scalar type enumeration associated with the specified scalar type,
		 * otherwise returns none.
		 */
		static
		boost::optional<EvolvedScalarType>
		is_evolved_scalar_type(
				const scalar_type_type &scalar_type);

		
		ScalarCoverageEvolution(
				const InitialEvolvedScalarCoverage &initial_evolved_scalar_coverage,
				const double &initial_time);


		/**
		 * Evolves the current scalar values from the current time to @a evolve_time.
		 *
		 * Note that 'boost::optional' is used for each point's scalar values and strain rate.
		 * This represents whether the associated point is active. Points can become inactive over time (active->inactive) but
		 * do not get re-activated (inactive->active). So if the current strain rate is inactive then so should the evolve-to strain rate.
		 * Also the active state of the current scalar value should match that of the current deformation strain rate
		 * (because they represent the same point). If the current scalar value is inactive then it just remains inactive.
		 * And if the current scalar value is active then it becomes inactive if the evolve-to strain rate is inactive, otherwise
		 * both (current and evolve-to) strain rates are active and the scalar value is evolved from its current value to its evolve-to value.
		 * This ensures the active state of the evolve-to scalar values match that of the evolve-to deformation strain rate
		 * (which in turn comes from the active state of the associated domain geometry point).
		 *
		 * Throws exception if the sizes of the strain rate arrays and the internal scalar values arrays do not match.
		 */
		void
		evolve(
				// Per-point deformation strain rates at the current time...
				const std::vector< boost::optional<DeformationStrainRate> > &current_deformation_strain_rates,
				// Per-point deformation strain rates at the evolve time...
				const std::vector< boost::optional<DeformationStrainRate> > &evolve_deformation_strain_rates,
				const double &evolve_time);

		/**
		 * Returns the current time.
		 *
		 * This is either 'evolve_time' in the last call to @a evolve or the initial time
		 * (passed into constructor) if @a evolve has not yet been called.
		 */
		const double &
		get_current_time() const
		{
			return d_current_time;
		}

		/**
		 * Return the current evolution of the scalar coverage corresponding to the last call to
		 * @a evolve (or the initial scalar coverage if has not yet been called).
		 */
		const EvolvedScalarCoverage &
		get_current_evolved_scalar_coverage() const
		{
			return d_current_evolved_scalar_coverage;
		}

		/**
		 * Returns number of scalar values (per scalar type).
		 */
		unsigned int
		get_num_scalar_values() const
		{
			return get_current_evolved_scalar_coverage().get_num_scalar_values();
		}

	private:
		double d_current_time;
		EvolvedScalarCoverage d_current_evolved_scalar_coverage;
	};
}

#endif // GPLATES_APP_LOGIC_SCALARCOVERAGEEVOLUTION_H
