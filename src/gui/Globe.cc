/* $Id$ */

/**
 * @file 
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

#include <iostream>
#include <cstdlib>
#include <algorithm>

#include "Globe.h"
#include "PlatesColourTable.h"
#include "state/Layout.h"


using namespace GPlatesMaths;
using namespace GPlatesState;


static void
CallVertexWithPoint(const PointOnSphere& p)
{
	UnitVector3D uv = p.unitvector();
	glVertex3d(uv.x().dval(), uv.y().dval(), uv.z().dval());
}


static void
CallVertexWithLine(const PolyLineOnSphere::const_iterator& begin, 
                   const PolyLineOnSphere::const_iterator& end)
{
	PolyLineOnSphere::const_iterator iter = begin;

	glBegin(GL_LINE_STRIP);
		CallVertexWithPoint(iter->startPoint());
		for ( ; iter != end; ++iter)
			CallVertexWithPoint(iter->endPoint());
	glEnd();
}


static void
PaintPointDataPos(const Layout::PointDataPos& pointdata)
{
	const PointOnSphere& point = pointdata.second;
	CallVertexWithPoint(point);
}


static void
PaintLineDataPos(const Layout::LineDataPos& linedata)
{
	using namespace GPlatesGui;

	const PlatesColourTable &ctab = *(PlatesColourTable::Instance());
	const PolyLineOnSphere& line = linedata.second;

	GPlatesGlobal::rid_t rgid = linedata.first->GetRotationGroupId();
	PlatesColourTable::const_iterator it = ctab.lookup(rgid);
	if (it != ctab.end()) {

		// There is an entry for this RG-ID in the colour table.
		glColor3fv(*it);

	} else glColor3fv(GPlatesGui::Colour::BLACK);
	CallVertexWithLine(line.begin(), line.end());
}


static void
PaintPoints()
{
	Layout::PointDataLayout::const_iterator 
		points_begin = Layout::PointDataLayoutBegin(),
		points_end   = Layout::PointDataLayoutEnd();

	glBegin(GL_POINTS);
		for_each(points_begin, points_end, PaintPointDataPos);
	glEnd();
}


static void
PaintLines()
{
	Layout::LineDataLayout::const_iterator 
		lines_begin = Layout::LineDataLayoutBegin(),
		lines_end   = Layout::LineDataLayoutEnd();
	
	for_each(lines_begin, lines_end, PaintLineDataPos);
}


void
GPlatesGui::Globe::SetNewHandlePos(const PointOnSphere &pos)
{
	_handle_pos = pos;
}


void
GPlatesGui::Globe::UpdateHandlePos(const PointOnSphere &pos)
{
	Rotation rot = CreateRotation(_handle_pos, pos);

	_cumul_rot = rot * _cumul_rot;
	_rev_cumul_rot = _cumul_rot.reverse();
	_handle_pos = pos;
}


PointOnSphere
GPlatesGui::Globe::Orient(const PointOnSphere &pos)
{
	return (_rev_cumul_rot * pos);
}


void
GPlatesGui::Globe::Paint()
{
	// NOTE: OpenGL rotations are *counter-clockwise* (API v1.4, p35).
	glPushMatrix();
		// rotate everything to get a nice almost-equatorial shot
//		glRotatef(-80.0, 1.0, 0.0, 0.0);

		UnitVector3D axis = _cumul_rot.axis();
		real_t angle_in_deg = radiansToDegrees(_cumul_rot.angle());
		glRotatef(angle_in_deg.dval(),
		           axis.x().dval(), axis.y().dval(), axis.z().dval());

		// Set the sphere's colour.
		glColor3fv(GPlatesGui::Colour(0.35, 0.35, 0.35));
		
		/*
		 * Draw sphere.
		 * DepthRange calls push the sphere back in the depth buffer
		 * a bit to avoid Z-fighting with the LineData.
		 */
		glDepthRange(0.1, 1.0);
		_sphere.Paint();

		// Set the grid's colour.
		glColor3fv(Colour::WHITE);
		
		/*
		 * Draw grid.
		 * DepthRange calls push the grid back in the depth buffer
		 * a bit to avoid Z-fighting with the LineData.
		 */
		glDepthRange(0.01, 1.0);
		_grid.Paint();

		// Restore DepthRange
		glDepthRange(0.0, 1.0);

		glPointSize(5.0f);
		
		/* 
		 * Paint the data.
		 */
		glColor3fv(GPlatesGui::Colour::GREEN);
		PaintPoints();
		
		glColor3fv(GPlatesGui::Colour::BLACK);
		PaintLines();

	glPopMatrix();
}
