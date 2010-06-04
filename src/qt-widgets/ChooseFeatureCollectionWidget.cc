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

#include <QListWidget>
#include <QListWidgetItem>
#include <QString>

#include "ChooseFeatureCollectionWidget.h"

#include "app-logic/FeatureCollectionFileIO.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "model/FeatureCollectionHandle.h"


namespace
{
	/**
	 * Subclass of QListWidgetItem so that we can display a list of FeatureCollection in
	 * the list widget using the filename as the label, while keeping track of which
	 * list item corresponds to which FeatureCollection.
	 */
	class FeatureCollectionItem :
			public QListWidgetItem
	{
	public:
		// Standard constructor for creating FeatureCollection entry.
		FeatureCollectionItem(
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_iter,
				const QString &label):
			QListWidgetItem(label),
			d_file_iter(file_iter)
		{  }

		// Constructor for creating fake "Make a new Feature Collection" entry.
		FeatureCollectionItem(
				const QString &label):
			QListWidgetItem(label)
		{  }
			
		bool
		is_create_new_collection_item()
		{
			return !d_file_iter;
		}

		/**
		 * NOTE: Check with @a is_create_new_collection_item first and set a valid file
		 * iterator if necessary before calling this method.
		 */
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator
		get_file_iterator()
		{
			GPlatesGlobal::Assert<GPlatesGlobal::AssertionFailureException>(
					d_file_iter, GPLATES_ASSERTION_SOURCE);

			return *d_file_iter;
		}

		void
		set_file_iterator(
				GPlatesAppLogic::FeatureCollectionFileState::file_iterator file_iter)
		{
			d_file_iter = file_iter;
		}
	
	private:
		boost::optional<GPlatesAppLogic::FeatureCollectionFileState::file_iterator> d_file_iter;
	};


	/**
	 * Fill the list with currently loaded FeatureCollections we can add the feature to.
	 */
	void
	populate_feature_collections_list(
			QListWidget &list_widget,
			GPlatesAppLogic::FeatureCollectionFileState &state)
	{
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator_range it_range =
				state.get_loaded_files();
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator it = it_range.begin;
		GPlatesAppLogic::FeatureCollectionFileState::file_iterator end = it_range.end;
		
		list_widget.clear();
		for (; it != end; ++it) {
			// Get the FeatureCollectionHandle for this file.
			GPlatesModel::FeatureCollectionHandle::weak_ref collection_opt =
					it->get_feature_collection();

			// Some files might not actually exist yet if the user created a new
			// feature collection internally and hasn't saved it to file yet.
			QString label;
			if (GPlatesFileIO::file_exists(it->get_file_info()))
			{

				// Get a suitable label; we will prefer the full filename.
				label = it->get_file_info().get_display_name(true);
			}
			else
			{
				// The file doesn't exist so give it a filename to indicate this.
				label = "New Feature Collection";
			}
			
			// We are only interested in loaded files which have valid FeatureCollections.
			if (collection_opt.is_valid()) {
				list_widget.addItem(new FeatureCollectionItem(it, label));
			}
		}
		// Add a final option for creating a brand new FeatureCollection.
		list_widget.addItem(new FeatureCollectionItem(QObject::tr(" < Create a new feature collection > ")));
		// Default to first entry.
		list_widget.setCurrentRow(0);
	}
}


GPlatesQtWidgets::ChooseFeatureCollectionWidget::ChooseFeatureCollectionWidget(
		GPlatesAppLogic::FeatureCollectionFileState &file_state,
		GPlatesAppLogic::FeatureCollectionFileIO &file_io,
		QWidget *parent_) :
	QGroupBox(parent_),
	d_file_state(file_state),
	d_file_io(file_io)
{
	setupUi(this);

	listwidget_feature_collections->setFocus();

	// Emit signal if the user pushes Enter or double-clicks on the list.
	QObject::connect(
			listwidget_feature_collections,
			SIGNAL(itemActivated(QListWidgetItem *)),
			this,
			SLOT(handle_listwidget_item_activated(QListWidgetItem *)));
}


void
GPlatesQtWidgets::ChooseFeatureCollectionWidget::initialise()
{
	populate_feature_collections_list(*listwidget_feature_collections, d_file_state);
}


void
GPlatesQtWidgets::ChooseFeatureCollectionWidget::handle_listwidget_item_activated(
		QListWidgetItem *)
{
	emit item_activated();
}


std::pair<GPlatesAppLogic::FeatureCollectionFileState::file_iterator, bool>
GPlatesQtWidgets::ChooseFeatureCollectionWidget::get_file_iterator() const
{
	FeatureCollectionItem *collection_item = dynamic_cast<FeatureCollectionItem *>(
			listwidget_feature_collections->currentItem());
	if (collection_item)
	{
		if (collection_item->is_create_new_collection_item())
		{
			return std::make_pair(d_file_io.create_empty_file(), true);
		}
		else
		{
			return std::make_pair(collection_item->get_file_iterator(), false);
		}
	}
	else
	{
		throw NoFeatureCollectionSelectedException();
	}
}
