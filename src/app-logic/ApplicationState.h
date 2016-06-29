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

#include "global/GPlatesAssert.h"
#include "global/PointerTraits.h"

#include "model/FeatureStoreRootHandle.h"
#include "model/FeatureCollectionHandle.h"
#include "model/ModelInterface.h"
#include "model/types.h"
#include "model/WeakReferenceCallback.h"

////////////////////////////////////////////////////////////////////////////////////////////////
// NOTE: Please use forward declarations (and boost::scoped_ptr) instead of including headers
// where possible.
// This header gets included in a lot of other files and we want to reduce compile times.
////////////////////////////////////////////////////////////////////////////////////////////////


namespace GPlatesFileIO
{
	namespace FeatureCollectionFileFormat
	{
		class Registry;
	}
}

namespace GPlatesAppLogic
{
	class FeatureCollectionFileIO;
	class LayerTask;
	class LayerTaskRegistry;
	class LogModel;
	class ReconstructGraph;
	class ReconstructMethodRegistry;
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

		//! Const overload.
		const FeatureCollectionFileState &
		get_feature_collection_file_state() const;

		/**
		 * Handling file formats for reading and/or writing feature collection files.
		 */
		GPlatesFileIO::FeatureCollectionFileFormat::Registry &
		get_feature_collection_file_format_registry();

		//! Const overload.
		const GPlatesFileIO::FeatureCollectionFileFormat::Registry &
		get_feature_collection_file_format_registry() const;

		/**
		 * Handling reading/writing feature collection files and notification of read errors.
		 */
		FeatureCollectionFileIO &
		get_feature_collection_file_io();

		//! Const overload.
		const FeatureCollectionFileIO &
		get_feature_collection_file_io() const;

		/**
		 * Responsible for all persistent GPlates session storage including user preferences.
		 */
		UserPreferences &
		get_user_preferences();

		//! Const overload.
		const UserPreferences &
		get_user_preferences() const;


		/**
		 * Returns the registry of various ways to reconstruct a feature
		 * into @a ReconstructedFeatureGeometry objects.
		 *
		 * This is exposed so that later we can registry user python reconstruct methods.
		 */
		ReconstructMethodRegistry &
		get_reconstruct_method_registry();

		//! Const overload.
		const ReconstructMethodRegistry &
		get_reconstruct_method_registry() const;

		/**
		 * Returns the layer task registry used to create layer tasks.
		 */
		LayerTaskRegistry &
		get_layer_task_registry();

		//! Const overload.
		const LayerTaskRegistry &
		get_layer_task_registry() const;

		/**
		 * The Log Model is a Qt Model/View class that does the back-end work for the LogDialog.
		 */
		LogModel &
		get_log_model();

		//! Const overload.
		const LogModel &
		get_log_model() const;

		/**
		 * Returns the reconstruct graph containing the connected layer tasks.
		 *
		 * NOTE: Make sure to call @a reconstruct when you've finished modifying
		 * the returned @a ReconstructGraph.
		 */
		ReconstructGraph &
		get_reconstruct_graph();

		//! Const overload.
		const ReconstructGraph &
		get_reconstruct_graph() const;

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

		/**
		 * Suppress the normal auto-creation of layers upon file load in handle_file_state_files_added(),
		 * which would normally be triggered by a call to FeatureCollectionFileIO::load_files().
		 *
		 * This is called from SessionManagement during the loading of a previous Session.
		 */
		void
		suppress_auto_layer_creation(
				bool suppress)
		{
			d_suppress_auto_layer_creation = suppress;
		}


		/**
		 * A convenience class that blocks calls to @a reconstruct and optionally also
		 * calls @a reconstruct when at the scope exit.
		 *
		 * This is useful for clients that are going to call @a reconstruct explicitly but
		 * don't want any unnecessary calls to @a reconstruct to be triggered before then.
		 *
		 * This is currently used by the file loading code to allow loading of multiple
		 * files in one group without causing each file loaded to trigger a separate reconstruction
		 * which can slow loading significantly. Things which trigger the individual loads are
		 * signals emitted when connecting newly created layers.
		 *
		 * These objects can be nested in which case only the destructor of the outermost
		 * scoped object will call @a reconstruct.
		 */
		class ScopedReconstructGuard :
				public boost::noncopyable
		{
		public:
			/**
			 * Constructor blocks calls to @a reconstruct.
			 *
			 * Also optionally calls @a reconstruct on scope exit if @a call_reconstruct_on_scope_exit is true.
			 */
			explicit
			ScopedReconstructGuard(
					ApplicationState &application_state,
					bool reconstruct_on_scope_exit = false) :
				d_application_state(application_state),
				d_call_reconstruct_on_scope_exit(reconstruct_on_scope_exit)
			{
				d_application_state.begin_reconstruct_on_scope_exit();
			}

			/**
			 * Destructor unblocks calls to @a reconstruct but does *not* detect and re-issue
			 * any blocked calls to @a reconstruct.
			 *
			 * But it does call @a reconstruct if 'call_reconstruct_on_scope_exit' was specified in constructor.
			 *
			 * NOTE: Only unblocks when *all* existing @a ScopedReconstructGuard objects go out of scope.
			 */
			~ScopedReconstructGuard()
			{
				d_application_state.end_reconstruct_on_scope_exit(d_call_reconstruct_on_scope_exit);
			}

			/**
			 * Causes @a reconstruct to be called on scope exit (more specifically when *all*
			 * existing @a ScopedReconstructGuard objects go out of scope).
			 *
			 * NOTE: Only one @a ScopedReconstructGuard object out of a nested group of objects
			 * needs to call this for @a reconstruct to be called (when all objects go out of scope).
			 *
			 * It is useful to call this near the end of the scope to avoid @a reconstruct being
			 * called if an exception is thrown (because destructors are called during stack unwind).
			 */
			void
			call_reconstruct_on_scope_exit()
			{
				d_call_reconstruct_on_scope_exit = true;
			}

		private:
			ApplicationState &d_application_state;
			bool d_call_reconstruct_on_scope_exit;
		};

	public Q_SLOTS:
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

	Q_SIGNALS:
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

	private Q_SLOTS:
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
		/**
		 * The model callback that notifies us when the feature store is modified so that
		 * we can do a reconstruction.
		 */
		struct ReconstructWhenFeatureStoreIsModified :
				public GPlatesModel::WeakReferenceCallback<const GPlatesModel::FeatureStoreRootHandle>
		{
			explicit
			ReconstructWhenFeatureStoreIsModified(
					ApplicationState &application_state) :
				d_application_state(&application_state)
			{  }

			void
			publisher_modified(
					const weak_reference_type &reference,
					const modified_event_type &event)
			{
				// Perform a reconstruction every time the model (feature store) is modified,
				// unless we are already inside a reconstruction (avoid infinite cycle).
				if (!d_application_state->d_currently_reconstructing)
				{
					// Clients should put model notification guards in the appropriate places to
					// avoid excessive reconstructions.
					d_application_state->reconstruct();
				}
			}

			ApplicationState *d_application_state;
		};


		//! The model store.
		GPlatesModel::ModelInterface d_model;

		//
		// NOTE: Most of these are boost::scoped_ptr's to avoid having to include header files.
		//

		/**
		 * A registry of the file formats for reading/writing feature collections.
		 *
		 * NOTE: This must be declared *before* @a d_feature_collection_file_io.
		 */
		boost::scoped_ptr<GPlatesFileIO::FeatureCollectionFileFormat::Registry> d_feature_collection_file_format_registry;

		/**
		 * Central access point and notification of loaded files.
		 */
		boost::scoped_ptr<FeatureCollectionFileState> d_feature_collection_file_state;

		/**
		 * All file reading/writing goes through here.
		 */
		boost::scoped_ptr<FeatureCollectionFileIO> d_feature_collection_file_io;

		boost::scoped_ptr<UserPreferences> d_user_preferences_ptr;

		/**
		 * A registry for various ways to reconstruct a feature into @a ReconstructedFeatureGeometry objects.
		 */
		boost::scoped_ptr<ReconstructMethodRegistry> d_reconstruct_method_registry;

		/**
		 * The layer task registry is used to create layer tasks.
		 */
		boost::scoped_ptr<LayerTaskRegistry> d_layer_task_registry;
		
		/**
		 * The Log Model is a Qt Model/View class that does the back-end work for the LogDialog.
		 */
		boost::scoped_ptr<LogModel> d_log_model;

		/**
		 * The reconstruct graph connects the inputs/outputs of layer tasks to each other and
		 * to input feature collections.
		 *
		 * It also is solely responsible for generating the complete aggregate reconstruction.
		 *
		 * NOTE: This must be declared after @a d_layer_task_registry since it uses it.
		 */
		boost::scoped_ptr<ReconstructGraph> d_reconstruct_graph;
		
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

		/**
		 * Keep tracks of the nesting of @a ScopedReconstruct objects.
		 *
		 * When the count reaches zero then @a reconstruct will be called and
		 * subsequent calls to @a reconstruct will no longer be blocked.
		 */
		int d_scoped_reconstruct_nesting_count;

		/**
		 * Used, along with @a d_scoped_reconstruct_nesting_count, to determine if @a reconstruct
		 * should be called when all @a ScopedReconstructGuard objects go out of scope.
		 */
		bool d_reconstruct_on_scope_exit;

		/**
		 * Is true if we are currently inside/executing the @a reconstruct method.
		 *
		 * This is used to avoid re-entering @a reconstruct (due to a model notification event
		 * happening inside the @a reconstruct method). Clients still need to make sure they
		 * don't call @a reconstruct if they form part of the @a reconstruct implementation -
		 * ie, it obviously makes no sense for @a ReconstructGraph to call @a reconstruct.
		 */
		bool d_currently_reconstructing;

		/**
		 * Suppress the normal auto-creation of layers upon file load in handle_file_state_files_added(),
		 * which would normally be triggered by a call to FeatureCollectionFileIO::load_files().
		 */
		bool d_suppress_auto_layer_creation;

		/**
		 * Keep a weak reference to the feature store root handle just for our callback.
		 *
		 * Only we have access to this weak ref and we make sure the client doesn't have
		 * access to it. This is because any copies of this weak reference also get
		 * copies of the callback thus allowing it to get called more than once per modification.
		 */
		GPlatesModel::FeatureStoreRootHandle::const_weak_ref d_callback_feature_store;


		/**
		 * Make signal/slot connections that coordinate the application logic structure
		 * of the application.
		 *
		 * This is a central place to coordinate these connections - a bit like a mediator.
		 */
		void
		mediate_signal_slot_connections();

		//! Begin blocking of calls to @a reconstruct.
		void
		begin_reconstruct_on_scope_exit();

		/**
		 * Ends blocking of calls to @a reconstruct and calls @a reconstruct if the nested
		 * scope count decrements to zero.
		 */
		void
		end_reconstruct_on_scope_exit(
				bool reconstruct_on_scope_exit);

		// Make friend so can call @a begin_reconstruct_on_scope_exit and @a end_reconstruct_on_scope_exit.
		friend class ScopedReconstructGuard;
	};
}

#endif // GPLATES_APP_LOGIC_APPLICATIONSTATE_H
