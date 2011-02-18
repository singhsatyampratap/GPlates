/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009, 2010, 2011 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_APPLICATIONSTATE_H
#define GPLATES_APP_LOGIC_APPLICATIONSTATE_H

#include <list>
#include <map>
#include <stack>
#include <vector>
#include <boost/noncopyable.hpp>
#include <boost/scoped_ptr.hpp>
#include <boost/shared_ptr.hpp>
#include <QObject>

#include "FeatureCollectionFileState.h"
#include "Layer.h"
#include "LayerTaskRegistry.h"
#include "ReconstructGraph.h"
#include "Reconstruction.h"
#include "ReconstructionTree.h"

#include "global/python.h"

#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/types.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: Please use forward declarations (and boost::scoped_ptr) instead of including headers
// where possible.
// This header gets included in a lot of other files and we want to reduce compile times.
////////////////////////////////////////////////////////////////////////////////////////////////

namespace GPlatesApi
{
	class PythonRunner;
	class PythonExecutionThread;
}

namespace GPlatesAppLogic
{
	class FeatureCollectionFileIO;
	class LayerTask;
	class LayerTaskRegistry;
	class ReconstructGraph;
	class SessionManagement;
	class UserPreferences;


	/**
	 * Stores state that deals with the model and feature collection files.
	 */
	class ApplicationState :
			public QObject,
			private boost::noncopyable
	{
		Q_OBJECT

	public:
		ApplicationState();

		~ApplicationState();

		GPlatesModel::ModelInterface &
		get_model_interface()
		{
			return d_model;
		}

		const double &
		get_current_reconstruction_time() const
		{
			return d_reconstruction_time;
		}

		GPlatesModel::integer_plate_id_type
		get_current_anchored_plate_id() const
		{
			return d_anchored_plate_id;
		}

		/**
		 * Returns the reconstruction generated by the most recent reconstruction.
		 */
		const Reconstruction &
		get_current_reconstruction() const;

		/**
		 * Keeps track of active feature collections loaded from files.
		 */
		FeatureCollectionFileState &
		get_feature_collection_file_state();

		/**
		 * Handling reading/writing feature collection files and notification of read errors.
		 */
		FeatureCollectionFileIO &
		get_feature_collection_file_io();

		/**
		 * Stores/Loads loaded file information to and from persistent storage.
		 */
		GPlatesAppLogic::SessionManagement &
		get_session_management();

		/**
		 * Responsible for all persistent GPlates session storage including user preferences.
		 */
		GPlatesAppLogic::UserPreferences &
		get_user_preferences();

		/**
		 * Returns the layer task registry used to create layer tasks.
		 */
		LayerTaskRegistry &
		get_layer_task_registry();

		/**
		 * Returns the reconstruct graph containing the connected layer tasks.
		 *
		 * NOTE: Make sure to call @a reconstruct when you've finished modifying
		 * the returned @a ReconstructGraph.
		 */
		ReconstructGraph &
		get_reconstruct_graph();

		/**
		 * Create any auto-generate layers necessary and connect to @a file_ref.
		 *
		 * This is needed for situations where a file is added (and hence any
		 * appropriate auto-generate layers are created) but then it is modified
		 * (for example, a new feature is added) such that a new type of autogenerate
		 * layer needs to be created.
		 *
		 * FIXME: This should not need to be called explicitly. Eventually it can
		 * be detected by listening for modifications to feature collections.
		 */
		void
		update_layers(
				const FeatureCollectionFileState::file_reference &file_ref);

		/**
		 * If @a update_default is true, this updates the default reconstruction tree
		 * when the user loads a new rotation file.
		 */
		void
		set_update_default_reconstruction_tree_layer(
				bool update_default = true)
		{
			d_update_default_reconstruction_tree_layer = update_default;
		}

		/**
		 * Returns whether this is updating the default reconstruction tree when the
		 * user loads a new rotation file.
		 */
		bool
		is_updating_default_reconstruction_tree_layer() const
		{
			return d_update_default_reconstruction_tree_layer;
		}

#if !defined(GPLATES_NO_PYTHON)
		const boost::python::object &
		get_python_main_module() const
		{
			return d_python_main_module;
		}

		const boost::python::object &
		get_python_main_namespace() const
		{
			return d_python_main_namespace;
		}
#endif

		/**
		 * Returns an object that runs Python on the main thread.
		 */
		GPlatesApi::PythonRunner *
		get_python_runner() const
		{
			return d_python_runner;
		}

		/**
		 * Returns a thread on which Python code can be run off the main thread.
		 */
		GPlatesApi::PythonExecutionThread *
		get_python_execution_thread()
		{
			return d_python_execution_thread;
		}

	public slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		/**
		 * Sets the reconstruction time and if the reconstruction time has changed
		 * performs a new reconstruction and emits the @a reconstruction_time_changed and
		 * @a reconstructed signals.
		 */
		void
		set_reconstruction_time(
				const double &reconstruction_time);

		/**
		 * Sets the anchor plate id and if the anchor plate id has changed
		 * performs a new reconstruction and emits the @a anchor_plate_id_changed and
		 * @a reconstructed signals.
		 */
		void
		set_anchored_plate_id(
				GPlatesModel::integer_plate_id_type anchor_plate_id);

		/**
		 * Performs a reconstruction and emits the @a reconstructed signal.
		 */
		void
		reconstruct();

	signals:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		//! Emitted when the reconstruction time has changed.
		void
		reconstruction_time_changed(
				GPlatesAppLogic::ApplicationState &application_state,
				const double &new_reconstruction_time);

		//! Emitted when the anchor plate id has changed.
		void
		anchor_plate_id_changed(
				GPlatesAppLogic::ApplicationState &application_state,
				const GPlatesModel::integer_plate_id_type &new_anchor_plate_id);

		//! Emitted when @a reconstruct called.
		void
		reconstructed(
				GPlatesAppLogic::ApplicationState &application_state);

	private slots:
		// NOTE: all signals/slots should use namespace scope for all arguments
		//       otherwise differences between signals and slots will cause Qt
		//       to not be able to connect them at runtime.

		void
		handle_file_state_files_added(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				const std::vector<GPlatesAppLogic::FeatureCollectionFileState::file_reference> &new_files);

		void
		handle_file_state_file_about_to_be_removed(
				GPlatesAppLogic::FeatureCollectionFileState &file_state,
				GPlatesAppLogic::FeatureCollectionFileState::file_reference file);

	private:
		//! Typedef for a sequence of layers.
		typedef std::list<Layer> layer_seq_type;

		//! Typedef for map of loaded files to primary (auto-generated) layers.
		typedef std::map<
				FeatureCollectionFileState::file_reference,
				layer_seq_type> file_to_primary_layers_mapping_type;

		//! Typedef for a stack of reconstruction tree layers.
		typedef std::stack<Layer> default_reconstruction_tree_layer_stack_type;

		//! The model store.
		GPlatesModel::ModelInterface d_model;

		//
		// NOTE: Most of these are boost::scoped_ptr's to avoid having to include header files.
		//

		/**
		 * Central access point and notification of loaded files.
		 */
		boost::scoped_ptr<FeatureCollectionFileState> d_feature_collection_file_state;

		/**
		 * All file reading/writing goes through here.
		 */
		boost::scoped_ptr<FeatureCollectionFileIO> d_feature_collection_file_io;


		boost::scoped_ptr<SessionManagement> d_session_management_ptr;

		boost::scoped_ptr<UserPreferences> d_user_preferences_ptr;

		/**
		 * The layer task registry is used to create layer tasks.
		 */
		boost::scoped_ptr<LayerTaskRegistry> d_layer_task_registry;

		/**
		 * The reconstruct graph connects the inputs/outputs of layer tasks to each other and
		 * to input feature collections.
		 *
		 * It also is solely responsible for generating the complete aggregate reconstruction.
		 */
		boost::scoped_ptr<ReconstructGraph> d_reconstruct_graph;

		/**
		 * A mapping of all primary layers (auto-generated when a file is loaded)
		 * to the auto-generated layers.
		 */
		file_to_primary_layers_mapping_type d_file_to_primary_layers_mapping;

		/**
		 * Keeps track of the default reconstruction tree layers set as rotation files are loaded.
		 * When the rotation file for the current default layer is unloaded the most recent
		 * valid layer is set as the new default.
		 */
		default_reconstruction_tree_layer_stack_type d_default_reconstruction_tree_layer_stack;

		/**
		 * If true, changes the default reconstruction tree layer upon loading a rotation file.
		 */
		bool d_update_default_reconstruction_tree_layer;

		/**
		 * The current reconstruction time.
		 */
		double d_reconstruction_time;

		/**
		 * The current plate id that all reconstruction trees are currently anchored.
		 */
		GPlatesModel::integer_plate_id_type d_anchored_plate_id;

		/**
		 * Reconstruction results are stored here.
		 */
		Reconstruction::non_null_ptr_to_const_type d_reconstruction;

#if !defined(GPLATES_NO_PYTHON)
		/**
		 * The "__main__" Python module.
		 */
		boost::python::object d_python_main_module;

		/**
		 * The "__dict__" attribute of the "__main__" Python module.
		 * This is useful for passing into exec() and eval() for context.
		 */
		boost::python::object d_python_main_namespace;
#endif

		/**
		 * Runs Python code on the main thread.
		 *
		 * Memory managed by Qt.
		 */
		GPlatesApi::PythonRunner *d_python_runner;

		/**
		 * The thread on which Python is executed, off the main thread.
		 *
		 * Memory is managed by Qt.
		 */
		GPlatesApi::PythonExecutionThread *d_python_execution_thread;

		/**
		 * Make signal/slot connections that coordinate the application logic structure
		 * of the application.
		 *
		 * This is a central place to coordinate these connections - a bit like a mediator.
		 */
		void
		mediate_signal_slot_connections();

		/**
		 * Creates all layer tasks that can process @a input_feature_collection.
		 */
		std::vector< boost::shared_ptr<LayerTask> >
		create_layer_tasks(
				const GPlatesModel::FeatureCollectionHandle::const_weak_ref &input_feature_collection);

		/**
		 * Creates a layer task from @a layer_task_type if it's a *primary* layer task type.
		 */
		boost::optional<boost::shared_ptr<GPlatesAppLogic::LayerTask> >
		create_primary_layer_task(
				LayerTaskRegistry::LayerTaskType& layer_task_type);

		/**
		 * Creates new layer(s) that can process the feature collection in @a input_file_ref and
		 * connects the feature collection to the main input of each new layer.
		 */
		void
		create_layers(
				const FeatureCollectionFileState::file_reference &input_file_ref);

		/**
		 * Creates a new layer using @a layer_task and connects @a file_ref to the main input channel.
		 */
		void
		create_layer(
				const FeatureCollectionFileState::file_reference &file_ref,
				const boost::shared_ptr<LayerTask> &layer_task);
	};
}

#endif // GPLATES_APP_LOGIC_APPLICATIONSTATE_H
