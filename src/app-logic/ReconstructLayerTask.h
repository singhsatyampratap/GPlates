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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTLAYERTASK_H
#define GPLATES_APP_LOGIC_RECONSTRUCTLAYERTASK_H

#include <boost/shared_ptr.hpp>

#include "LayerTask.h"


namespace GPlatesAppLogic
{
	class ReconstructLayerTask :
			public LayerTask
	{
	public:
		static
		bool
		can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection);


		static
		boost::shared_ptr<ReconstructLayerTask>
		create_layer_task()
		{
			return boost::shared_ptr<ReconstructLayerTask>(new ReconstructLayerTask());
		}


		virtual
		std::vector<ReconstructGraph::input_channel_definition_type>
		get_input_channel_definitions() const;


		virtual
		ReconstructGraph::DataType
		get_output_definition() const;


		virtual
		bool
		process(
				const input_data_type &input_data,
				layer_data_type &output_data,
				const double &reconstruction_time);

	private:
		ReconstructLayerTask()
		{  }

		static const char *RECONSTRUCTION_TREE_CHANNEL_NAME;
		static const char *RECONSTRUCTABLE_FEATURES_CHANNEL_NAME;
	};
}


#endif // GPLATES_APP_LOGIC_RECONSTRUCTLAYERTASK_H
