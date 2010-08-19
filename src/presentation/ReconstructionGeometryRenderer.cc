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

#include "ReconstructionGeometryRenderer.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructedVirtualGeomagneticPole.h"
#include "app-logic/ResolvedRaster.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/PlateVelocityUtils.h"
#include "app-logic//VGPRenderSettings.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"
#include "global/NotYetImplementedException.h"

#include "gui/Colour.h"

#include "presentation/ViewState.h"

#include "maths/CalculateVelocity.h"
#include "maths/MathsUtils.h"

#include "view-operations/RenderedGeometryFactory.h"
#include "view-operations/RenderedGeometryLayer.h"
#include "view-operations/RenderedGeometryParameters.h"


namespace
{
	/**
	 * Creates a @a RenderedGeometry from @a geometry and wraps it in another @a RenderedGeometry
	 * that references @a reconstruction_geometry.
	 */
	GPlatesViewOperations::RenderedGeometry
	create_rendered_reconstruction_geometry(
			const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
			const GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type &reconstruction_geometry,
			const GPlatesPresentation::ReconstructionGeometryRenderer::StyleParams &style_params,
			const boost::optional<GPlatesGui::Colour> &colour,
			const boost::optional<GPlatesMaths::Rotation> &rotation = boost::none)
	{
		// Create a RenderedGeometry for drawing the reconstructed geometry.
		// Draw it in the specified colour (if specified) otherwise defer colouring to a later time
		// using ColourProxy.
		GPlatesViewOperations::RenderedGeometry rendered_geom =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_geometry_on_sphere(
						rotation ? rotation.get() * geometry : geometry,
						colour ? colour.get() : GPlatesGui::ColourProxy(reconstruction_geometry),
						style_params.reconstruction_point_size_hint,
						style_params.reconstruction_line_width_hint);

		// Create a RenderedGeometry for storing the ReconstructionGeometry and
		// a RenderedGeometry associated with it.
		return GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
				reconstruction_geometry,
				rendered_geom);
	}


	/**
	 * Get the reconstruction geometries that are resolved topological networks and
	 * draw the velocities at the network points if there are any.
	 */
	void
	render_topological_network_velocities(
			const GPlatesAppLogic::ResolvedTopologicalNetwork::non_null_ptr_to_const_type &topological_network,
			GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
			const GPlatesPresentation::ReconstructionGeometryRenderer::StyleParams &style_params,
			const boost::optional<GPlatesGui::Colour> &colour)
	{
		const GPlatesAppLogic::PlateVelocityUtils::TopologicalNetworkVelocities &network_velocites =
				topological_network->get_network_velocities();

		if (!network_velocites.contains_velocities())
		{
			return;
		}

		const GPlatesGui::Colour &velocity_colour = colour
				? colour.get()
				: GPlatesGui::Colour::get_white();

		// Get the velocities at the network points.
		std::vector<GPlatesMaths::PointOnSphere> network_points;
		std::vector<GPlatesMaths::VectorColatitudeLongitude> network_velocities;
		network_velocites.get_network_velocities(
				network_points, network_velocities);

		GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
				network_points.size() == network_velocities.size(),
				GPLATES_ASSERTION_SOURCE);

		// Render each velocity in the current network.
		for (std::size_t velocity_index = 0;
			velocity_index < network_velocities.size();
			++velocity_index)
		{
			const GPlatesMaths::PointOnSphere &point = network_points[velocity_index];
			const GPlatesMaths::Vector3D velocity_vector =
					GPlatesMaths::convert_vector_from_colat_lon_to_xyz(
							point, network_velocities[velocity_index]);

			// Create a RenderedGeometry using the velocity vector.
			const GPlatesViewOperations::RenderedGeometry rendered_vector =
				GPlatesViewOperations::RenderedGeometryFactory::create_rendered_direction_arrow(
					point,
					velocity_vector,
					style_params.velocity_ratio_unit_vector_direction_to_globe_radius,
					velocity_colour);

			// Create a RenderedGeometry for storing the ReconstructionGeometry and
			// a RenderedGeometry associated with it.
			//
			// This means the resolved topological network can be selected by clicking on of
			// its velocity arrows (note: currently arrows cannot be selected so this will
			// not do anything).
			const GPlatesViewOperations::RenderedGeometry rendered_reconstruction_geometry =
					GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
							topological_network,
							rendered_vector);

			rendered_geometry_layer.add_rendered_geometry(rendered_reconstruction_geometry);
		}
	}
}


GPlatesPresentation::ReconstructionGeometryRenderer::StyleParams::StyleParams(
		float reconstruction_line_width_hint_,
		float reconstruction_point_size_hint_,
		float velocity_ratio_unit_vector_direction_to_globe_radius_) :
	reconstruction_line_width_hint(reconstruction_line_width_hint_),
	reconstruction_point_size_hint(reconstruction_point_size_hint_),
	velocity_ratio_unit_vector_direction_to_globe_radius(
			velocity_ratio_unit_vector_direction_to_globe_radius_)
{
}


GPlatesPresentation::ReconstructionGeometryRenderer::ReconstructionGeometryRenderer(
		GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer,
		const StyleParams &style_params,
		const boost::optional<GPlatesGui::Colour> &colour,
		const boost::optional<GPlatesMaths::Rotation> &reconstruction_adjustment) :
	d_rendered_geometry_layer(rendered_geometry_layer),
	d_style_params(style_params),
	d_colour(colour),
	d_reconstruction_adjustment(reconstruction_adjustment)
{
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
{
	GPlatesViewOperations::RenderedGeometry rendered_geometry =
			create_rendered_reconstruction_geometry(
					rfg->geometry(), rfg, d_style_params, d_colour, d_reconstruction_adjustment);

	// Add to the rendered geometry layer.
	d_rendered_geometry_layer.add_rendered_geometry(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_raster_type> &rr)
{
	// Create a RenderedGeometry for drawing the resolved raster.
	GPlatesViewOperations::RenderedGeometry rendered_resolved_raster =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_resolved_raster(
					rr->get_layer(),
					rr->get_georeferencing(),
					rr->get_raster(),
					rr->get_reconstruct_raster_polygons());

	// Create a RenderedGeometry for storing the ReconstructionGeometry and
	// a RenderedGeometry associated with it.
	GPlatesViewOperations::RenderedGeometry rendered_geometry =
			GPlatesViewOperations::RenderedGeometryFactory::create_rendered_reconstruction_geometry(
					rr,
					rendered_resolved_raster);

	// Add to the rendered geometry layer.
	d_rendered_geometry_layer.add_rendered_geometry(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstructed_virtual_geomagnetic_pole_type> &rvgp)
{
	GPlatesAppLogic::VGPRenderSettings* vgp_render_setting = GPlatesAppLogic::VGPRenderSettings::instance();

	if(rvgp->vgp_params().d_vgp_point)
	{
		GPlatesViewOperations::RenderedGeometry rendered_vgp_point =
				create_rendered_reconstruction_geometry(
						*rvgp->vgp_params().d_vgp_point,
						rvgp,
						d_style_params,
						d_colour,
						d_reconstruction_adjustment);
		// Add to the rendered geometry layer.
		d_rendered_geometry_layer.add_rendered_geometry(rendered_vgp_point);
	}

	boost::optional<GPlatesMaths::PointOnSphere> pole_point = boost::none;
	boost::optional<GPlatesMaths::PointOnSphere> site_point = boost::none;
	if(d_reconstruction_adjustment)
	{
		pole_point = (*d_reconstruction_adjustment) * (**rvgp->vgp_params().d_vgp_point);
		if(rvgp->vgp_params().d_site_point)
		{
			site_point = (*d_reconstruction_adjustment) * (**rvgp->vgp_params().d_site_point);
		}
	}
	else
	{
		pole_point = (**rvgp->vgp_params().d_vgp_point);
		if(rvgp->vgp_params().d_site_point)
		{
			site_point = (**rvgp->vgp_params().d_site_point);
		}
	}
	if (vgp_render_setting->should_draw_circular_error() &&
		rvgp->vgp_params().d_a95 )
	{
		GPlatesViewOperations::RenderedGeometry rendered_small_circle = 
				GPlatesViewOperations::create_rendered_small_circle(
						*pole_point,
						GPlatesMaths::convert_deg_to_rad(*rvgp->vgp_params().d_a95),
						GPlatesGui::ColourProxy(rvgp));
		// The circle/ellipse geometries are not (currently) queryable, so we
		// just add the rendered geometry to the layer.
		d_rendered_geometry_layer.add_rendered_geometry(rendered_small_circle);	
	}
	// We can only draw an ellipse if we have dm and dp defined, and if we have
	// a site point. We need the site point so that we can align the ellipse axes
	// appropriately. 
	else if (
			!vgp_render_setting->should_draw_circular_error() && 
			rvgp->vgp_params().d_dm  && 
			rvgp->vgp_params().d_dp  && 
			rvgp->vgp_params().d_site_point )
	{
		GPlatesMaths::GreatCircle great_circle(
				*site_point,
				*pole_point);
				
		GPlatesViewOperations::RenderedGeometry rendered_ellipse = 
			GPlatesViewOperations::create_rendered_ellipse(
					*pole_point,
					GPlatesMaths::convert_deg_to_rad(*rvgp->vgp_params().d_dp),
					GPlatesMaths::convert_deg_to_rad(*rvgp->vgp_params().d_dm),
					great_circle,
					GPlatesGui::ColourProxy(rvgp));

		// The circle/ellipse geometries are not (currently) queryable, so we
		// just add the rendered geometry to the layer.
		d_rendered_geometry_layer.add_rendered_geometry(rendered_ellipse);
	}
}

void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_boundary_type> &rtb)
{
	GPlatesViewOperations::RenderedGeometry rendered_geometry =
			create_rendered_reconstruction_geometry(
					rtb->resolved_topology_geometry(), rtb, d_style_params, d_colour);

	// Add to the rendered geometry layer.
	d_rendered_geometry_layer.add_rendered_geometry(rendered_geometry);
}


void
GPlatesPresentation::ReconstructionGeometryRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
{
	const std::vector<resolved_topological_network_type::resolved_topology_geometry_ptr_type>& 
		geometries = rtn->resolved_topology_geometries();
	std::vector<resolved_topological_network_type::resolved_topology_geometry_ptr_type>::const_iterator 
		it = geometries.begin();
	std::vector<resolved_topological_network_type::resolved_topology_geometry_ptr_type>::const_iterator 
		it_end = geometries.end();
	for(; it !=it_end; it++)
	{
		GPlatesViewOperations::RenderedGeometry rendered_geometry =
				create_rendered_reconstruction_geometry(
						*it, rtn, d_style_params, d_colour);

		// Add to the rendered geometry layer.
		d_rendered_geometry_layer.add_rendered_geometry(rendered_geometry);
	}

	render_topological_network_velocities(rtn, d_rendered_geometry_layer, d_style_params, d_colour);
}
