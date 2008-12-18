/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#ifndef GPLATES_FEATUREVISITORS_FEATURECOLLECTIONCLASSIFIER_H
#define GPLATES_FEATUREVISITORS_FEATURECOLLECTIONCLASSIFIER_H


#include <boost/optional.hpp>
#include <vector>
#include "model/PropertyName.h"

#include "model/ConstFeatureVisitor.h"
#include "model/FeatureCollectionHandle.h"


namespace GPlatesFeatureVisitors
{
	/**
	 * This const feature visitor can be applied to all the features in
	 * a FeatureCollection and accumulates a summary of the kind of
	 * FeatureCollection the user is dealing with.
	 * 
	 * Remember that GPlates uses Duck typing and it is quite possible
	 * for the user to craft data which does not resemble anything
	 * that neatly fits into the normal categories of Feature.
	 */
	class FeatureCollectionClassifier:
			public GPlatesModel::ConstFeatureVisitor
	{
	public:
		FeatureCollectionClassifier();

		virtual
		~FeatureCollectionClassifier()
		{  }
		
		
		/**
		 * Iterates over every feature in the feature collection to
		 * produce the summary of feature types. This is the recommended
		 * way to use this visitor.
		 */
		void
		scan_feature_collection(
				GPlatesModel::FeatureCollectionHandle::const_weak_ref feature_collection_ref);
		
		
		/**
		 * Returns the count of features seen by the visitor which
		 * appear to be 'reconstruction' features.
		 */
		int
		reconstruction_feature_count()
		{
			return d_reconstruction_feature_count;
		}


		/**
		 * Returns the count of features seen by the visitor which
		 * appear to be 'reconstructable' features.
		 */
		int
		reconstructable_feature_count()
		{
			return d_reconstructable_feature_count;
		}


		/**
		 * Returns the count of features seen by the visitor which
		 * appear to be 'instantaneous' features.
		 */
		int
		instantaneous_feature_count()
		{
			return d_instantaneous_feature_count;
		}


		/**
		 * Returns the total number of features seen by the visitor.
		 */
		int
		total_feature_count()
		{
			return d_total_feature_count;
		}
		
		
		/**
		 * Resets the state of the visitor, allowing the same instance
		 * to be re-used.
		 */
		void
		reset();
		

		virtual
		void
		visit_feature_handle(
				const GPlatesModel::FeatureHandle &feature_handle);

		virtual
		void
		visit_inline_property_container(
				const GPlatesModel::InlinePropertyContainer &inline_property_container);

		virtual
		void
		visit_gml_time_instant(
				const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant);

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value);

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id);

	private:
		std::vector<GPlatesModel::PropertyName> d_property_names_to_allow;
		boost::optional<GPlatesModel::PropertyName> d_most_recent_propname_read;
		
		bool d_looks_like_reconstruction_feature;
		bool d_looks_like_reconstructable_feature;
		bool d_looks_like_instantaneous_feature;
		
		int d_reconstruction_feature_count;
		int d_reconstructable_feature_count;
		int d_instantaneous_feature_count;
		int d_total_feature_count;
	};
}


#endif	// GPLATES_FEATUREVISITORS_FEATURECOLLECTIONCLASSIFIER_H