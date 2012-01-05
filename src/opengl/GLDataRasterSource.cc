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

#include <boost/cast.hpp>
/*
 * The OpenGL Extension Wrangler Library (GLEW).
 * Must be included before the OpenGL headers (which also means before Qt headers).
 * For this reason it's best to try and include it in ".cc" files only.
 */
#include <GL/glew.h>
#include <opengl/OpenGL.h>
#include <QDebug>

#include "GLDataRasterSource.h"
#include "GLContext.h"
#include "GLTextureUtils.h"
#include "GLUtils.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "property-values/ProxiedRasterResolver.h"
#include "property-values/RawRasterUtils.h"

#include "utils/Base2Utils.h"
#include "utils/Profile.h"


boost::optional<GPlatesOpenGL::GLDataRasterSource::non_null_ptr_type>
GPlatesOpenGL::GLDataRasterSource::create(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &data_raster,
		unsigned int tile_texel_dimension)
{
	boost::optional<GPlatesPropertyValues::ProxiedRasterResolver::non_null_ptr_type> proxy_resolver_opt =
			GPlatesPropertyValues::ProxiedRasterResolver::create(data_raster);
	if (!proxy_resolver_opt)
	{
		return boost::none;
	}

	// Get the raster dimensions.
	boost::optional<std::pair<unsigned int, unsigned int> > raster_dimensions =
			GPlatesPropertyValues::RawRasterUtils::get_raster_size(*data_raster);

	// If raster happens to be uninitialised then return false.
	if (!raster_dimensions)
	{
		return boost::none;
	}

	const unsigned int raster_width = raster_dimensions->first;
	const unsigned int raster_height = raster_dimensions->second;

	// Make sure our tile size does not exceed the maximum texture size...
	if (tile_texel_dimension > GLContext::get_parameters().texture.gl_max_texture_size)
	{
		tile_texel_dimension = GLContext::get_parameters().texture.gl_max_texture_size;
	}

	// Make sure tile_texel_dimension is a power-of-two.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			tile_texel_dimension > 0 && GPlatesUtils::Base2::is_power_of_two(tile_texel_dimension),
			GPLATES_ASSERTION_SOURCE);

	return non_null_ptr_type(new GLDataRasterSource(
			proxy_resolver_opt.get(),
			raster_width,
			raster_height,
			tile_texel_dimension));
}


GPlatesOpenGL::GLDataRasterSource::GLDataRasterSource(
		const GPlatesGlobal::PointerTraits<GPlatesPropertyValues::ProxiedRasterResolver>::non_null_ptr_type &
				proxy_raster_resolver,
		unsigned int raster_width,
		unsigned int raster_height,
		unsigned int tile_texel_dimension) :
	d_proxied_raster_resolver(proxy_raster_resolver),
	d_raster_width(raster_width),
	d_raster_height(raster_height),
	d_tile_texel_dimension(tile_texel_dimension),
	d_tile_pack_working_space(
			new float[
					GPLATES_OPENGL_BOOL(GLEW_ARB_texture_rg)
					// RG format...
					? 2 * tile_texel_dimension * tile_texel_dimension
					// RGBA format...
					: 4 * tile_texel_dimension * tile_texel_dimension]),
	d_logged_tile_load_failure_warning(false)
{
	// Floating-point textures must be supported.
	GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
			GPLATES_OPENGL_BOOL(GLEW_ARB_texture_float),
			GPLATES_ASSERTION_SOURCE);
}


GLint
GPlatesOpenGL::GLDataRasterSource::get_target_texture_internal_format() const
{
	// We use RG format where possible since it saves memory.
	// NOTE: Otherwise we use RGBA (instead of RGB) because hardware typically uses
	// four channels for RGB formats anyway and uploading to the hardware should be faster
	// since driver doesn't need to be involved (consuming CPU cycles to convert RGB to RGBA).
	return GPLATES_OPENGL_BOOL(GLEW_ARB_texture_rg) ? GL_RG32F : GL_RGBA32F_ARB;
}


GPlatesOpenGL::GLDataRasterSource::cache_handle_type
GPlatesOpenGL::GLDataRasterSource::load_tile(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GLRenderer &renderer)
{
	PROFILE_BEGIN(profile_proxy_raster_data, "GLDataRasterSource: get_region_from_level");
	// Get the region of the raster covered by this tile at the level-of-detail of this tile.
	boost::optional<GPlatesPropertyValues::RawRaster::non_null_ptr_type> raster_region_opt =
			d_proxied_raster_resolver->get_region_from_level(
					level,
					texel_x_offset,
					texel_y_offset,
					texel_width,
					texel_height);
	PROFILE_END(profile_proxy_raster_data);

	PROFILE_BEGIN(profile_proxy_raster_coverage, "GLDataRasterSource: get_coverage_from_level");
	// Get the region of the raster covered by this tile at the level-of-detail of this tile.
	boost::optional<GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type> raster_coverage_opt =
			d_proxied_raster_resolver->get_coverage_from_level(
					level,
					texel_x_offset,
					texel_y_offset,
					texel_width,
					texel_height);
	PROFILE_END(profile_proxy_raster_coverage);

	// If there was an error accessing raster data, or coverage, then zero the raster data/coverage values.
	if (!raster_region_opt || !raster_coverage_opt)
	{
		handle_error_loading_source_raster(
				level,
				texel_x_offset,
				texel_y_offset,
				texel_width,
				texel_height,
				target_texture,
				renderer);

		// Nothing needs caching.
		return cache_handle_type();
	}

	// Pack the raster data/coverage values into the target texture.
	// This will fail if the raster is not a floating-point raster.
	if (!pack_raster_data_into_tile_working_space(
			raster_region_opt.get(),
			raster_coverage_opt.get(),
			texel_width,
			texel_height))
	{
		handle_error_loading_source_raster(
				level,
				texel_x_offset,
				texel_y_offset,
				texel_width,
				texel_height,
				target_texture,
				renderer);

		// Nothing needs caching.
		return cache_handle_type();
	}

	// Load the packed data into the texture.
	if (GLEW_ARB_texture_rg)
	{
		// Use RG-only format to pack raster data/coverage values.
		GLTextureUtils::load_image_into_texture_2D(
				renderer,
				target_texture,
				d_tile_pack_working_space.get(),
				GL_RG,
				GL_FLOAT,
				texel_width,
				texel_height);
	}
	else
	{
		// Use RGBA format to pack raster data/coverage values.
		GLTextureUtils::load_image_into_texture_2D(
				renderer,
				target_texture,
				d_tile_pack_working_space.get(),
				GL_RGBA,
				GL_FLOAT,
				texel_width,
				texel_height);
	}

	// Nothing needs caching.
	return cache_handle_type();
}


void
GPlatesOpenGL::GLDataRasterSource::handle_error_loading_source_raster(
		unsigned int level,
		unsigned int texel_x_offset,
		unsigned int texel_y_offset,
		unsigned int texel_width,
		unsigned int texel_height,
		const GLTexture::shared_ptr_type &target_texture,
		GLRenderer &renderer)
{
	if (!d_logged_tile_load_failure_warning)
	{
		qWarning() << "Unable to load floating-point data/coverage data into raster tile:";

		qWarning() << "  level, texel_x_offset, texel_y_offset, texel_width, texel_height: "
				<< level << ", "
				<< texel_x_offset << ", "
				<< texel_y_offset << ", "
				<< texel_width << ", "
				<< texel_height << ", ";

		d_logged_tile_load_failure_warning = true;
	}

	// Set the data/coverage values to zero for all pixels.
	if (GLEW_ARB_texture_rg)
	{
		// Use RG-only format.
		GLTextureUtils::fill_float_texture_2D(
				renderer, target_texture, 0.0f, 0.0f, GL_RG, texel_width, texel_height);
	}
	else
	{
		// Use RGBA format.
		GLTextureUtils::fill_float_texture_2D(
				renderer, target_texture, 0.0f, 0.0f, 0.0f, 0.0f, texel_width, texel_height);
	}
}


template <typename RealType>
void
GPlatesOpenGL::GLDataRasterSource::pack_raster_data_into_tile_working_space(
		const RealType *const region_data,
		const float *const coverage_data,
		unsigned int texel_width,
		unsigned int texel_height)
{
	float *tile_pack_working_space = d_tile_pack_working_space.get();
	const unsigned int num_texels = texel_width * texel_height;

	if (GLEW_ARB_texture_rg)
	{
		// Use RG-only format to pack raster data/coverage values.
		for (unsigned int texel = 0; texel < num_texels; ++texel)
		{
			// If we've sampled outside the coverage then we have no valid data value so
			// just set it a valid floating-point value (ie, not NAN) so that the graphics hardware
			// can still do valid operations on it - this makes it easy to do coverage-weighted
			// calculations in order to avoid having to explicitly check that a texel coverage
			// value is zero (multiplying by zero effectively disables it but multiplying zero
			// will give NAN instead of zero). An example of this kind of calculation is
			// mean which is M = sum(Ci * Xi) / sum(Ci) where Ci is coverage and Xi is data value.
			const float coverage_data_texel = coverage_data[texel];
			const RealType region_data_texel = (coverage_data_texel > 0) ? region_data[texel] : 0;

			// Distribute the data/coverage values into the red/green channels.
			tile_pack_working_space[0] = static_cast<GLfloat>(region_data_texel);
			tile_pack_working_space[1] = coverage_data_texel;

			tile_pack_working_space += 2;
		}
	}
	else
	{
		// Use RGBA format to pack raster data/coverage values.
		for (unsigned int texel = 0; texel < num_texels; ++texel)
		{
			// See comment above.
			const float coverage_data_texel = coverage_data[texel];
			const RealType region_data_texel = (coverage_data_texel > 0) ? region_data[texel] : 0;

			// Distribute the data/coverage values into the red/green channels.
			tile_pack_working_space[0] = static_cast<GLfloat>(region_data_texel);
			tile_pack_working_space[1] = coverage_data_texel;
			tile_pack_working_space[2] = 0.0f; // This channel unused/ignored.
			tile_pack_working_space[3] = 0.0f; // This channel unused/ignored.

			tile_pack_working_space += 4;
		}
	}
}


bool
GPlatesOpenGL::GLDataRasterSource::pack_raster_data_into_tile_working_space(
		const GPlatesPropertyValues::RawRaster::non_null_ptr_type &raster_region,
		const GPlatesPropertyValues::CoverageRawRaster::non_null_ptr_type &raster_coverage,
		unsigned int texel_width,
		unsigned int texel_height)
{
	boost::optional<GPlatesPropertyValues::FloatRawRaster::non_null_ptr_type> float_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::FloatRawRaster>(*raster_region);
	if (float_region_tile)
	{
		pack_raster_data_into_tile_working_space(
				float_region_tile.get()->data(),
				raster_coverage->data(),
				texel_width,
				texel_height);

		return true;
	}

	boost::optional<GPlatesPropertyValues::DoubleRawRaster::non_null_ptr_type> double_region_tile =
			GPlatesPropertyValues::RawRasterUtils::try_raster_cast<
					GPlatesPropertyValues::DoubleRawRaster>(*raster_region);
	if (double_region_tile)
	{
		pack_raster_data_into_tile_working_space(
				double_region_tile.get()->data(),
				raster_coverage->data(),
				texel_width,
				texel_height);

		return true;
	}

	return false;
}