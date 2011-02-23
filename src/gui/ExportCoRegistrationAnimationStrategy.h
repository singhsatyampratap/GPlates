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
 
#ifndef GPLATES_GUI_EXPORTCOREGISTRATIONANIMATIONSTRATEGY_H
#define GPLATES_GUI_EXPORTCOREGISTRATIONANIMATIONSTRATEGY_H

#include <boost/optional.hpp>
#include <boost/none.hpp>
#include <QString>

#include "gui/ExportAnimationStrategy.h"
#include "data-mining/DataTable.h"
#include "utils/ExportTemplateFilenameSequence.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/NullIntrusivePointerHandler.h"
#include "utils/ReferenceCount.h"

namespace GPlatesGui
{
	class ExportAnimationContext;

	/**
	 * Concrete implementation of the ExportAnimationStrategy class for 
	 * writing co-registration data at each timestep.
	 */
	class ExportCoRegistrationAnimationStrategy:
			public GPlatesGui::ExportAnimationStrategy
	{
	public:

 		static const QString DEFAULT_FILENAME_TEMPLATE;
		static const QString FILENAME_TEMPLATE_DESC;
		static const QString CO_REGISTRATION_DESC;
		
		typedef GPlatesUtils::non_null_intrusive_ptr<ExportCoRegistrationAnimationStrategy,
				GPlatesUtils::NullIntrusivePointerHandler> non_null_ptr_type;
		
		static
		const non_null_ptr_type
		create(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const ExportAnimationStrategy::Configuration &cfg=
					ExportAnimationStrategy::Configuration(
							DEFAULT_FILENAME_TEMPLATE));

		virtual
		~ExportCoRegistrationAnimationStrategy()
		{  }

		/**
		 * Does one frame of export. Called by the ExportAnimationContext.
		 * @param frame_index - the frame we are to export this round, indexed from 0.
		 */
		virtual
		bool
		do_export_iteration(
				std::size_t frame_index);

		virtual
		const QString&
		get_default_filename_template();

		virtual
		const QString&
		get_filename_template_desc();

		virtual
		const QString&
				get_description()
		{
			return CO_REGISTRATION_DESC;
		}

	protected:
		/**
		 * Protected constructor to prevent instantiation on the stack.
		 * Use the create() method on the individual Strategy subclasses.
		 */
		explicit
		ExportCoRegistrationAnimationStrategy(
				GPlatesGui::ExportAnimationContext &export_animation_context,
				const QString &filename_template);
		
	private:
		ExportCoRegistrationAnimationStrategy();
		char d_delimiter;
		
	};
}


#endif //GPLATES_GUI_EXPORTCOREGISTRATIONANIMATIONSTRATEGY_H

