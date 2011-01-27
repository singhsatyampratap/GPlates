/* $Id: ReconstructedFeatureGeometryExportImpl.cc -1   $ */

/**
 * \file 
 * $Revision: -1 $
 * $Date: $
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
 * Copyright (C) 2010 Geological Survey of Norway
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

#include <algorithm>
#include <map>
#include "global/CompilerWarnings.h"
// Disable Visual Studio warning "qualifier applied to reference type; ignored" in boost 1.36.0
PUSH_MSVC_WARNINGS
DISABLE_MSVC_WARNING(4181)
#include <boost/lambda/bind.hpp>
#include <boost/lambda/lambda.hpp>
POP_MSVC_WARNINGS

#include "ReconstructedFlowlineExportImpl.h"

#include "app-logic/ReconstructedFeatureGeometry.h"
#include "app-logic/ReconstructedFlowline.h"
#include "file-io/File.h"

namespace
{
	//! Convenience typedef for referenced files.
	typedef GPlatesFileIO::ReconstructedFlowlineExportImpl::referenced_files_collection_type
			referenced_files_collection_type;

	//! Convenience typedef for active reconstructable files.
	typedef GPlatesFileIO::ReconstructedFlowlineExportImpl::files_collection_type
			files_collection_type;

	//! Convenience typedef for sequence of flowlines.
	typedef GPlatesFileIO::ReconstructedFlowlineExportImpl::reconstructed_flowline_seq_type
			reconstructed_flowline_seq_type;

	//! Convenience typedef for feature-handle-to-collection map.
	typedef GPlatesFileIO::ReconstructedFlowlineExportImpl::feature_handle_to_collection_map_type
			feature_handle_to_collection_map_type;

	//! Convenience typedef for a sequence of grouped RFGs.
	typedef GPlatesFileIO::ReconstructedFlowlineExportImpl::flowline_group_seq_type
			feature_geometry_group_seq_type;

	class ContainsSameFilePointerPredicate: public std::unary_function<
		GPlatesFileIO::ReconstructedFlowlineExportImpl::FeatureCollectionFlowlineGroup,bool>
	{
	public:
		bool 
		operator()(
			const GPlatesFileIO::ReconstructedFlowlineExportImpl::FeatureCollectionFlowlineGroup& elem) const
		{
			return elem.file_ptr == file_ptr;
		}


		explicit
		ContainsSameFilePointerPredicate(const GPlatesFileIO::File::Reference * file_ptr_):
			file_ptr(file_ptr_){}

	private:
		const GPlatesFileIO::File::Reference *file_ptr;
	};

	/**
	 * Populates mapping of feature handle to feature collection file.
	 */
	void
	populate_feature_handle_to_collection_map(
			feature_handle_to_collection_map_type &feature_handle_to_collection_map,
			const files_collection_type &reconstructable_files)
	{
		// Iterate through the feature collections of the active reconstructable files.
		files_collection_type::const_iterator reconstructable_files_iter;
		for (reconstructable_files_iter = reconstructable_files.begin();
			reconstructable_files_iter != reconstructable_files.end();
			++reconstructable_files_iter)
		{
			const GPlatesFileIO::File::Reference *recon_file = *reconstructable_files_iter;

			const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection_handle =
					recon_file->get_feature_collection();

			if (!feature_collection_handle.is_valid())
			{
				continue;
			}

			// Iterate through the feature handles in the current feature collection.
			GPlatesModel::FeatureCollectionHandle::const_iterator features_iter;
			for (features_iter = feature_collection_handle->begin();
				features_iter != feature_collection_handle->end();
				++features_iter)
			{
				const GPlatesModel::FeatureHandle *feature_handle_ptr = (*features_iter).get();

				// Add feature handle key to our mapping.
				feature_handle_to_collection_map[feature_handle_ptr] = recon_file;
			}
		}
	}


	/**
	 * Returns a unique list of files that reference the visible RFGs.
	 * Result is stored in @a referenced_files.
	 */
	void
	get_unique_list_of_referenced_files(
			referenced_files_collection_type &referenced_files,
			const reconstructed_flowline_seq_type &reconstructed_flowline_seq,
			const feature_handle_to_collection_map_type &feature_handle_to_collection_map)
	{
		// Iterate through the list of RFGs and build up a unique list of
		// feature collection files referenced by them.
		reconstructed_flowline_seq_type::const_iterator rfg_iter;
		for (rfg_iter = reconstructed_flowline_seq.begin();
			rfg_iter != reconstructed_flowline_seq.end();
			++rfg_iter)
		{
			const GPlatesAppLogic::ReconstructedFlowline *rfg = *rfg_iter;
			const GPlatesModel::FeatureHandle *feature_handle_ptr = rfg->feature_handle_ptr();

			const feature_handle_to_collection_map_type::const_iterator map_iter =
					feature_handle_to_collection_map.find(feature_handle_ptr);
			if (map_iter == feature_handle_to_collection_map.end())
			{
				continue;
			}

			const GPlatesFileIO::File::Reference *file = map_iter->second;
			referenced_files.push_back(file);
		}

		using boost::lambda::_1;
		using boost::lambda::_2;

		// Sort in preparation for removing duplicates.
		// We end up sorting on 'const GPlatesFileIO::File::weak_ref' objects.
		std::sort(referenced_files.begin(), referenced_files.end(), _1 < _2);

		// Remove duplicates.
		referenced_files.erase(
				std::unique(referenced_files.begin(), referenced_files.end()),
				referenced_files.end());
	}



}

void
GPlatesFileIO::ReconstructedFlowlineExportImpl::get_files_referenced_by_geometries(
		referenced_files_collection_type &referenced_files,
		const reconstructed_flowline_seq_type &reconstructed_flowline_seq,
		const files_collection_type &reconstructable_files,
		feature_handle_to_collection_map_type &feature_handle_to_collection_map)
{

	populate_feature_handle_to_collection_map(
			feature_handle_to_collection_map,
			reconstructable_files);

	get_unique_list_of_referenced_files(
			referenced_files,
			reconstructed_flowline_seq,
			feature_handle_to_collection_map);
}


void
GPlatesFileIO::ReconstructedFlowlineExportImpl::group_flowlines_with_their_feature(
		flowline_group_seq_type &grouped_flowlines_seq,
		const reconstructed_flowline_seq_type &reconstructed_flowline_seq)
{
	// Copy sequence so we can sort the flowlines by feature.
	reconstructed_flowline_seq_type flowlines_sorted_by_feature(
			reconstructed_flowline_seq);

	using boost::lambda::_1;
	using boost::lambda::_2;

	// Sort in preparation for grouping RFGs by feature.
	std::sort(flowlines_sorted_by_feature.begin(), flowlines_sorted_by_feature.end(),
		boost::lambda::bind(&GPlatesAppLogic::ReconstructedFlowline::feature_handle_ptr, _1) <
					boost::lambda::bind(
							&GPlatesAppLogic::ReconstructedFlowline::feature_handle_ptr, _2));

	const GPlatesModel::FeatureHandle *current_feature_handle_ptr = NULL;

	// Iterate through the sorted sequence and put adjacent RFGs with the same feature
	// into a group.
	reconstructed_flowline_seq_type::const_iterator sorted_flowlines_iter;
	for (sorted_flowlines_iter = flowlines_sorted_by_feature.begin();
		sorted_flowlines_iter != flowlines_sorted_by_feature.end();
		++sorted_flowlines_iter)
	{
		const GPlatesAppLogic::ReconstructedFlowline *rf = *sorted_flowlines_iter;
		const GPlatesModel::FeatureHandle *feature_handle_ptr = rf->feature_handle_ptr();

		if (feature_handle_ptr != current_feature_handle_ptr)
		{
			// Start a new group.
			const GPlatesModel::FeatureHandle::const_weak_ref &feature_ref =
					rf->get_feature_ref();

			grouped_flowlines_seq.push_back(FlowlineGroup(feature_ref));

			current_feature_handle_ptr = feature_handle_ptr;
		}

		// Add the current reconstructed flowline to the current feature.
		grouped_flowlines_seq.back().recon_flowlines.push_back(rf);
	}
}

void
GPlatesFileIO::ReconstructedFlowlineExportImpl::group_flowline_groups_with_their_collection(
	const feature_handle_to_collection_map_type &feature_handle_to_collection_map,
	feature_collection_flowline_group_seq_type &grouped_features_seq,
	const flowline_group_seq_type &grouped_flowlines_seq)
{

	feature_geometry_group_seq_type::const_iterator feature_iter = grouped_flowlines_seq.begin();
	for (; feature_iter != grouped_flowlines_seq.end() ; ++feature_iter)
	{


		GPlatesModel::FeatureHandle::const_weak_ref handle_ref = feature_iter->feature_ref;

		// Need a pointer to use the map. 
		const GPlatesModel::FeatureHandle *handle_ptr = handle_ref.handle_ptr();
		const feature_handle_to_collection_map_type::const_iterator map_iter =
			feature_handle_to_collection_map.find(handle_ptr);
		if (map_iter != feature_handle_to_collection_map.end())
		{
			const GPlatesFileIO::File::Reference *file_ptr = map_iter->second;
			
			ContainsSameFilePointerPredicate predicate(file_ptr);
			feature_collection_flowline_group_seq_type::iterator it =
					std::find_if(grouped_features_seq.begin(),grouped_features_seq.end(),predicate);
			if (it != grouped_features_seq.end())
			{
				// We found the file_ref in the FeatureCollectionFeatureGroup, so add this grouped_refs_seq to it.
				it->flowline_groups.push_back(*feature_iter);
			}
			else
			{
				// We have found a new collection, so create an entry in the feature_collection_feature_group_seq
				FeatureCollectionFlowlineGroup group_of_features(file_ptr);
				group_of_features.flowline_groups.push_back(*feature_iter);
				grouped_features_seq.push_back(group_of_features);
			}
		}
	}

}