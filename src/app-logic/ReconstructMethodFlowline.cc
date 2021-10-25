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
#include <boost/optional.hpp>

#include "ReconstructMethodFlowline.h"

#include "GeometryUtils.h"
#include "FlowlineGeometryPopulator.h"
#include "FlowlineUtils.h"
#include "RotationUtils.h"

#include "maths/MultiPointOnSphere.h"
#include "maths/PointOnSphere.h"
#include "maths/PolygonOnSphere.h"
#include "maths/PolylineOnSphere.h"

#include "model/FeatureVisitor.h"
#include "model/types.h"

#include "property-values/GmlLineString.h"
#include "property-values/GmlMultiPoint.h"
#include "property-values/GmlOrientableCurve.h"
#include "property-values/GmlPoint.h"
#include "property-values/GmlPolygon.h"
#include "property-values/GmlTimePeriod.h"
#include "property-values/GpmlConstantValue.h"
#include "property-values/GpmlPlateId.h"


namespace GPlatesAppLogic
{
	namespace
	{
		/**
		 * Finds the present day geometries of a flowline feature.
		 *
		 * Present day geometries probably don't make too much sense for flowlines but
		 * we'll add points and multipoints since they are what is currently used to seed
		 * flowlines.
		 *
		 * FIXME: Currently multiple RFGs map to a single geometry property iterator so even though
		 * we probably only have one present day geometry (the seed point or multipoint) we'll
		 * get multiple RFGs for it. Worst that'll happen is ReconstructContext will map all
		 * RFGs (generated by a flowline feature) to that single present day geometry but this
		 * is ok because the present day geometries are only used by clients interested in RFGs
		 * that have a finite rotation (so they can rotate the present day geometry to get
		 * the reconstructed geometry such as on the graphics card where it's cheaper).
		 * When rendering flowlines the painters will only find reconstructed geometries, which
		 * is fine, and it will paint them directly without rotating them. Also means flowlines
		 * can't be used for reconstructing rasters, etc, but they're not designed for that anyway.
		 */
		class GetPresentDayGeometries :
				public GPlatesModel::FeatureVisitor
		{
		public:
			GetPresentDayGeometries(
					std::vector<ReconstructMethodInterface::Geometry> &present_day_geometries) :
				d_present_day_geometries(present_day_geometries)
			{  }

		private:
			virtual
			void
			visit_gml_multi_point(
					GPlatesPropertyValues::GmlMultiPoint &gml_multi_point)
			{
				d_present_day_geometries.push_back(
						ReconstructMethodInterface::Geometry(
								*current_top_level_propiter(),
								gml_multi_point.multipoint()));
			}

			virtual
			void
			visit_gml_point(
					GPlatesPropertyValues::GmlPoint &gml_point)
			{
				d_present_day_geometries.push_back(
						ReconstructMethodInterface::Geometry(
								*current_top_level_propiter(),
								gml_point.point()));
			}


			virtual
			void
			visit_gpml_constant_value(
				GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value)
			{
				gpml_constant_value.value()->accept_visitor(*this);
			}

			std::vector<ReconstructMethodInterface::Geometry> &d_present_day_geometries;
		};
	}
}


bool
GPlatesAppLogic::ReconstructMethodFlowline::can_reconstruct_feature(
		const GPlatesModel::FeatureHandle::const_weak_ref &feature_weak_ref)
{
	FlowlineUtils::DetectFlowlineFeatures visitor;
	visitor.visit_feature(feature_weak_ref);

	return visitor.has_flowline_features();
}


void
GPlatesAppLogic::ReconstructMethodFlowline::get_present_day_feature_geometries(
		std::vector<Geometry> &present_day_geometries) const
{
	GetPresentDayGeometries visitor(present_day_geometries);

	visitor.visit_feature(get_feature_ref());
}


void
GPlatesAppLogic::ReconstructMethodFlowline::reconstruct_feature_geometries(
		std::vector<ReconstructedFeatureGeometry::non_null_ptr_type> &reconstructed_feature_geometries,
		const ReconstructHandle::type &reconstruct_handle,
		const Context &context,
		const double &reconstruction_time)
{
	FlowlineGeometryPopulator visitor(
			reconstructed_feature_geometries,
			context.reconstruction_tree_creator,
			reconstruction_time);

	visitor.visit_feature(get_feature_ref());
}


void
GPlatesAppLogic::ReconstructMethodFlowline::reconstruct_feature_velocities(
		std::vector<MultiPointVectorField::non_null_ptr_type> &reconstructed_feature_velocities,
		const ReconstructHandle::type &reconstruct_handle,
		const Context &context,
		const double &reconstruction_time,
		const double &velocity_delta_time,
		VelocityDeltaTime::Type velocity_delta_time_type)
{
	// Get some flowline properties.
	FlowlineUtils::FlowlinePropertyFinder flowline_property_finder(reconstruction_time);
	flowline_property_finder.visit_feature(get_feature_ref());

    if (!flowline_property_finder.can_process_seed_point() ||
		!flowline_property_finder.can_process_flowline())
    {
		return;
    }

	//
	// FIXME: I'm not sure if the following velocity calculation is correct.
	// Need to go through FlowlineUtils to see how flowlines are reconstructed.
	// For now we'll just calculate the velocity at the seed point at the current reconstruction time.
	//

	// If we can't get left/right plate IDs then we'll just use plate id zero (spin axis)
	// which can still give a non-identity rotation if the anchor plate id is non-zero.
	GPlatesModel::integer_plate_id_type left_plate_id = 0;
	GPlatesModel::integer_plate_id_type right_plate_id = 0;
	if (flowline_property_finder.get_left_plate())
	{
		left_plate_id = flowline_property_finder.get_left_plate().get();
	}
	if (flowline_property_finder.get_right_plate())
	{
		right_plate_id = flowline_property_finder.get_right_plate().get();
	}

	const std::pair<double, double> time_range = VelocityDeltaTime::get_time_range(
			velocity_delta_time_type, reconstruction_time, velocity_delta_time);

	// Iterate over the feature's present day geometries and rotate each one.
	std::vector<Geometry> present_day_geometries;
	get_present_day_feature_geometries(present_day_geometries);
	BOOST_FOREACH(const Geometry &present_day_geometry, present_day_geometries)
	{
		// Get the half-stage rotation.
		const GPlatesMaths::FiniteRotation finite_rotation_1 = RotationUtils::get_half_stage_rotation(
				context.reconstruction_tree_creator,
				time_range.second/*young*/,
				left_plate_id,
				right_plate_id);
		const GPlatesMaths::FiniteRotation finite_rotation_2 = RotationUtils::get_half_stage_rotation(
				context.reconstruction_tree_creator,
				time_range.first/*old*/,
				left_plate_id,
				right_plate_id);

		// Use either the young or old half-stage rotations if the reconstruction time matches.
		// Otherwise calculate a new half-stage rotation.
		const GPlatesMaths::FiniteRotation finite_rotation =
				GPlatesMaths::are_almost_exactly_equal(reconstruction_time, time_range.second/*young*/)
				? finite_rotation_1
				: (GPlatesMaths::are_almost_exactly_equal(reconstruction_time, time_range.first/*old*/)
						? finite_rotation_2
						: RotationUtils::get_half_stage_rotation(
								context.reconstruction_tree_creator,
								reconstruction_time,
								left_plate_id,
								right_plate_id));

		// NOTE: This is slightly dodgy because we will end up creating a MultiPointVectorField
		// that stores a multi-point domain and a corresponding velocity field but the
		// geometry property iterator (referenced by the MultiPointVectorField) could be a
		// non-multi-point geometry.
		GPlatesMaths::MultiPointOnSphere::non_null_ptr_to_const_type velocity_domain =
				GeometryUtils::convert_geometry_to_multi_point(*present_day_geometry.geometry);

		// Rotate the velocity domain.
		// We do this even if the plate id is zero because the anchor plate might be non-zero.
		velocity_domain = finite_rotation * velocity_domain;

		// Create an RFG purely for the purpose of representing the feature that generated the
		// plate ID (ie, this feature).
		// This is required in order for the velocity arrows to be coloured correctly -
		// because the colouring code requires a reconstruction geometry (it will then
		// lookup the plate ID or other feature property(s) depending on the colour scheme).
		const ReconstructedFeatureGeometry::non_null_ptr_type plate_id_rfg =
				ReconstructedFeatureGeometry::create(
						context.reconstruction_tree_creator.get_reconstruction_tree(reconstruction_time),
						context.reconstruction_tree_creator,
						*get_feature_ref(),
						present_day_geometry.property_iterator,
						velocity_domain,
						ReconstructMethod::FLOWLINE,
						flowline_property_finder.get_reconstruction_plate_id(),
						flowline_property_finder.get_time_of_appearance(),
						reconstruct_handle);

		GPlatesMaths::MultiPointOnSphere::const_iterator domain_iter = velocity_domain->begin();
		GPlatesMaths::MultiPointOnSphere::const_iterator domain_end = velocity_domain->end();

		MultiPointVectorField::non_null_ptr_type vector_field =
				MultiPointVectorField::create_empty(
						reconstruction_time,
						velocity_domain,
						*get_feature_ref(),
						present_day_geometry.property_iterator,
						reconstruct_handle);
		MultiPointVectorField::codomain_type::iterator field_iter = vector_field->begin();

		// Iterate over the domain points and calculate their velocities.
		for ( ; domain_iter != domain_end; ++domain_iter, ++field_iter)
		{
			// Calculate the velocity.
			const GPlatesMaths::Vector3D vector_xyz =
					GPlatesMaths::calculate_velocity_vector(
							*domain_iter,
							finite_rotation_1,
							finite_rotation_2,
							time_range.first/*old*/ - time_range.second/*young*/);

			*field_iter = MultiPointVectorField::CodomainElement(
					vector_xyz,
					MultiPointVectorField::CodomainElement::ReconstructedDomainPoint,
					flowline_property_finder.get_reconstruction_plate_id(),
					ReconstructionGeometry::maybe_null_ptr_to_const_type(plate_id_rfg.get()));
		}

		reconstructed_feature_velocities.push_back(vector_field);
	}
}


GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type
GPlatesAppLogic::ReconstructMethodFlowline::reconstruct_geometry(
		const GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type &geometry,
		const Context &context,
		const double &reconstruction_time,
		bool reverse_reconstruct)
{
    return GPlatesAppLogic::FlowlineUtils::reconstruct_flowline_seed_points(
			geometry,
			reconstruction_time,
			context.reconstruction_tree_creator,
			get_feature_ref(),
			reverse_reconstruct);
}
