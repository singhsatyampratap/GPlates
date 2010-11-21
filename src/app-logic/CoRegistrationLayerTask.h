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
 
#ifndef GPLATES_APP_LOGIC_COREGISTRATIONLAYERTASK_H
#define GPLATES_APP_LOGIC_COREGISTRATIONLAYERTASK_H

#include <utility>
#include <boost/shared_ptr.hpp>
#include <QString>

#include "LayerTask.h"

#include "model/FeatureCollectionHandle.h"
#include "LayerTaskParams.h"


namespace GPlatesAppLogic
{
	/**
	 * A layer task that calculates velocity fields on domains of mesh points.
	 */
	class CoRegistrationLayerTask :
			public LayerTask
	{
	public:

		/**
		 * Can be used to create a layer automatically when a file is first loaded.
		 */
		static
		bool
		is_primary_layer_task_type()
		{
			return true;
		}

		virtual
		LayerTaskParams &
		get_layer_task_params()
		{
			return d_layer_params;
		}

		static
		bool
		can_process_feature_collection(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &feature_collection);


		static
		boost::shared_ptr<CoRegistrationLayerTask>
		create_layer_task()
		{
			return boost::shared_ptr<CoRegistrationLayerTask>(new CoRegistrationLayerTask());
		}


		virtual
		LayerTaskType::Type
		get_layer_type() const
		{
			return LayerTaskType::CO_REGISTRATION;
		}


		virtual
		std::vector<Layer::input_channel_definition_type>
		get_input_channel_definitions() const;


		virtual
		QString
		get_main_input_feature_collection_channel() const;


		virtual
		Layer::LayerOutputDataType
		get_output_definition() const;


		virtual
		bool
		is_topological_layer_task() const
		{
			return false;
		}


		virtual
		boost::optional<layer_task_data_type>
		process(
				const Layer &layer_handle /* the layer invoking this */,
				const input_data_type &input_data,
				const double &reconstruction_time,
				GPlatesModel::integer_plate_id_type anchored_plate_id,
				const ReconstructionTree::non_null_ptr_to_const_type &default_reconstruction_tree);

	private:
		CoRegistrationLayerTask()
		{ }

		//TODO: 
		// This is a temporary function used to export co-registration data
		// Later on, we are going to move export functionality to export dialog.
		const QString
		get_export_file_name(
				const QString, //path
				const QString, //base file name
				const double); //reconstruction time

		LayerTaskParams d_layer_params;
	};
}


#endif // GPLATES_APP_LOGIC_VELOCITYFIELDCALCULATORLAYERTASK_H