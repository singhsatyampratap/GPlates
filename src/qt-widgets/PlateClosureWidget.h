/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_PLATECLOSUREWIDGET_H
#define GPLATES_QTWIDGETS_PLATECLOSUREWIDGET_H

#include <QDebug>
#include <QWidget>
#include <QTreeWidget>
#include <QUndoStack>
#include <boost/optional.hpp>
#include <boost/none.hpp>
#include "PlateClosureWidgetUi.h"

#include "maths/GeometryOnSphere.h"
#include "model/ModelInterface.h"

#include "model/FeatureHandle.h"
#include "model/ReconstructedFeatureGeometry.h"

#include "model/PropertyValue.h"
#include "property-values/GpmlTopologicalSection.h"

// An effort to reduce the dependency spaghetti currently plaguing the GUI.
namespace GPlatesGui
{
	class FeatureFocus;
}

namespace GPlatesQtWidgets
{
	// Forward declaration to avoid the GUI header spaghetti as much as possible.
	// class ExportCoordinatesDialog;
	class CreateFeatureDialog;
	class ViewportWindow;

	class PlateClosureWidget:
			public QWidget, 
			protected Ui_PlateClosureWidget
	{
		Q_OBJECT

	public:

		/**
		 * What kinds of geometry the PlateClosureWidget can be configured for.
		 */
		enum GeometryType
		{
			PLATEPOLYGON,
			DEFORMINGPLATE
		};
		

		explicit
		PlateClosureWidget(
				GPlatesGui::FeatureFocus &feature_focus,
				GPlatesModel::ModelInterface &model_interface,
				ViewportWindow &view_state_,
				QWidget *parent_ = NULL);
		
#if 0
		/**
		 * Accessor so the QUndoCommands can get at the table of points.
		 */
		QTreeWidget *
		coordinates_table()
		{
			return treewidget_coordinates;
		}
					
		/**
		 * Updates the text on all top-level QTreeWidgetItems (the labels)
		 * in the table to reflect what geometry (parts) you'll actually get.
		 *
		 * This public method is used by the QUndoCommands that manipulate this widget.
		 */
		void
		update_table_labels();

		/**
		 * Updates the temporary geometry rendered on screen.
		 *
		 * This sets d_geometry_opt_ptr appropriately and pushes new temporary geometry
		 * to the ViewState after clearing it.
		 *
		 * If there are no (valid) coordinates in the table, d_geometry_opt_ptr will be
		 * set to boost::none and the temporary geometry will be cleared from the view.
		 *
		 * This public method is used by the QUndoCommands that manipulate this widget.
		 */
		void
		update_geometry();
#endif

		/**
		 * Sets the desired geometry type, d_geometry_type.
		 *
		 * This public method is used by the QUndoCommands that manipulate this widget.
		 *
		 * FIXME: If we move d_geometry_type into the QTreeWidgetItems themselves, we
		 * wouldn't need this ugly setter.
		 */
		void
		set_geometry_type(
				GeometryType geom_type)
		{
			d_geometry_type = geom_type;
		}

		/**
		 * Access the desired geometry type, d_geometry_type.
		 */
		GeometryType
		geometry_type() const
		{
			return d_geometry_type;
		}

		/**
		 * Accessor for the Create Feature Dialog, for signal/slot connections etc.
		 */
		CreateFeatureDialog &
		create_feature_dialog()
		{
			return *d_create_feature_dialog;
		}
		
		/**
		 * Accessor for the QUndoStack used for digitisation operations.
		 * 
		 * This method allows the ViewportWindow to add it to the main QUndoGroup,
		 * and lets the stack be set as active or inactive.
		 */
		QUndoStack &
		undo_stack()
		{
			return d_undo_stack;
		}


	public slots:
		
		/**
		 * Resets all fields to their defaults.
		 */
		void
		clear();

		void
		display_feature(
			GPlatesModel::FeatureHandle::weak_ref feature_ref,
			GPlatesModel::ReconstructedFeatureGeometry::maybe_null_ptr_type associated_rfg);

		/**
		 * Configures widgets to accept new geometry of a specific type.
		 * This will clear the coordinates table and purge the undo stack.
		 */
		void
		initialise_geometry(
				GeometryType geom_type);

		/**
		 * Configures widgets for a new geometry type while preserving the
		 * points that are currently being digitised.
		 * 
		 * Triggered when the user switches to a different PlateClosure CanvasTool.
		 */
		void
		change_geometry_type(
				GeometryType geom_type);

#if 0
		/**
		 * Adds a new lat,lon to the specified geometry (defaults to NULL,
		 * indicating the 'default geometry', which for now is just the
		 * first geometry available. This is convenient because to start
		 * with, I'm only handling unbroken LineString and MultiPoint.)
		 *
		 * If the specified geometry is a gml:Point, the given lat,lon will
		 * NOT be added to it but will instead replace it - I believe this
		 * would be the appropriate behaviour when (re)digitising a gml:position.
		 * (FIXME: Unimplemented).
		 *
		 * The PlateClosureetry canvas tool uses this function to populate
		 * this PlateClosureWidget.
		 */
		void
		append_point_to_geometry(
				double lat,
				double lon,
				QTreeWidgetItem *target_geometry = NULL);
#endif

		/**
		 * Draw the temporary geometry (if there is one) on the screen.
		 */
		void
		draw_temporary_geometry();

	private slots:

		/**
		 * The slot that gets called when the user clicks "Use Coordinates in Reverse".
		 */
		void
		handle_use_coordinates_in_reverse();

		/**
		 * The slot that gets called when the user clicks "Choose Feature".
		 */
		void
		handle_choose_feature();

		/**
		 * The slot that gets called when the user clicks "Clear".
		 */
		void
		handle_clear();

		/**
		 * The slot that gets called when the user clicks "Create".
		 */
		void
		handle_create();

		/**
		 * The slot that gets called when the user clicks "Cancel".
		 */
		void
		handle_cancel();

#if 0
		/**
		 * Feeds the ExportCoordinatesDialog a GeometryOnSphere, and
		 * then displays it.
		 */
		void
		handle_export();
#endif

	private:

		/**
		 * The Undo Stack that handles all the Undo Commands for this widget.
		 * 
		 * We may want to move this stack into e.g. ViewState,
		 * or use a @a QUndoGroup to manage this stack and others.
		 */
		QUndoStack d_undo_stack;

		/**
		 * The View State is used to access the digitisation layer in the globe in the
		 * globe canvas.
		 */
		ViewportWindow *d_view_state_ptr;

		/**
		 * The dialog the user sees when they hit the Create button.
		 * Memory managed by Qt.
		 */
		CreateFeatureDialog *d_create_feature_dialog;
		
		/**
		 * What kind of geometry are we -supposed- to be digitising?
		 * Note that what we actually get when the user hits Create may be
		 * different (A LineString with only one point?! That's unpossible.)
		 */
		GeometryType d_geometry_type;

		/**
		 *  FIXME
		 */
		std::vector<GPlatesPropertyValues::GpmlTopologicalSection> d_sections_vector;
		
		/**
		 * What kind of geometry did we successfully build last?
		 * 
		 * This may be boost::none if the digitisation widget has no
		 * (valid) point data yet.
		 *
		 * The kind of geometry we get might not match the user's intention.
		 * For example, if there are not enough points to make a gml:LineString
		 * but there are enough for a gml:Point.
		 * 
		 * If the user were to manage to click a point, then click a point on the
		 * exact opposite side of the globe, they should be congratulated with a
		 * little music and fireworks show (and the geometry will stubbornly refuse
		 * to update, because we can't create a PolylineOnSphere out of it).
		 */
		boost::optional<GPlatesMaths::GeometryOnSphere::non_null_ptr_to_const_type> d_geometry_opt_ptr;
	};
}

#endif  // GPLATES_QTWIDGETS_PLATECLOSUREWIDGET_H

