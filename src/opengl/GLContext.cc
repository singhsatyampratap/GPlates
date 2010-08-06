/* $Id$ */
 
/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2010 The University of Sydney, Australia
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

#include <cstddef> // For std::size_t
#include <iostream>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLContext.h"

#include "global/CompilerWarnings.h"
#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"


bool GPlatesOpenGL::GLContext::s_initialised_GLEW = false;
GLint GPlatesOpenGL::GLContext::s_max_texture_units = 1;


// We use macros in <GL/glew.h> that contain old-style casts.
DISABLE_GCC_WARNING("-Wold-style-cast")

void
GPlatesOpenGL::GLContext::initialise()
{
	// Currently we only initialise once for the whole application instead of
	// once for each rendering context.
	// This is because the GLEW library would need to be compiled with the GLEW_MX
	// flag (see http://glew.sourceforge.net/advanced.html under multiple rendering contexts)
	// and this does not appear to be supported in all package managers (eg, linux and MacOS X).
	// There's not much information on whether we need one if we share contexts in Qt but
	// this is the assumption here.
	if (!s_initialised_GLEW)
	{
		GLenum err = glewInit();
		if (GLEW_OK != err)
		{
			// glewInit failed.
			//
			// We'll assume all calls to test whether an extension is available
			// (such as "if (GLEW_ARB_multitexture) ..." will fail since they just
			// test boolean variables which are assumed to be initialised by GLEW to zero.
			// This just means we will be forced to fall back to OpenGL version 1.1.
			qWarning() << "Error: " << reinterpret_cast<const char *>(glewGetErrorString(err));
		}
		qDebug() << "Status: Using GLEW " << reinterpret_cast<const char *>(glewGetString(GLEW_VERSION));

		s_initialised_GLEW = true;

		// Get the maximum number of texture units supported.
		if (GLEW_ARB_multitexture)
		{
			glGetIntegerv(GL_MAX_TEXTURE_UNITS_ARB, &s_max_texture_units);
		}
	}
}

ENABLE_GCC_WARNING("-Wold-style-cast")


std::size_t
GPlatesOpenGL::GLContext::get_max_texture_units_ARB()
{
	// GLEW must have been initialised.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			s_initialised_GLEW,
			GPLATES_ASSERTION_SOURCE);

	return s_max_texture_units;
}


GLenum
GPlatesOpenGL::GLContext::get_GL_TEXTURE0_ARB()
{
	return GL_TEXTURE0_ARB;
}


GPlatesOpenGL::GLContext::SharedState::SharedState() :
	d_texture_resource_manager(GLTextureResourceManager::create())
{
}
