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
 *   Hamish Law <hlaw@es.usyd.edu.au>
 *   James Boyden <jboyden@geosci.usyd.edu.au>
 */

#include "PointData.h"
#include "state/Layout.h"


using namespace GPlatesGeo;

PointData::PointData(const DataType_t& dt, const RotationGroupId_t& id,
	const TimeWindow& tw, const Attributes_t& attrs, 
	const GPlatesMaths::PointOnSphere& point)
	: DataOnSphere(dt, id, tw, attrs), _point(point)
{ }


void
PointData::Rotate(const GPlatesMaths::FiniteRotation &rot) const {

	GPlatesMaths::PointOnSphere pos = (rot * GetPointOnSphere());
	GPlatesState::Layout::InsertPointDataPos(this, pos);
}
