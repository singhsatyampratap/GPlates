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

#ifndef _GPLATES_MATHS_GREATCIRCLE_H_
#define _GPLATES_MATHS_GREATCIRCLE_H_

#include "types.h"
#include "Axial.h"
#include "UnitVector3D.h"
#include "PointOnSphere.h"
#include "Vector3D.h"

namespace GPlatesMaths
{
	/** 
	 * A great circle of a unit sphere.
	 */
	class GreatCircle : public Axial
	{
		public:
			/**
			 * Create a great circle, given its axis.
			 * @param axis The axis vector.
			 */
			GreatCircle (const UnitVector3D &axis)
				: Axial (axis) { }

			/**
			 * Create a great circle, given two points on it.
			 * @param p1 One point.
			 * @param p2 Another point. Must be distinct from v1.
			 *
			 * @throws IndeterminateResultException if @a p1 and 
			 *   @a p2 are either coincident or antipodal.
			 */
			GreatCircle (const PointOnSphere &p1,
			             const PointOnSphere &p2);

			UnitVector3D
			normal () const { return axisvector(); }

			/**
			 * Evaluate whether the point @a pt lies on this
			 * great circle.
			 */
			bool
			contains (const PointOnSphere &pt) const {

				return perpendicular (axisvector (),
				                   pt.unitvector ());
			}

#if 0  // No longer needed
			/**
			 * Computes one intersection point of this GreatCircle
			 * with another. The other intersection point is the
			 * antipodal point to this.
			 * @param other The other GreatCircle.
			 */
			UnitVector3D intersection (const GreatCircle &other)
			 const;
#endif

		private:
			/**
			 * Given two unit vectors, @a v1 and @a v2, calculate
			 * the normal of the great circle they define.
			 *
			 * This function is invoked by one of the constructors.
			 * 
			 * @throws IndeterminateResultException if the vectors 
			 *   @a v1 and @a v2 are either parallel or antiparallel.
			 */
			static UnitVector3D calcNormal(const UnitVector3D &v1,
			                               const UnitVector3D &v2);
	};

	inline GreatCircle
	operator- (const GreatCircle &c)
	{
		UnitVector3D v = -c.normal ();
		return GreatCircle (v);
	}

	inline bool
	areEquivalent (const GreatCircle &a, const GreatCircle &b)
	{
		// faster than two vector comparisons and a vector negation...
		return collinear(a.normal(), b.normal());
	}
}

#endif  // _GPLATES_MATHS_GREATCIRCLE_H_
