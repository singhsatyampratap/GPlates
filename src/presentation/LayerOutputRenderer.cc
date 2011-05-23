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

#include <boost/foreach.hpp>

#include "LayerOutputRenderer.h"

#include "app-logic/MultiPointVectorField.h"
#include "app-logic/RasterLayerProxy.h"
#include "app-logic/ReconstructionLayerProxy.h"
#include "app-logic/ReconstructLayerProxy.h"
#include "app-logic/ResolvedTopologicalBoundary.h"
#include "app-logic/ResolvedTopologicalNetwork.h"
#include "app-logic/TopologyBoundaryResolverLayerProxy.h"
#include "app-logic/TopologyNetworkResolverLayerProxy.h"
#include "app-logic/VelocityFieldCalculatorLayerProxy.h"

#include "presentation/ReconstructionGeometryRenderer.h"


GPlatesPresentation::LayerOutputRenderer::LayerOutputRenderer(
		ReconstructionGeometryRenderer &reconstruction_geometry_renderer,
		GPlatesViewOperations::RenderedGeometryLayer &rendered_geometry_layer) :
	d_reconstruction_geometry_renderer(reconstruction_geometry_renderer),
	d_rendered_geometry_layer(rendered_geometry_layer)
{
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<co_registration_layer_proxy_type> &layer_proxy)
{
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<raster_layer_proxy_type> &raster_layer_proxy)
{
	// Get the resolved raster for the current reconstruction time.
	boost::optional<GPlatesAppLogic::ResolvedRaster::non_null_ptr_type> resolved_raster =
			raster_layer_proxy->get_resolved_raster();

	if (resolved_raster)
	{
		d_reconstruction_geometry_renderer.begin_render();

		// Render the resolved raster.
		resolved_raster.get()->accept_visitor(d_reconstruction_geometry_renderer);

		d_reconstruction_geometry_renderer.end_render(d_rendered_geometry_layer);
	}
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstruct_layer_proxy_type> &reconstruct_layer_proxy)
{
	// Get the reconstructed feature geometries in a spatial partition for the current reconstruction time.
	GPlatesAppLogic::ReconstructLayerProxy::reconstructed_feature_geometries_spatial_partition_type::non_null_ptr_to_const_type
			rfg_spatial_partition =
					reconstruct_layer_proxy->get_reconstructed_feature_geometries_spatial_partition();

	d_reconstruction_geometry_renderer.begin_render();

	// Render all reconstructed feature geometries in the spatial partition.
	d_reconstruction_geometry_renderer.render(*rfg_spatial_partition);

	d_reconstruction_geometry_renderer.end_render(d_rendered_geometry_layer);
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<reconstruction_layer_proxy_type> &reconstruction_layer_proxy)
{
	// Nothing to visualise for this layer type.
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<topology_boundary_resolver_layer_proxy_type> &topology_boundary_resolver_layer_proxy)
{
	// Get the resolved topological boundaries for the current reconstruction time.
	std::vector<GPlatesAppLogic::resolved_topological_boundary_non_null_ptr_type> resolved_topological_boundaries;
	topology_boundary_resolver_layer_proxy->get_resolved_topological_boundaries(resolved_topological_boundaries);

	d_reconstruction_geometry_renderer.begin_render();

	// Render each resolved topological boundary.
	BOOST_FOREACH(
			const GPlatesAppLogic::resolved_topological_boundary_non_null_ptr_type &resolved_topological_boundary,
			resolved_topological_boundaries)
	{
		resolved_topological_boundary->accept_visitor(d_reconstruction_geometry_renderer);
	}

	d_reconstruction_geometry_renderer.end_render(d_rendered_geometry_layer);
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<topology_network_resolver_layer_proxy_type> &topology_network_resolver_layer_proxy)
{
	// Get the resolved topological networks for the current reconstruction time.
	std::vector<GPlatesAppLogic::resolved_topological_boundary_non_null_ptr_type> resolved_topological_boundaries;
	std::vector<GPlatesAppLogic::resolved_topological_network_non_null_ptr_type> resolved_topological_networks;

	topology_network_resolver_layer_proxy->get_resolved_topological_networks(
		resolved_topological_boundaries,
		resolved_topological_networks);

	d_reconstruction_geometry_renderer.begin_render();

	// Render each resolved topological network.
	BOOST_FOREACH(
			const GPlatesAppLogic::resolved_topological_network_non_null_ptr_type &resolved_topological_network,
			resolved_topological_networks)
	{
		resolved_topological_network->accept_visitor(d_reconstruction_geometry_renderer);
	}

	d_reconstruction_geometry_renderer.end_render(d_rendered_geometry_layer);
}


void
GPlatesPresentation::LayerOutputRenderer::visit(
		const GPlatesUtils::non_null_intrusive_ptr<velocity_field_calculator_layer_proxy_type> &velocity_field_calculator_layer_proxy)
{
	// Get the velocity vector fields for the current reconstruction time.
	std::vector<GPlatesAppLogic::multi_point_vector_field_non_null_ptr_type> multi_point_vector_fields;
	velocity_field_calculator_layer_proxy->get_velocity_multi_point_vector_fields(multi_point_vector_fields);

	d_reconstruction_geometry_renderer.begin_render();

	// Render each velocity vector field.
	BOOST_FOREACH(
			const GPlatesAppLogic::multi_point_vector_field_non_null_ptr_type &multi_point_vector_field,
			multi_point_vector_fields)
	{
		multi_point_vector_field->accept_visitor(d_reconstruction_geometry_renderer);
	}

	d_reconstruction_geometry_renderer.end_render(d_rendered_geometry_layer);
}