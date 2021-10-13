/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>

#include "GLVertexElementBufferObject.h"

#include "GLRenderer.h"


GLenum
GPlatesOpenGL::GLVertexElementBufferObject::get_target_type()
{
	return GL_ELEMENT_ARRAY_BUFFER_ARB;
}


GPlatesOpenGL::GLVertexElementBufferObject::GLVertexElementBufferObject(
		const GLBufferObject::shared_ptr_type &buffer) :
	d_buffer(buffer)
{
	// We should only get here if the vertex buffer object extension is supported.
	GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_vertex_buffer_object),
			GPLATES_ASSERTION_SOURCE);
}


GPlatesOpenGL::GLBuffer::shared_ptr_to_const_type
GPlatesOpenGL::GLVertexElementBufferObject::get_buffer() const
{
	return d_buffer;
}


void
GPlatesOpenGL::GLVertexElementBufferObject::gl_bind(
		GLRenderer &renderer) const
{
	renderer.gl_bind_vertex_element_buffer_object(
			boost::dynamic_pointer_cast<const GLVertexElementBufferObject>(shared_from_this()));
}


void
GPlatesOpenGL::GLVertexElementBufferObject::gl_draw_range_elements(
		GLRenderer &renderer,
		GLenum mode,
		GLuint start,
		GLuint end,
		GLsizei count,
		GLenum type,
		GLint indices_offset) const
{
	// Use the overload that doesn't require a client memory pointer since we're using
	// the bound buffer object and *not* client memory.
	renderer.gl_draw_range_elements(mode, start, end, count, type, indices_offset);
}
