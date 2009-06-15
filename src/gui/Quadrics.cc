/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#include <cstdlib>
#include <iostream>
#include "Quadrics.h"
#include "OpenGLBadAllocException.h"

namespace
{
	// Handle GLU quadric errors
	GLvoid __CONVENTION__
	QuadricError()
	{

		// XXX: Not sure if glGetError returns GLU error codes as well
		// as GL codes.
		std::cerr
		 << "Quadric Error: "
		 << gluErrorString(glGetError())
		 << std::endl;

		exit(1);  // FIXME: should this be an exception instead?
	}
}


GPlatesGui::Quadrics::Quadrics()
{

	d_quadric = gluNewQuadric();
	if (d_quadric == 0) {

		// not enough memory to allocate object
		throw OpenGLBadAllocException(GPLATES_EXCEPTION_SOURCE,
		 "Not enough memory for OpenGL to create new quadric.");
	}
	// Previously, the type-parameter of the cast was 'void (*)()'.
	// On Mac OS X, the compiler complained, so it was changed to this.
	// Update: Fixed the prototype of the QuadricError callback function 
	// and removed the varargs ellipsis from the cast type.
#if 1
	gluQuadricCallback(d_quadric, GLU_ERROR, &QuadricError);
#else
	// A few OS X platforms need this instead - could be OS X 10.4 or
	// gcc 4.0.0 or PowerPC Macs ?
	gluQuadricCallback(d_quadric, GLU_ERROR,
		reinterpret_cast< GLvoid (__CONVENTION__ *)(...) >(&QuadricError));
#endif

}
