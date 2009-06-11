/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
 * 
 * Copyright (C) 2009 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_APPLOGICUTILS_H
#define GPLATES_APP_LOGIC_APPLOGICUTILS_H

#include "model/FeatureCollectionHandle.h"


namespace GPlatesModel
{
	class FeatureVisitor;
}

namespace GPlatesAppLogic
{
	namespace AppLogicUtils
	{
		/**
		 * A convenience function for iterating over a the features in a
		 * @a GPlatesModel::FeatureCollectionHandle::weak_ref and visiting
		 * them with @a visitor.
		 *
		 * FIXME: Remove this when this functionality is taken care of by
		 * the feature visitor base classes.
		 */
		void
		visit_feature_collection(
				GPlatesModel::FeatureCollectionHandle::weak_ref &feature_collection,
				GPlatesModel::FeatureVisitor &visitor);


		/**
		 * A convenience function for iterating over a sequence of
		 * @a GPlatesModel::FeatureCollectionHandle::weak_ref objects and visiting
		 * them with @a visitor.
		 */
		template< typename FeatureCollectionIterator >
		void
		visit_feature_collections(
				FeatureCollectionIterator collections_begin, 
				FeatureCollectionIterator collections_end,
				GPlatesModel::FeatureVisitor &visitor)
		{
			using namespace GPlatesModel;

			// We visit each of the features in each of the feature collections in
			// the given range.
			FeatureCollectionIterator collections_iter = collections_begin;
			for ( ; collections_iter != collections_end; ++collections_iter)
			{
				FeatureCollectionHandle::weak_ref feature_collection = *collections_iter;

				// FIXME: change this to visitor.visit(feature_collection) when
				// new functionality added to feature visitor base classes.
				visit_feature_collection(feature_collection, visitor);
			}
		}
	}
}

#endif // GPLATES_APP_LOGIC_APPLOGICUTILS_H
