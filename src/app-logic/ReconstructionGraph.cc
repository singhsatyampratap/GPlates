/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#include <loki/ScopeGuard.h>
#include <QDebug>

#include "ReconstructionGraph.h"
#include "ReconstructionTree.h"

#include "utils/OverloadResolution.h"


namespace
{
	bool
	edge_is_already_in_graph(
			const GPlatesAppLogic::ReconstructionGraph &graph,
			GPlatesModel::integer_plate_id_type fixed_plate_id,
			GPlatesModel::integer_plate_id_type moving_plate_id,
			const GPlatesMaths::FiniteRotation &pole,
			bool finite_rotation_was_interpolated)
	{
		using namespace GPlatesModel;

		// First, ensure that an edge of this same (fixed plate ID, moving plate ID) does
		// not already exist inside this graph.
		//
		// Note that it's fine if *either one* of the fixed plate ID or moving plate ID are
		// the same as an edge in the graph (same fixed plate ID is natural for a tree;
		// same moving plate ID is a cross-over point; plus, there's the fact that we're
		// also inserting edges for reversed poles), as long as *both* aren't equal.  (If
		// you think about it, how can it be a cross-over point if it's not "crossing over"
		// from one fixed plate ID to a different one?)
		GPlatesAppLogic::ReconstructionGraph::edge_refs_by_plate_id_map_const_range_type
				fixed_plate_id_match_range =
						graph.find_edges_whose_fixed_plate_id_match(fixed_plate_id);
		for (GPlatesAppLogic::ReconstructionGraph::edge_refs_by_plate_id_map_const_iterator iter =
				fixed_plate_id_match_range.first;
				iter != fixed_plate_id_match_range.second;
				++iter) {
			if (iter->second->moving_plate() == moving_plate_id) {
#if 0
				// Output warning message to console.
				qWarning() << "Duplicate reconstruction tree edge with plate ids (moving="
					<< moving_plate_id << ", fixed=" << fixed_plate_id << ")";
				std::cerr << "Warning: Duplicate edges detected:\n";
				ReconstructionGraph::edge_ref_type edge =
						ReconstructionTreeEdge::create(fixed_plate_id,
						moving_plate_id,
						pole,
						originating_feature,
						finite_rotation_was_interpolated,
						ReconstructionTreeEdge::PoleTypes::ORIGINAL);

				output_for_debugging(std::cerr, *edge);
				std::cerr << "is for the same pole as the existing edge:\n";
				output_for_debugging(std::cerr, *(iter->second));
				std::cerr << "Won't add the new edge!\n" << std::endl;
#endif

				return true;
			}
		}

		// Next, ensure that an edge of *reversed* (fixed plate ID, moving plate ID) does
		// not already exist inside this graph.
		GPlatesAppLogic::ReconstructionGraph::edge_refs_by_plate_id_map_const_range_type
				moving_plate_id_match_range =
						graph.find_edges_whose_fixed_plate_id_match(moving_plate_id);
		for (GPlatesAppLogic::ReconstructionGraph::edge_refs_by_plate_id_map_const_iterator iter =
				moving_plate_id_match_range.first;
				iter != moving_plate_id_match_range.second;
				++iter) {
			if (iter->second->moving_plate() == fixed_plate_id) {
				// Output warning message to console.
				qWarning() << "Duplicate reconstruction tree edge with plate ids (moving="
					<< moving_plate_id << ", fixed=" << fixed_plate_id << ")";
#if 0
				std::cerr << "Warning: Duplicate edges detected:\n";
				ReconstructionGraph::edge_ref_type edge =
						ReconstructionTreeEdge::create(fixed_plate_id,
						moving_plate_id,
						pole,
						originating_feature,
						finite_rotation_was_interpolated,
						ReconstructionTreeEdge::PoleTypes::ORIGINAL);

				output_for_debugging(std::cerr, *edge);
				std::cerr << "is for the same pole (reversed) as the existing edge:\n";
				output_for_debugging(std::cerr, *(iter->second));
				std::cerr << "Won't add the new edge!\n" << std::endl;
#endif

				return true;
			}
		}

		return false;
	}
}


void
GPlatesAppLogic::ReconstructionGraph::insert_total_reconstruction_pole(
		GPlatesModel::integer_plate_id_type fixed_plate_id_,
		GPlatesModel::integer_plate_id_type moving_plate_id_,
		const GPlatesMaths::FiniteRotation &pole,
		bool finite_rotation_was_interpolated)
{
	// FIXME:  Confirm that 'fixed_plate_id' and 'moving_plate_id' are not equal

	using namespace GPlatesUtils::OverloadResolution;

	if (edge_is_already_in_graph(*this, fixed_plate_id_, moving_plate_id_, pole,
				finite_rotation_was_interpolated)) {
		// FIXME:  Should we complain?
		return;
	}

	// This is an edge for the "original" pole.
	edge_ref_type original_edge =
			ReconstructionTreeEdge::create(fixed_plate_id_, moving_plate_id_,
					pole, finite_rotation_was_interpolated,
					ReconstructionTreeEdge::PoleTypes::ORIGINAL);

	// This is an edge for the "reversed" pole.
	edge_ref_type reversed_edge =
			ReconstructionTreeEdge::create(moving_plate_id_, fixed_plate_id_,
					GPlatesMaths::get_reverse(pole),
					finite_rotation_was_interpolated,
					ReconstructionTreeEdge::PoleTypes::REVERSED);

	// Now, let's insert the "original" edge.
	// Remember that this function is supposed to be strongly exception-safe; ie, we should
	// provide a commit-or-rollback guarantee.  The only operations left which affect program
	// state, and can throw, are the two 'insert' invocations.  This function is strongly
	// exception-safe (that is, it either succeeds or has no effect), but since we're invoking
	// it twice, it's possible that it could fail the second time around, after the first has 
	// succeeded, leaving the program state changed.  Hence, we're going to have to be a bit
	// more careful, and clean up after ourselves if necessary.
	edge_refs_by_plate_id_map_iterator pos_of_inserted_original_edge =
			d_edges_by_fixed_plate_id.insert(std::make_pair(fixed_plate_id_, original_edge));

	// Undo the insert if an exception is thrown.
	// Note that the 'erase' function does not throw.
	Loki::ScopeGuard guard_insert_original = Loki::MakeGuard(
			// std_map_erase_fn1<edge_refs_by_plate_id_map_type>(),
			resolve<mem_fn_types<edge_refs_by_plate_id_map_type>::erase1>(&edge_refs_by_plate_id_map_type::erase),
			d_edges_by_fixed_plate_id,
			pos_of_inserted_original_edge);

	// Now, let's insert the "reversed" edge.
	//edge_refs_by_plate_id_map_iterator pos_of_inserted_reversed_edge =
			d_edges_by_fixed_plate_id.insert(std::make_pair(moving_plate_id_, reversed_edge));

	// We've made it this far so we can dismiss the undos.
	guard_insert_original.Dismiss();
}


GPlatesUtils::non_null_intrusive_ptr<GPlatesAppLogic::ReconstructionTree>
GPlatesAppLogic::ReconstructionGraph::build_tree(
		GPlatesModel::integer_plate_id_type root_plate_id,
		const double &reconstruction_time,
		const std::vector<GPlatesModel::FeatureCollectionHandle::weak_ref> &reconstruction_features)
{
	return ReconstructionTree::create(
			*this,
			root_plate_id,
			reconstruction_time,
			reconstruction_features);
}


GPlatesAppLogic::ReconstructionGraph::edge_refs_by_plate_id_map_const_range_type
GPlatesAppLogic::ReconstructionGraph::find_edges_whose_fixed_plate_id_match(
		GPlatesModel::integer_plate_id_type plate_id) const
{
	return d_edges_by_fixed_plate_id.equal_range(plate_id);
}


GPlatesAppLogic::ReconstructionGraph::edge_refs_by_plate_id_map_range_type
GPlatesAppLogic::ReconstructionGraph::find_edges_whose_fixed_plate_id_match(
		GPlatesModel::integer_plate_id_type plate_id)
{
	return d_edges_by_fixed_plate_id.equal_range(plate_id);
}
