/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#include <ostream>
#include <sstream>
#include "PolygonOnSphere.h"
#include "PolygonProximityHitDetail.h"
#include "ProximityCriteria.h"
#include "ConstGeometryOnSphereVisitor.h"
#include "HighPrecision.h"
#include "global/InvalidParametersException.h"
#include "global/UninitialisedIteratorException.h"
#include "global/IntrusivePointerZeroRefCountException.h"


const unsigned
GPlatesMaths::PolygonOnSphere::s_min_num_collection_points = 3;


GPlatesMaths::PolygonOnSphere::ConstructionParameterValidity
GPlatesMaths::PolygonOnSphere::evaluate_segment_endpoint_validity(
		const PointOnSphere &p1,
		const PointOnSphere &p2)
{
	GreatCircleArc::ConstructionParameterValidity cpv =
			GreatCircleArc::evaluate_construction_parameter_validity(p1, p2);

	// Using a switch-statement, along with GCC's "-Wswitch" option (implicitly enabled by
	// "-Wall"), will help to ensure that no cases are missed.
	switch (cpv) {

	case GreatCircleArc::VALID:

		// Continue after switch block so that we don't get warnings
		// about "control reach[ing] end of non-void function".
		break;

	case GreatCircleArc::INVALID_ANTIPODAL_ENDPOINTS:

		return INVALID_ANTIPODAL_SEGMENT_ENDPOINTS;
	}
	return VALID;
}


GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesMaths::PolygonOnSphere::test_proximity(
		const ProximityCriteria &criteria) const
{
	// FIXME: This function should get its own implementation, rather than delegating to
	// 'is_close_to', to enable it to provide more hit detail (for example, whether a vertex or
	// a segment was hit).

	real_t closeness;  // Don't bother initialising this.
	if (this->is_close_to(criteria.test_point(), criteria.closeness_inclusion_threshold(),
			criteria.latitude_exclusion_threshold(), closeness)) {
		// OK, this polygon is close to the test point.
		return make_maybe_null_ptr(PolygonProximityHitDetail::create(
				this->get_non_null_pointer(),
				closeness.dval()));
	} else {
		return ProximityHitDetail::null;
	}
}


GPlatesMaths::ProximityHitDetail::maybe_null_ptr_type
GPlatesMaths::PolygonOnSphere::test_vertex_proximity(
	const ProximityCriteria &criteria) const
{
	vertex_const_iterator 
		v_it = vertex_begin(),
		v_end = vertex_end();

	real_t closeness;
	real_t closest_closeness_so_far = 0.;
	unsigned int index_of_closest_closeness = 0;
	bool have_found_close_vertex = false;
	for (unsigned int index = 0; v_it != v_end ; ++v_it, ++index)
	{
		ProximityHitDetail::maybe_null_ptr_type hit = v_it->test_proximity(criteria);
		if (hit)
		{
			have_found_close_vertex = true;
			closeness = hit->closeness();
			if (closeness.is_precisely_greater_than(closest_closeness_so_far.dval()))
			{
				closest_closeness_so_far = closeness;
				index_of_closest_closeness = index;
			}
		}
	}	

	if (have_found_close_vertex)
	{
		return make_maybe_null_ptr(PolygonProximityHitDetail::create(
			this->get_non_null_pointer(),
			closest_closeness_so_far.dval(),
			index_of_closest_closeness));
	}
	else
	{
		return ProximityHitDetail::null;	 
	}

}



void
GPlatesMaths::PolygonOnSphere::accept_visitor(
		ConstGeometryOnSphereVisitor &visitor) const
{
	visitor.visit_polygon_on_sphere(this->get_non_null_pointer());
}


bool
GPlatesMaths::PolygonOnSphere::is_close_to(
		const PointOnSphere &test_point,
		const real_t &closeness_inclusion_threshold,
		const real_t &latitude_exclusion_threshold,
		real_t &closeness) const
{
	// First, ensure the parameters are valid.
	if (((closeness_inclusion_threshold * closeness_inclusion_threshold) +
	     (latitude_exclusion_threshold * latitude_exclusion_threshold)) != 1.0) {

		/*
		 * Well, *duh*, they *should* equal 1.0: those two thresholds
		 * are supposed to form the non-hypotenuse legs (the "catheti")
		 * of a right-angled triangle inscribed in a unit circle.
		 */
		real_t calculated_hypotenuse =
				closeness_inclusion_threshold * closeness_inclusion_threshold +
				latitude_exclusion_threshold * latitude_exclusion_threshold;

		std::ostringstream oss;
		oss
		 << "The squares of the closeness inclusion threshold ("
		 << HighPrecision< real_t >(closeness_inclusion_threshold)
		 << ")\nand the latitude exclusion threshold ("
		 << HighPrecision< real_t >(latitude_exclusion_threshold)
		 << ") sum to ("
		 << HighPrecision< real_t >(calculated_hypotenuse)
		 << ")\nrather than the expected value of 1.";

		throw GPlatesGlobal::InvalidParametersException(GPLATES_EXCEPTION_SOURCE,
				oss.str().c_str());
	}

	real_t &closest_closeness_so_far = closeness;  // A descriptive alias.
	bool have_already_found_a_close_gca = false;

	const_iterator iter = begin(), the_end = end();
	for ( ; iter != the_end; ++iter) {

		const GreatCircleArc &the_gca = *iter;

		// Don't bother initialising this.
		real_t gca_closeness;

		if (the_gca.is_close_to(test_point,
		     closeness_inclusion_threshold,
		     latitude_exclusion_threshold, gca_closeness)) {

			if (have_already_found_a_close_gca) {

				if (gca_closeness.is_precisely_greater_than(
				     closest_closeness_so_far.dval())) {

					closest_closeness_so_far =
					 gca_closeness;
				}
				// else, do nothing.

			} else {

				closest_closeness_so_far = gca_closeness;
				have_already_found_a_close_gca = true;
			}
		}
	}
	return (have_already_found_a_close_gca);
}


void
GPlatesMaths::PolygonOnSphere::create_segment_and_append_to_seq(
		seq_type &seq, 
		const PointOnSphere &p1,
		const PointOnSphere &p2)
{
	// We'll assume that the validity of 'p1' and 'p2' to create a GreatCircleArc has been
	// evaluated in the function 'PolygonOnSphere::evaluate_construction_parameter_validity',
	// which was presumably invoked in 'PolygonOnSphere::generate_segments_and_swap' before
	// this function was.

	GreatCircleArc segment = GreatCircleArc::create(p1, p2);
	seq.push_back(segment);
}


void
GPlatesMaths::InvalidPointsForPolygonConstructionError::write_message(
		std::ostream &os) const
{
	const char *message;
	switch (d_cpv) {
	case PolygonOnSphere::VALID:
		message = "valid";
		break;
	case PolygonOnSphere::INVALID_INSUFFICIENT_DISTINCT_POINTS:
		message = "insufficient distinct points";
		break;
	case PolygonOnSphere::INVALID_ANTIPODAL_SEGMENT_ENDPOINTS:
		message = "antipodal segment endpoints";
		break;
	default:
		// Control-flow should never reach here.
		message = NULL;
		break;
	}

	if (message)
	{
		os << message;
	}
}

