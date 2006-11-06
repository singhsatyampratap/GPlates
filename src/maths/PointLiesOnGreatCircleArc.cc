/* $Id$ */

/**
 * @file
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 *
 * Copyright (C) 2003 The GPlates Consortium
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "PointLiesOnGreatCircleArc.h"
#include "Vector3D.h"


bool
GPlatesMaths::PointLiesOnGreatCircleArc::operator()(
 const PointOnSphere &test_point) const {

	const UnitVector3D &start_uv = d_arc_start.unitvector();
	const UnitVector3D &test_uv = test_point.unitvector();

	/*
	 * The normal to the plane which contains the start-point of the arc
	 * and the test-point.
	 */
	Vector3D test_normal = cross(start_uv, test_uv);

	/*
	 * The dot-product of the unit-vectors of the start-point of the arc
	 * and the test-point.  If this value is greater than that of
	 * 'd_arc_dot', the test-point is closer to the start-point than the
	 * end-point is.
	 */
	real_t test_dot = dot(start_uv, test_uv);

	/*
	 * If the function 'parallel' evaluates to true, the test-point must
	 * lie on the same great-circle as the arc; in fact, it must lie on the
	 * closed half-circle which begins at the start-point of the arc and
	 * passes through the end-point of the arc (or else the normals would
	 * be antiparallel rather than parallel).
	 *
	 * This is a *closed* half-circle (ie, one which includes both of its
	 * endpoints) because 'parallel' defines zero vectors as parallel to
	 * everything.  Obviously the unit-vector 'd_arc_normal' cannot be a
	 * zero-vector, but the vector 'test_normal' may be; if it is, then the
	 * test-point must be either coincident-with or antipodal-to the
	 * start-point.  Coincident with the start-point is OK, as this means
	 * that the test-point lies on the arc; antipodal to the start-point is
	 * not OK, as no arc may span an angle of PI radians (and more than PI
	 * radians is impossible).
	 *
	 * If, in addition to 'parallel' evaluating to true, 'test_dot' is
	 * greater-than or equal-to 'd_arc_dot', the test-point *must* lie on
	 * the arc, as it is both on the same closed half-circle (beginning at
	 * the start-point) as the arc, and at least as close to the
	 * start-point as the end-point is.
	 */
	return (parallel(d_arc_normal, test_normal) && (test_dot >= d_arc_dot));
}