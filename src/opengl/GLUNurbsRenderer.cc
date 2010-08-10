/* $Id$ */

/**
 * @file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#include <iostream>
#include <utility>

#include <QDebug>
#include "boost/optional.hpp"

#include "GLUNurbsRenderer.h"

#include "GLCompositeDrawable.h"
#include "GLUNurbsRendererDrawable.h"
#include "OpenGLBadAllocException.h"

#include "maths/GenericVectorOps3D.h"
#include "maths/GreatCircleArc.h"
#include "maths/LatLonPoint.h"
#include "maths/MathsUtils.h"
#include "maths/Rotation.h"
#include "maths/UnitVector3D.h"
#include "maths/Vector3D.h"


namespace
{
	/**
	 *  We need different sampling tolerances for great circles and small circles.
	 *  Great circles will always appear quite big in GPlates, because we can't zoom
	 *  the globe out beyond 100%, so a sampling tolerance of 25 is OK; the curves will
	 *  still appear smooth.
	 *
	 *  For small circles this can make the circle appear to have jagged edges.
	 *  (Small circles are, after all, small).
	 *  So we use a lower sampling tolerance to ensure a smooth appearance.
	 */
	static const double GREAT_CIRCLE_SAMPLING_TOLERANCE = 25.;
	static const double SMALL_CIRCLE_SAMPLING_TOLERANCE = 5.;


	// Handle GLU NURBS errors
	GLvoid __CONVENTION__
	NurbsError()
	{
		// XXX I'm not sure if glGetError actually returns the GLU 
		// error codes as well as the GL codes.
		std::cerr
		 << "NURBS Error: "
		 << gluErrorString(glGetError())
		 << std::endl;

		exit(1);  // FIXME: should this be an exception instead?
	}
}


GPlatesOpenGL::GLUNurbsRenderer::GLUNurbsRenderer()
{
	// Increase the resolution so we get smoother curves:
	//  -  Even though it's the default, we force GLU_SAMPLING_METHOD
	//    to be GLU_PATH_LENGTH.
	d_current_parameters.sampling_method = GLU_PATH_LENGTH;
	//  -  The default GLU_SAMPLING_TOLERANCE is 50.0 pixels.  The OpenGL
	//    API notes that this is rather conservative, so we halve it.
	d_current_parameters.sampling_tolerance = GREAT_CIRCLE_SAMPLING_TOLERANCE;
}


void
GPlatesOpenGL::GLUNurbsRenderer::create_nurbs_obj()
{
	// Create a new 'GLUnurbsObj'.
	d_nurbs.reset(gluNewNurbsRenderer(), gluDeleteNurbsRenderer);
	if (d_nurbs.get() == 0)
	{
		// not enough memory to allocate object
		throw OpenGLBadAllocException(GPLATES_EXCEPTION_SOURCE,
		 "Not enough memory for OpenGL to create new NURBS renderer.");
	}

#if !defined(__APPLE__) || \
	(MAC_OSX_MAJOR_VERSION >= 10 && MAC_OSX_MINOR_VERSION >= 5) || \
	(CXX_MAJOR_VERSION >= 4 && CXX_MINOR_VERSION >= 2)
	// All non apple platforms use this path.
	// Also MacOS X using OS X version 10.5 and greater (or g++ 4.2 and greater)
	// use this code path.
	gluNurbsCallback(d_nurbs.get(), GLU_ERROR, &NurbsError);
#else
	// A few OS X platforms need this instead - could be OS X 10.4 or
	// gcc 4.0.0 or PowerPC Macs ?
	// Update: it seems after many installations on different Macs that
	// Mac OSX versions 10.4 using the default compiler g++ 4.0 require this code path.
	gluNurbsCallback(d_nurbs.get(), GLU_ERROR,
		reinterpret_cast< GLvoid (__CONVENTION__ *)(...) >(&NurbsError));
#endif
}


namespace
{
	/**
	 * Return the mid-point of the arc.
	 */
	GPlatesMaths::UnitVector3D
	mid_point_of(
			const GPlatesMaths::UnitVector3D &start,
			const GPlatesMaths::UnitVector3D &end)
	{
		const GPlatesMaths::Vector3D start_pt(start);
		const GPlatesMaths::Vector3D end_pt(end);

		return GPlatesMaths::Vector3D(start_pt + 0.5*(end_pt - start_pt)).get_normalisation();
	}


	typedef std::pair< GPlatesMaths::Vector3D, GLfloat > ControlPointData;


	/**
	 * Returns a Vector3D corresponding to the middle control point of the arc.
	 *
	 * The actual return values are determined thus:
	 *
	 *  - Draw a (straight) line from start_pt to end_pt.  This will be the base of a triangle.
	 *  - Using this line, create an isoceles triangle with both base angles equal to half of 
	 *    the angular extent of the arc traced out by start_pt and end_pt (i.e. the base angle
	 *    is half of  acos( dot( start_pt, end_pt ) ).)
	 *  - The top of this triangle is the location of the control point we want.
	 *  - The weight of the control point is  cos( base_angle ).
	 */
	ControlPointData
	calc_great_circle_arc_control_point_data(
			const GPlatesMaths::Vector3D &start_pt,
			const GPlatesMaths::Vector3D &end_pt)
	{
		const GPlatesMaths::Vector3D arc_direction = end_pt - start_pt;
		GPlatesMaths::Vector3D triangle_base_mid = start_pt + 0.5*arc_direction;

		double triangle_base_angle = 0.5 * std::acos(GPlatesMaths::GenericVectorOps3D::dot(start_pt, end_pt).dval());
		double triangle_height = 0.5 * arc_direction.magnitude().dval() * std::tan(triangle_base_angle);

		GLfloat weight = static_cast<GLfloat>(std::cos(triangle_base_angle));

		GPlatesMaths::Vector3D triangle_tip = triangle_base_mid + triangle_height * triangle_base_mid.get_normalisation();
		return std::make_pair(triangle_tip, weight);
	}
	
	/**
	 * Returns the control point and weight required for a small circle arc.        
	 * 
	 * The algorithm is similar to that for the great circle                                                              
	 */
	ControlPointData
	calculate_small_circle_arc_control_point_data(
		const GPlatesMaths::UnitVector3D &centre_pt,
		const GPlatesMaths::UnitVector3D &start_pt,
		const GPlatesMaths::UnitVector3D &end_pt,
		const GPlatesMaths::Real &arc_length_in_radians)
	{
		using namespace GPlatesMaths;
		// Get the angle subtended at the centre of the globe by the centre point and 
		// one of the end points. This gives us the radius angle of the small circle.
		double cosangle = dot(centre_pt,start_pt).dval();
		
		// centre_in_plane is the small circle centre in the plane of the small circle.
		Vector3D centre_in_plane(centre_pt*cosangle);
		
		Vector3D start_to_end(Vector3D(end_pt)- Vector3D(start_pt));		
		double half_base = 0.5*start_to_end.magnitude().dval();		
		Vector3D midpoint = Vector3D(start_pt) + 0.5*start_to_end;
		
		Vector3D direction_of_amplitude = midpoint - centre_in_plane;
		UnitVector3D direction = direction_of_amplitude.get_normalisation();
		
		// The angle at base of the isosceles triangle.
		double triangle_base_angle = 0.5*arc_length_in_radians.dval();
		double amplitude_of_triangle = half_base*tan(triangle_base_angle);
		
		Vector3D amplitude_vector(direction);
		amplitude_vector = amplitude_of_triangle*amplitude_vector;
		
		Vector3D control_point = midpoint+amplitude_vector;
		
		GLfloat weight = static_cast<GLfloat>(std::cos(triangle_base_angle));

		return std::make_pair(control_point,weight);
	}
}


GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesOpenGL::GLUNurbsRenderer::draw_curve(
		GLint num_knots,
		const boost::shared_array<GLfloat> &knots,
		GLint stride,
		const boost::shared_array<GLfloat> &ctrl_pts,
		GLint order,
		GLenum curve_type,
		const GPlatesGui::Colour &colour)
{
	// Create GLUnurbsObj if it hasn't been created yet.
	//
	// We do this here because this is a draw call and we know that
	// the OpenGL context is current and hence creation of a
	// GLUnurbsObj should succeed.
	if (d_nurbs.get() == NULL)
	{
		create_nurbs_obj();
	}

	boost::shared_ptr<GLUNurbsGeometry> curve(
			new GLUNurbsCurve(num_knots, knots, stride, ctrl_pts, order, curve_type));

	return GLUNurbsRendererDrawable::create(d_nurbs, curve, d_current_parameters, colour);
}


GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesOpenGL::GLUNurbsRenderer::draw_great_circle_arc(
		const GPlatesMaths::PointOnSphere &start,
		const GPlatesMaths::PointOnSphere &end,
		const GPlatesGui::Colour &colour)
{
	const GPlatesMaths::UnitVector3D &start_pt = start.position_vector();
	const GPlatesMaths::UnitVector3D &end_pt = end.position_vector();

	const GPlatesMaths::real_t dot_of_endpoints = dot(start_pt, end_pt);

	return draw_great_circle_arc(start_pt, end_pt, dot_of_endpoints, colour);
}


GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesOpenGL::GLUNurbsRenderer::draw_great_circle_arc(
		const GPlatesMaths::GreatCircleArc &arc,
		const GPlatesGui::Colour &colour)
{
	const GPlatesMaths::UnitVector3D &start_pt = arc.start_point().position_vector();
	const GPlatesMaths::UnitVector3D &end_pt = arc.end_point().position_vector();

	const GPlatesMaths::real_t dot_of_endpoints = dot(start_pt, end_pt);

	return draw_great_circle_arc(start_pt, end_pt, dot_of_endpoints, colour);
}


GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesOpenGL::GLUNurbsRenderer::draw_great_circle_arc(
		const GPlatesMaths::UnitVector3D &start_pt,
		const GPlatesMaths::UnitVector3D &end_pt,
		const GPlatesMaths::real_t &dot_of_endpoints,
		const GPlatesGui::Colour &colour)
{
	if (dot_of_endpoints < 0.0) {
		// arc is bigger than 90 degrees.
		GPlatesMaths::UnitVector3D mid_pt = mid_point_of(start_pt, end_pt);

		GLCompositeDrawable::non_null_ptr_type composite_drawable = GLCompositeDrawable::create();

		// A great circle arc is always less than 180 degress, so if we split it
		// into two pieces, we definitely get two arcs of less than 90 degress.
		composite_drawable->add_drawable(
				draw_great_circle_arc_smaller_than_ninety_degrees(start_pt, mid_pt, colour));

		composite_drawable->add_drawable(
				draw_great_circle_arc_smaller_than_ninety_degrees(mid_pt, end_pt, colour));

		return composite_drawable;
	} else {
		return draw_great_circle_arc_smaller_than_ninety_degrees(start_pt, end_pt, colour);
	}
}


GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesOpenGL::GLUNurbsRenderer::draw_great_circle_arc_smaller_than_ninety_degrees(
		const GPlatesMaths::UnitVector3D &start_pt,
		const GPlatesMaths::UnitVector3D &end_pt,
		const GPlatesGui::Colour &colour)
{
	static const GLint STRIDE = 4;
	static const GLint ORDER = 3;
	static const GLsizei NUM_CONTROL_POINTS = 3;
	static const GLsizei KNOT_SIZE = 6;
	boost::shared_array<GLfloat> KNOTS(new GLfloat[KNOT_SIZE]);
	KNOTS[0] = KNOTS[1] = KNOTS[2] = 0.0f;
	KNOTS[3] = KNOTS[4] = KNOTS[5] = 1.0f;

	ControlPointData mid_ctrl_pt_data = calc_great_circle_arc_control_point_data(
			GPlatesMaths::Vector3D(start_pt), 
			GPlatesMaths::Vector3D(end_pt));

	const GPlatesMaths::Vector3D &mid_ctrl_pt = mid_ctrl_pt_data.first;
	const GLfloat &weight = mid_ctrl_pt_data.second;

	boost::shared_array<GLfloat> ctrl_points(new GLfloat[NUM_CONTROL_POINTS * STRIDE]);
	ctrl_points[0 * STRIDE + 0] = static_cast<GLfloat>(start_pt.x().dval());
	ctrl_points[0 * STRIDE + 1] = static_cast<GLfloat>(start_pt.y().dval());
	ctrl_points[0 * STRIDE + 2] = static_cast<GLfloat>(start_pt.z().dval());
	ctrl_points[0 * STRIDE + 3] = 1.0f;
	ctrl_points[1 * STRIDE + 0] = static_cast<GLfloat>(weight*mid_ctrl_pt.x().dval());
	ctrl_points[1 * STRIDE + 1] = static_cast<GLfloat>(weight*mid_ctrl_pt.y().dval());
	ctrl_points[1 * STRIDE + 2] = static_cast<GLfloat>(weight*mid_ctrl_pt.z().dval());
	ctrl_points[1 * STRIDE + 3] = static_cast<GLfloat>(weight);
	ctrl_points[2 * STRIDE + 0] = static_cast<GLfloat>(end_pt.x().dval());
	ctrl_points[2 * STRIDE + 1] = static_cast<GLfloat>(end_pt.y().dval());
	ctrl_points[2 * STRIDE + 2] = static_cast<GLfloat>(end_pt.z().dval());
	ctrl_points[2 * STRIDE + 3] = 1.0f;

	return draw_curve(KNOT_SIZE, KNOTS, STRIDE,
			ctrl_points, ORDER, GL_MAP1_VERTEX_4, colour);
}

GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesOpenGL::GLUNurbsRenderer::draw_small_circle(
	const GPlatesMaths::PointOnSphere &point_on_sphere,
	const GPlatesMaths::Real &radius_in_radians,
	const GPlatesGui::Colour &colour)
{
	using namespace GPlatesMaths;
	
	const UnitVector3D axis = point_on_sphere.position_vector();
	const Real cos_colat = cos(radius_in_radians);
	
	return draw_small_circle(axis, cos_colat, colour);
}


GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesOpenGL::GLUNurbsRenderer::draw_small_circle_arc(
	const GPlatesMaths::PointOnSphere &centre,
	const GPlatesMaths::PointOnSphere &first_point_on_arc,
	const GPlatesMaths::Real &arc_length_in_radians,
	const GPlatesGui::Colour &colour)
{
	GPlatesMaths::UnitVector3D uv_centre = centre.position_vector();
	GPlatesMaths::UnitVector3D uv_point_on_arc = first_point_on_arc.position_vector();

	GLCompositeDrawable::non_null_ptr_type composite_drawable = GLCompositeDrawable::create();

	double remaining_radians = arc_length_in_radians.dval();
	while (remaining_radians > GPlatesMaths::HALF_PI)
	{
		composite_drawable->add_drawable(
				draw_small_circle_arc_smaller_than_or_equal_to_ninety_degrees(
						uv_centre, uv_point_on_arc, GPlatesMaths::HALF_PI, colour));
		
		GPlatesMaths::Rotation rot = GPlatesMaths::Rotation::create(
			uv_centre,
			GPlatesMaths::HALF_PI);
			
		uv_point_on_arc = rot*uv_point_on_arc;
		
		remaining_radians -= GPlatesMaths::HALF_PI;		
	}

	composite_drawable->add_drawable(
			draw_small_circle_arc_smaller_than_or_equal_to_ninety_degrees(
					uv_centre, uv_point_on_arc, remaining_radians, colour));

	return composite_drawable;

}

GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesOpenGL::GLUNurbsRenderer::draw_small_circle_arc_smaller_than_or_equal_to_ninety_degrees(
	const GPlatesMaths::UnitVector3D &centre_pt,
	const GPlatesMaths::UnitVector3D &start_pt,
	const GPlatesMaths::Real &arc_length_in_radians,
	const GPlatesGui::Colour &colour)
{

	GPlatesMaths::Rotation rot = GPlatesMaths::Rotation::create(
			centre_pt,
			arc_length_in_radians);

	GPlatesMaths::UnitVector3D end_pt = rot*start_pt;

// Use the same knots etc as the great circle arc routine.
	static const GLint STRIDE = 4;
	static const GLint ORDER = 3;
	static const GLsizei NUM_CONTROL_POINTS = 3;
	static const GLsizei KNOT_SIZE = 6;
	boost::shared_array<GLfloat> KNOTS(new GLfloat[KNOT_SIZE]);
	KNOTS[0] = KNOTS[1] = KNOTS[2] = 0.0f;
	KNOTS[3] = KNOTS[4] = KNOTS[5] = 1.0f;
	
	ControlPointData control_point_data = calculate_small_circle_arc_control_point_data(
											centre_pt,
											start_pt,
											end_pt,
											arc_length_in_radians);
											
	const GPlatesMaths::Vector3D &mid_ctrl_pt = control_point_data.first;
	const GLfloat &weight = control_point_data.second;

	boost::shared_array<GLfloat> ctrl_points(new GLfloat[NUM_CONTROL_POINTS * STRIDE]);
	ctrl_points[0 * STRIDE + 0] = static_cast<GLfloat>(start_pt.x().dval());
	ctrl_points[0 * STRIDE + 1] = static_cast<GLfloat>(start_pt.y().dval());
	ctrl_points[0 * STRIDE + 2] = static_cast<GLfloat>(start_pt.z().dval());
	ctrl_points[0 * STRIDE + 3] = 1.0f;
	ctrl_points[1 * STRIDE + 0] = static_cast<GLfloat>(weight*mid_ctrl_pt.x().dval());
	ctrl_points[1 * STRIDE + 1] = static_cast<GLfloat>(weight*mid_ctrl_pt.y().dval());
	ctrl_points[1 * STRIDE + 2] = static_cast<GLfloat>(weight*mid_ctrl_pt.z().dval());
	ctrl_points[1 * STRIDE + 3] = static_cast<GLfloat>(weight);
	ctrl_points[2 * STRIDE + 0] = static_cast<GLfloat>(end_pt.x().dval());
	ctrl_points[2 * STRIDE + 1] = static_cast<GLfloat>(end_pt.y().dval());
	ctrl_points[2 * STRIDE + 2] = static_cast<GLfloat>(end_pt.z().dval());
	ctrl_points[2 * STRIDE + 3] = 1.0f;

	d_current_parameters.sampling_tolerance = SMALL_CIRCLE_SAMPLING_TOLERANCE;
	GLDrawable::non_null_ptr_to_const_type drawable = draw_curve(
			KNOT_SIZE, KNOTS, STRIDE, ctrl_points, ORDER, GL_MAP1_VERTEX_4, colour);
	d_current_parameters.sampling_tolerance = GREAT_CIRCLE_SAMPLING_TOLERANCE;

	return drawable;

}

GPlatesOpenGL::GLDrawable::non_null_ptr_to_const_type
GPlatesOpenGL::GLUNurbsRenderer::draw_small_circle(
	const GPlatesMaths::UnitVector3D &axis, 
	const GPlatesMaths::Real &cos_colatitude,
	const GPlatesGui::Colour &colour)
{
	using namespace GPlatesMaths;

	/**
	*  The following weight, stride, order, and knots are taken from SphericalGrid.cc
	*/
	static const GLfloat WEIGHT = static_cast<GLfloat>(1.0 / sqrt(2.0));
	static const GLint STRIDE = 4;
	static const GLint ORDER = 3;
	static const GLsizei NUM_CONTROL_POINTS = 9;
	static const GLsizei KNOT_SIZE = 12;
	boost::shared_array<GLfloat> KNOTS(new GLfloat[KNOT_SIZE]);
	KNOTS[0] = KNOTS[1] = KNOTS[2] = 0.0f;
	KNOTS[3] = KNOTS[4] = 0.25f;
	KNOTS[5] = KNOTS[6] = 0.5f;
	KNOTS[7] = KNOTS[8] = 0.75f;
	KNOTS[9] = KNOTS[10] = KNOTS[11] = 1.0f;

	// Define the height, radius, and control points as they would be defined
	// for a line of latitude, then rotate them according to the llp of the 
	// centre of the small circle. 

	// The rotation axis for a rotation from the north pole to the centre of the small circle.
	// We could check here for an axis which is coincident with the north or south poles;
	// if it's a north pole we wouldn't need to rotate, we could just draw it as a line-of-latitude; 
	// if it's a south pole we could draw it as line-of-latitude using (1-cos_colat) as the cos of the co_lat.
	boost::optional<Rotation> rotation;
	
	double cos_colat_to_use = cos_colatitude.dval();
	
	if (axis.z() == 1)
	{
		// do nothing here...so we can re-factor the logic. 
	}
	else if (axis.z() == -1)
	{
		// South pole => reverse the z-axis and draw it as an (unrotated) line-of-latitude.
		cos_colat_to_use = -cos_colatitude.dval();
	}
	else
	{
		rotation.reset(Rotation::create(UnitVector3D(0,0,1),axis));	
	}
	



	// Set up control points for drawing a line of latitude.
	/*
	* We want to draw a small circle around the z-axis.
	* Calculate the height (above z = 0) and radius of this circle.
	*/
	GLfloat height = static_cast<GLfloat>(cos_colat_to_use);
	GLfloat radius = static_cast<GLfloat>(sqrt(1.-cos_colat_to_use*cos_colat_to_use));

	GLfloat u_radius = WEIGHT * radius;
	GLfloat u_height = WEIGHT * height;

	// From "right", going clockwise.
	std::vector<Vector3D> control_vectors;

	Vector3D v1(radius,0.,height);
	Vector3D v2(u_radius,u_radius,u_height);
	Vector3D v3(0.,radius,height);
	Vector3D v4(-u_radius,u_radius,u_height);
	Vector3D v5(-radius,0.,height);
	Vector3D v6(-u_radius,-u_radius,u_height);
	Vector3D v7(0.,-radius,height);
	Vector3D v8(u_radius,-u_radius,u_height);
	Vector3D v9(radius,0.,height);	

	control_vectors.push_back(v1);
	control_vectors.push_back(v2);
	control_vectors.push_back(v3);
	control_vectors.push_back(v4);
	control_vectors.push_back(v5);
	control_vectors.push_back(v6);
	control_vectors.push_back(v7);
	control_vectors.push_back(v8);
	control_vectors.push_back(v9);	

	// If we're at neither the north nor south poles, then we have to rotate the control points. 
	if (rotation)
	{	
		std::vector<Vector3D>::iterator
			it = control_vectors.begin(),
			end = control_vectors.end();

		// Not all the control points will lie on the surface of the sphere, hence they've been
		// defined as Vector3Ds instead of UnitVector3Ds. To rotate the vectors, they need to be unit vectors, 
		// so normalise them before rotation, and scale up to their former magnitude after rotation.
		for (; it != end ; ++it)
		{
			Real mag = it->magnitude();
			UnitVector3D uv(it->get_normalisation());
			uv  = (*rotation)*uv;
			Vector3D v(uv);
			*it = v*mag;
		}	
	}

	boost::shared_array<GLfloat> ctrl_points(new GLfloat[NUM_CONTROL_POINTS * STRIDE]);
	for (unsigned int n = 0; n < 4; ++n)
	{
		const unsigned int even_index = 2 * n;
		ctrl_points[even_index * STRIDE + 0] = GLfloat(control_vectors[even_index].x().dval());
		ctrl_points[even_index * STRIDE + 1] = GLfloat(control_vectors[even_index].y().dval());
		ctrl_points[even_index * STRIDE + 2] = GLfloat(control_vectors[even_index].z().dval());
		ctrl_points[even_index * STRIDE + 3] = 1.0f;

		const unsigned int odd_index = even_index + 1;
		ctrl_points[odd_index * STRIDE + 0] = GLfloat(control_vectors[odd_index].x().dval());
		ctrl_points[odd_index * STRIDE + 1] = GLfloat(control_vectors[odd_index].y().dval());
		ctrl_points[odd_index * STRIDE + 2] = GLfloat(control_vectors[odd_index].z().dval());
		ctrl_points[odd_index * STRIDE + 3] = WEIGHT;
	}
	ctrl_points[8 * STRIDE + 0] = GLfloat(control_vectors[8].x().dval());
	ctrl_points[8 * STRIDE + 1] = GLfloat(control_vectors[8].y().dval());
	ctrl_points[8 * STRIDE + 2] = GLfloat(control_vectors[8].z().dval());
	ctrl_points[8 * STRIDE + 3] = 1.0f;


	// Use a smaller sampling tolerance for small circles, so that they appear smooth
	// even when small.
	d_current_parameters.sampling_tolerance = SMALL_CIRCLE_SAMPLING_TOLERANCE;
	GLDrawable::non_null_ptr_to_const_type drawable = draw_curve(
			KNOT_SIZE, KNOTS, STRIDE, ctrl_points, ORDER, GL_MAP1_VERTEX_4, colour);
	d_current_parameters.sampling_tolerance = GREAT_CIRCLE_SAMPLING_TOLERANCE;

	return drawable;
}


GPlatesOpenGL::GLUNurbsRenderer::Parameters::Parameters() :
	sampling_method(GLU_PATH_LENGTH),
	sampling_tolerance(50.0f)
{
}