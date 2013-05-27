/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2013 The University of Sydney, Australia
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

//
// Fragment shader source code to render coloured (white) sphere with lighting.
//

uniform bool lighting_enabled;
uniform float light_ambient_contribution;
uniform vec3 world_space_light_direction;

// The world-space coordinates are interpolated across the geometry.
varying vec3 world_space_position;

void main (void)
{
	// The interpolated fragment colour.
	vec4 colour = gl_Color;

	if (lighting_enabled)
	{
		// Apply the Lambert diffuse lighting using the world-space position as the globe surface normal.
		// Note that neither the light direction nor the surface normal need be normalised.
		float lambert = lambert_diffuse_lighting(world_space_light_direction, world_space_position);

		// Blend between ambient and diffuse lighting.
		float lighting = mix_ambient_with_diffuse_lighting(lambert, light_ambient_contribution);

		colour.rgb *= lighting;
	}

	// The final fragment colour.
	gl_FragColor = colour;
}
