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

#include <boost/foreach.hpp>

#include "VisualLayer.h"

#include "ReconstructionGeometryRenderer.h"
#include "VisualLayers.h"

#include "app-logic/ApplicationState.h"
#include "app-logic/Layer.h"
#include "app-logic/ReconstructionGeometryCollection.h"

#include "file-io/FileInfo.h"


GPlatesPresentation::VisualLayer::VisualLayer(
		VisualLayers &visual_layers,
		const GPlatesAppLogic::Layer &layer,
		GPlatesViewOperations::RenderedGeometryCollection &rendered_geometry_collection,
		int layer_number) :
	d_layer(layer),
	// Create a child rendered geometry layer in the main RECONSTRUCTION layer.
	d_rendered_geometry_layer_index(
			rendered_geometry_collection.create_child_rendered_layer(
				GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER)),
	d_rendered_geometry_layer(
			rendered_geometry_collection.transfer_ownership_of_child_rendered_layer(
				d_rendered_geometry_layer_index,
				GPlatesViewOperations::RenderedGeometryCollection::RECONSTRUCTION_LAYER)),
	d_visual_layers(visual_layers),
	d_expanded(false),
	d_visible(true),
	d_layer_number(layer_number)
{
}


void
GPlatesPresentation::VisualLayer::create_rendered_geometries()
{
	// Delay any notification of changes to the rendered geometry collection
	// until end of current scope block. This is so we can do multiple changes
	// without any canvas' redrawing themselves after each change.
	// This should ideally be located at the highest level to capture one
	// user GUI interaction - the user performs an action and we update canvas once.
	// But since these guards can be nested it's probably a good idea to have it here too.
	GPlatesViewOperations::RenderedGeometryCollection::UpdateGuard update_guard;

	// Activate the layer.
	d_rendered_geometry_layer->set_active();

	// Clear all RenderedGeometry's before adding new ones.
	d_rendered_geometry_layer->clear_rendered_geometries();	

	// Don't create RenderedGeometry's if hidden.
	if (!d_visible)
	{
		return;
	}

	// Get the most recent output data generated by this layer and see if it contains
	// reconstruction geometries.
	boost::optional<GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type>
			reconstruction_geometry_collection = d_layer.get_output_data<
					GPlatesAppLogic::ReconstructionGeometryCollection::non_null_ptr_to_const_type>();
	if (!reconstruction_geometry_collection)
	{
		// Return with an empty rendered geometry layer.
		return;
	}

	// This creates the RenderedGeometry's from the ReconstructionGeometry's.
	GPlatesPresentation::ReconstructionGeometryRenderer reconstruction_geometry_renderer(
			*d_rendered_geometry_layer);

	// Iterate over the reconstruction geometries in the collection.
	GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator reconstruction_geometry_iter =
			reconstruction_geometry_collection.get()->begin();
	GPlatesAppLogic::ReconstructionGeometryCollection::const_iterator reconstruction_geometry_end =
			reconstruction_geometry_collection.get()->end();
	for ( ; reconstruction_geometry_iter != reconstruction_geometry_end; ++reconstruction_geometry_iter)
	{
		GPlatesAppLogic::ReconstructionGeometry::non_null_ptr_to_const_type reconstruction_geometry =
				*reconstruction_geometry_iter;

		reconstruction_geometry->accept_visitor(reconstruction_geometry_renderer);
	}
}


bool
GPlatesPresentation::VisualLayer::is_expanded() const
{
	return d_expanded;
}


void
GPlatesPresentation::VisualLayer::set_expanded(
		bool expanded)
{
	if (expanded != d_expanded)
	{
		d_expanded = expanded;
		emit_layer_modified();
	}
}


void
GPlatesPresentation::VisualLayer::toggle_expanded()
{
	d_expanded = !d_expanded;
	emit_layer_modified();
}


bool
GPlatesPresentation::VisualLayer::is_visible() const
{
	return d_visible;
}


void
GPlatesPresentation::VisualLayer::set_visible(
		bool visible)
{
	if (visible != d_visible)
	{
		d_visible = visible;
		emit_layer_modified();
		
		// Clear the rendered geometries, or re-create rendered geometries, as needed.
		create_rendered_geometries();
	}
}


void
GPlatesPresentation::VisualLayer::toggle_visible()
{
	d_visible = !d_visible;
	emit_layer_modified();

	// Clear the rendered geometries, or re-create rendered geometries, as needed.
	create_rendered_geometries();
}


QString
GPlatesPresentation::VisualLayer::get_generated_name() const
{
	// Get the (feature collection) inputs on the main channel.
	typedef GPlatesAppLogic::Layer::InputConnection InputConnection;
	QString main_channel = d_layer.get_main_input_feature_collection_channel();
	std::vector<InputConnection> inputs = d_layer.get_channel_inputs(main_channel);

	// Use the first input that has an InputFile.
	typedef GPlatesAppLogic::Layer::InputFile InputFile;
	QString result;
	BOOST_FOREACH(InputConnection &input_connection, inputs)
	{
		boost::optional<InputFile> input_file = input_connection.get_input_file();
		if (input_file)
		{
			result = input_file->get_file_info().get_file_name_without_extension();
			break;
		}
	}

	// If we can't use an InputFile's name, then we generate a name from the layer number.
	if (result.isEmpty())
	{
		result = QString("Layer %1").arg(d_layer_number);
	}

	return result;
}


const boost::optional<QString> &
GPlatesPresentation::VisualLayer::get_custom_name() const
{
	return d_custom_name;
}


void
GPlatesPresentation::VisualLayer::set_custom_name(
		const boost::optional<QString> &custom_name)
{
	d_custom_name = custom_name;
}


QString
GPlatesPresentation::VisualLayer::get_name() const
{
	if (d_custom_name)
	{
		return *d_custom_name;
	}
	else
	{
		return get_generated_name();
	}
}


void
GPlatesPresentation::VisualLayer::emit_layer_modified()
{
	// Emit signal via VisualLayers.
	d_visual_layers.emit_layer_modified(d_rendered_geometry_layer_index);
}
