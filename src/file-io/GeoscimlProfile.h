/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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

#ifndef GPLATES_FILEIO_GEOSCIMLPROFILE_H
#define GPLATES_FILEIO_GEOSCIMLPROFILE_H

#include <QString>
#include <QXmlItem>

#include "ArbitraryXmlProfile.h"
#include "model/FeatureCollectionHandle.h"

namespace GPlatesFileIO
{
	class GeoscimlProfile :
		public ArbitraryXmlProfile
	{
	public:
		GeoscimlProfile()
		{
			init();
		}

		explicit
		GeoscimlProfile(
				const QString& profile_name)
		{
			init();
		}
		
		void
		populate(
				const File::Reference& xml_file);

		void
		populate(
				QByteArray& xml_data,
				GPlatesModel::FeatureCollectionHandle::weak_ref fch);
				
	protected:
		inline
		void
		init()
		{ }
		GeoscimlProfile(
					const GeoscimlProfile&);
	};
}

#endif  // GPLATES_FILEIO_GEOSCIMLPROFILE_H