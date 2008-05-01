/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006, 2007 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_CONSTFEATUREVISITOR_H
#define GPLATES_MODEL_CONSTFEATUREVISITOR_H


namespace GPlatesPropertyValues
{
	// Forward declarations for the member functions.
	// Please keep these ordered alphabetically.
	class Enumeration;
	class GmlLineString;
	class GmlOrientableCurve;
	class GmlPoint;
	class GmlTimeInstant;
	class GmlTimePeriod;
	class GpmlConstantValue;
	class GpmlFeatureReference;
	class GpmlFiniteRotation;
	class GpmlFiniteRotationSlerp;
	class GpmlHotSpotTrailMark;
	class GpmlIrregularSampling;
	class GpmlMeasure;
	class GpmlOldPlatesHeader;
	class GpmlPiecewiseAggregation;
	class GpmlPlateId;
	class GpmlPolarityChronId;
	class GpmlRevisionId;
	class GpmlTimeSample;
	class GpmlTimeWindow;
	class UninterpretedPropertyValue;
	class XsBoolean;
	class XsDouble;
	class XsInteger;
	class XsString;
}

namespace GPlatesModel
{
	// Forward declarations for the member functions.
	class FeatureHandle;
	class InlinePropertyContainer;

	/**
	 * This class defines an abstract interface for a Visitor to visit const features.
	 *
	 * See the Visitor pattern (p.331) in Gamma95 for more information on the design and
	 * operation of this class.  This class corresponds to the abstract Visitor class in the
	 * pattern structure.
	 *
	 * @par The applicability of this class:
	 * This Visitor is actually less applicable than you might initially think, since there are
	 * relatively few situations in which you actually want to treat features as const:  Not
	 * only will the feature be const when you are iterating through the feature collection or
	 * traversing the structure of the feature, it will also be const as the target of any
	 * caching references which were established during the iteration or traversal.
	 *  - For example, you might think of a "find" operation as an ideal situation in which to
	 *    treat features as const, since the "find" operation should not modify any of the
	 *    features.  However, the purpose of the "find" operation is to return a reference to
	 *    the matching feature(s), which may then be "highlighted" or "selected" in the GUI;
	 *    the user might then wish to modify one of these features for which he has searched,
	 *    which would not be possible if the feature were const.
	 *  - Similarly, the interpolation of total reconstruction sequences or the reconstruction
	 *    of reconstructable features might seem like non-modifying operations.  However, again
	 *    the user is presented with a proxy value (a node in a reconstruction tree or a
	 *    reconstructed feature geometry which is drawn on-screen) with which he will wish to
	 *    interact, which will result in modification of the original features.
	 *  - The writing of features to file for "save" operations or debugging purposes seems to
	 *    be one of the few situations in which features really can be iterated and traversed
	 *    as const objects.  The const-ness of the features is a useful aspect in this regard,
	 *    to ensure that the features are not changed during the writing.
	 *  - In general, you may find the FeatureVisitor class more applicable.
	 *
	 * @par Notes on the class implementation:
	 *  - All the virtual member "visit" functions have (empty) definitions for convenience, so
	 *    that derivations of this abstract base need only override the "visit" functions which
	 *    interest them.
	 *  - The "visit" functions explicitly include the name of the target type in the function
	 *    name, to avoid the problem of name hiding in derived classes.  (If you declare *any*
	 *    functions named 'f' in a derived class, the declaration will hide *all* functions
	 *    named 'f' in the base class; if all the "visit" functions were simply called 'visit'
	 *    and you wanted to override *any* of them in a derived class, you would have to
	 *    override them all.)
	 */
	class ConstFeatureVisitor {

	public:

		// We'll make this function pure virtual so that the class is abstract.  The class
		// *should* be abstract, but wouldn't be unless we did this, since all the virtual
		// member functions have (empty) definitions.
		virtual
		~ConstFeatureVisitor() = 0;

		/**
		 * Override this function in your own derived class.
		 *
		 * When you override this function in your own derived class, don't forget to
		 * invoke @a visit_feature_properties in the function body, to visit each of the
		 * properties in turn.
		 */
		virtual
		void
		visit_feature_handle(
				const FeatureHandle &feature_handle)
		{
			visit_feature_properties(feature_handle);
		}

		/**
		 * Invoke this function in @a visit_feature_handle to visit each of the the feature
		 * properties in turn.
		 *
		 * Note that this function is not virtual.  This function should not be overridden.
		 */
		void
		visit_feature_properties(
				const FeatureHandle &feature_handle);

		/**
		 * Override this function in your own derived class.
		 *
		 * When you override this function in your own derived class, don't forget to
		 * invoke @a visit_property_values in the function body, to visit each of the
		 * property-values in turn.
		 */
		virtual
		void
		visit_inline_property_container(
				const InlinePropertyContainer &inline_property_container)
		{
			visit_property_values(inline_property_container);
		}

		/**
		 * Invoke this function in @a visit_inline_property_container to visit each of the
		 * property-values in turn.
		 *
		 * Note that this function is not virtual.  This function should not be overridden.
		 */
		void
		visit_property_values(
				const InlinePropertyContainer &inline_property_container);

		// Please keep these property-value types ordered alphabetically.

		virtual
		void
		visit_enumeration(
				const GPlatesPropertyValues::Enumeration &enumeration) {  }

		virtual
		void
		visit_gml_line_string(
				const GPlatesPropertyValues::GmlLineString &gml_line_string) {  }

		virtual
		void
		visit_gml_orientable_curve(
				const GPlatesPropertyValues::GmlOrientableCurve &gml_orientable_curve) {  }

		virtual
		void
		visit_gml_point(
				const GPlatesPropertyValues::GmlPoint &gml_point) {  }

		virtual
		void
		visit_gml_time_instant(
				const GPlatesPropertyValues::GmlTimeInstant &gml_time_instant) {  }

		virtual
		void
		visit_gml_time_period(
				const GPlatesPropertyValues::GmlTimePeriod &gml_time_period) {  }

		virtual
		void
		visit_gpml_constant_value(
				const GPlatesPropertyValues::GpmlConstantValue &gpml_constant_value) {  }

		virtual
		void
		visit_gpml_feature_reference(
				const GPlatesPropertyValues::GpmlFeatureReference &gpml_feature_reference) {  }

		virtual
		void
		visit_gpml_finite_rotation(
				const GPlatesPropertyValues::GpmlFiniteRotation &gpml_finite_rotation) {  }

		virtual
		void
		visit_gpml_finite_rotation_slerp(
				const GPlatesPropertyValues::GpmlFiniteRotationSlerp &gpml_finite_rotation_slerp) {  }

		virtual
		void
		visit_gpml_hot_spot_trail_mark(
				const GPlatesPropertyValues::GpmlHotSpotTrailMark &gpml_hot_spot_trail_mark) {  }

		virtual
		void
		visit_gpml_irregular_sampling(
				const GPlatesPropertyValues::GpmlIrregularSampling &gpml_irregular_sampling) {  }

		virtual
		void
		visit_gpml_measure(
				const GPlatesPropertyValues::GpmlMeasure &gpml_measure) {  }

		virtual
		void
		visit_gpml_old_plates_header(
				const GPlatesPropertyValues::GpmlOldPlatesHeader &gpml_old_plates_header) {  }

		virtual
		void
		visit_gpml_piecewise_aggregation(
				const GPlatesPropertyValues::GpmlPiecewiseAggregation &gpml_piecewise_aggregation) 
		{  }

		virtual
		void
		visit_gpml_plate_id(
				const GPlatesPropertyValues::GpmlPlateId &gpml_plate_id) {  }

		virtual
		void
		visit_gpml_polarity_chron_id(
				const GPlatesPropertyValues::GpmlPolarityChronId &gpml_polarity_chron_id) {  }

		virtual
		void
		visit_gpml_revision_id(
				const GPlatesPropertyValues::GpmlRevisionId &gpml_revision_id) {  }

		virtual
		void
		visit_gpml_time_sample(
				const GPlatesPropertyValues::GpmlTimeSample &gpml_time_sample) {  }

		virtual
		void
		visit_gpml_time_window(
				const GPlatesPropertyValues::GpmlTimeWindow &gpml_time_window) {  }

		virtual
		void
		visit_uninterpreted_property_value(
				const GPlatesPropertyValues::UninterpretedPropertyValue &uninterpreted_prop_val) 
		{  }

		virtual
		void
		visit_xs_boolean(
				const GPlatesPropertyValues::XsBoolean &xs_boolean) {  }

		virtual
		void
		visit_xs_double(
				const GPlatesPropertyValues::XsDouble &xs_double) {  }

		virtual
		void
		visit_xs_integer(
				const GPlatesPropertyValues::XsInteger &xs_integer) {  }


		virtual
		void
		visit_xs_string(
				const GPlatesPropertyValues::XsString &xs_string) {  }

	private:

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		ConstFeatureVisitor &
		operator=(
				const ConstFeatureVisitor &);

	};

}

#endif  // GPLATES_MODEL_CONSTFEATUREVISITOR_H
