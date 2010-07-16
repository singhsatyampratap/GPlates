/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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
 

#include <QFileInfo>
#include <QString>
#include "ExportVelocityAnimationStrategy.h"

#include "app-logic/AppLogicUtils.h"

#include "file-io/GpmlOnePointSixOutputVisitor.h"

#include "global/NotYetImplementedException.h"

#include "gui/ExportAnimationContext.h"
#include "gui/AnimationController.h"

#include "presentation/ViewState.h"

#include "utils/FloatingPointComparisons.h"


namespace
{
	QString
	substitute_placeholder(
			const QString &output_filebasename,
			const QString &placeholder,
			const QString &placeholder_replacement)
	{
		return QString(output_filebasename).replace(placeholder, placeholder_replacement);
	}

	QString
	calculate_output_basename(
			const QString &output_filename_prefix,
			const QFileInfo &cap_qfileinfo)
	{
#if 0	
		//remove the cap file extension name 
		QString cap_filename = cap_qfileinfo.fileName();
		int idx = cap_filename.lastIndexOf(".gpml",-1);
		if(-1 != idx)
		{
			cap_filename = cap_filename.left(idx);
		}
#endif

		const QString output_basename = substitute_placeholder(
				output_filename_prefix,
				GPlatesUtils::ExportTemplateFilename::PLACEHOLDER_FORMAT_STRING,
				cap_qfileinfo.fileName());
				//cap_filename);

		return output_basename;
	}
}

const QString 
GPlatesGui::ExportVelocityAnimationStrategy::DEFAULT_MESH_VILOCITIES_FILENAME_TEMPLATE
		="velocity_colat+lon_at_%u_%0.2fMa_on_mesh-%P.gpml";
const QString 
GPlatesGui::ExportVelocityAnimationStrategy::MESH_VILOCITIES_FILENAME_TEMPLATE_DESC
		=FORMAT_CODE_DESC;
const QString 
GPlatesGui::ExportVelocityAnimationStrategy::MESH_VILOCITIES_DESC
		="Export velocity data.";

const GPlatesGui::ExportVelocityAnimationStrategy::non_null_ptr_type
GPlatesGui::ExportVelocityAnimationStrategy::create(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const ExportAnimationStrategy::Configuration& cfg)
{
	ExportVelocityAnimationStrategy * ptr=
			new ExportVelocityAnimationStrategy(export_animation_context,
			cfg.filename_template()); 
	
	ptr->d_class_id="MESH_VILOCITIES_GPML";
	return non_null_ptr_type(
			ptr,
			GPlatesUtils::NullIntrusivePointerHandler());
}

GPlatesGui::ExportVelocityAnimationStrategy::ExportVelocityAnimationStrategy(
		GPlatesGui::ExportAnimationContext &export_animation_context,
		const QString filename_template):
	ExportAnimationStrategy(export_animation_context)
{
	set_template_filename(filename_template);
}

void
GPlatesGui::ExportVelocityAnimationStrategy::set_template_filename(
		const QString &filename)
{
// 	TODO: temporary thing. the template needs to be sorted out completely later.
// 						ExportAnimationStrategy::set_template_filename(
// 								filename.toStdString().substr(
// 										0,
// 										filename.toStdString().find_first_of("<")).c_str());
	ExportAnimationStrategy::set_template_filename(filename);
}

bool
GPlatesGui::ExportVelocityAnimationStrategy::do_export_iteration(
		std::size_t frame_index)
{
	if(!check_filename_sequence())
	{
		return false;
	}
	GPlatesUtils::ExportTemplateFilenameSequence::const_iterator &filename_it = 
		*d_filename_iterator_opt;

	// Assemble parts of this iteration's filename from the template filename sequence.
	QString output_filename_prefix = *filename_it++;
	QDir target_dir = d_export_animation_context_ptr->target_dir();

#if 1
	throw GPlatesGlobal::NotYetImplementedException(GPLATES_EXCEPTION_SOURCE);
#else
	// Here's where we would do the actual calculating and exporting of the
	// velocities. The View is already set to the appropriate reconstruction time for
	// this frame; all we have to do is the maths and the file-writing (to @a full_filename)
	//
	const GPlatesAppLogic::PlateVelocityWorkflow &plate_velocity_workflow =
			d_export_animation_context_ptr->view_state().get_plate_velocity_workflow();
	
	// Iterate over the velocity feature collections currently being solved.
	const unsigned int num_velocity_feature_collections =
			plate_velocity_workflow.get_num_velocity_feature_collections();
	for (unsigned int velocity_index = 0;
		velocity_index < num_velocity_feature_collections;
		++velocity_index)
	{
		// Get cap file information, work out filenames we will use.
		const GPlatesFileIO::FileInfo &velocity_file_info =
				plate_velocity_workflow.get_velocity_file_info(velocity_index);
		const QString &velocity_filename = velocity_file_info.get_qfileinfo().absoluteFilePath();
		//QString cap_display_name = velocity_filename.fileName();
		QString output_basename = calculate_output_basename(output_filename_prefix, velocity_filename);
		QString full_output_filename = target_dir.absoluteFilePath(output_basename);

#if 0
		// First, the computing (unless this will already have been handled?)
		// Update the dialog status message.
		d_export_animation_context_ptr->update_status_message(
				QObject::tr("Processing file \"%1\"...")
				.arg(cap_display_name));
#endif

		// Next, the file writing. Update the dialog status message.
		d_export_animation_context_ptr->update_status_message(
				QObject::tr("Writing mesh velocities at frame %2 to file \"%1\"...")
				.arg(output_basename)
				.arg(frame_index) );

		//
		// Output the velocity feature collection
		//
		GPlatesModel::FeatureCollectionHandle::const_weak_ref velocity_feature_collection =
				plate_velocity_workflow.get_velocity_feature_collection(velocity_index);
		GPlatesFileIO::FileInfo export_file_info(full_output_filename);
		GPlatesFileIO::GpmlOnePointSixOutputVisitor gpml_writer(export_file_info, false);
		GPlatesAppLogic::AppLogicUtils::visit_feature_collection(
				velocity_feature_collection, gpml_writer);
	}
#endif
	
	// Normal exit, all good, ask the Context to process the next iteration please.
	return true;
}


void
GPlatesGui::ExportVelocityAnimationStrategy::wrap_up(
		bool export_successful)
{
	// If we need to do anything after writing a whole batch of velocity files,
	// here's the place to do it.
	// Of course, there's also the destructor, which should free up any resources
	// we acquired in the constructor; this method is intended for any "last step"
	// iteration operations that might need to occur. Perhaps all iterations end
	// up in the same file and we should close that file (if all steps completed
	// successfully).
}

const QString&
GPlatesGui::ExportVelocityAnimationStrategy::get_default_filename_template()
{
	return DEFAULT_MESH_VILOCITIES_FILENAME_TEMPLATE;
}

const QString&
GPlatesGui::ExportVelocityAnimationStrategy::get_filename_template_desc()
{
	return MESH_VILOCITIES_FILENAME_TEMPLATE_DESC;
}


