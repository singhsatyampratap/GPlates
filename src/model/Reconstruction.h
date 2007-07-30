/* $Id$ */

/**
 * \file 
 * Contains the definition of the class Reconstruction.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#ifndef GPLATES_MODEL_RECONSTRUCTION_H
#define GPLATES_MODEL_RECONSTRUCTION_H

#include <vector>
#include "ReconstructedFeatureGeometry.h"
#include "ReconstructionTree.h"
#include "maths/PointOnSphere.h"
#include "maths/PolylineOnSphere.h"
#include "contrib/non_null_intrusive_ptr.h"

namespace GPlatesModel
{
	/**
	 * This class represents a plate-tectonic reconstruction at a particular geological
	 * time-instant.
	 */
	class Reconstruction
	{
	public:
		/**
		 * A convenience typedef for
		 * GPlatesContrib::non_null_intrusive_ptr<Reconstruction>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<Reconstruction> non_null_ptr_type;

		/**
		 * A convenience typedef for
		 * GPlatesContrib::non_null_intrusive_ptr<const Reconstruction>.
		 */
		typedef GPlatesContrib::non_null_intrusive_ptr<const Reconstruction>
				non_null_ptr_to_const_type;

		/**
		 * The type used to store the reference-count of an instance of this class.
		 */
		typedef long ref_count_type;

		~Reconstruction()
		{  }

		/**
		 * Create a new Reconstruction instance.
		 */
		static
		const non_null_ptr_type
		create()
		{
			non_null_ptr_type ptr(*(new Reconstruction));
			return ptr;
		}

		/**
		 * Access the reconstructed point-geometries.
		 */
		std::vector<ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> > &
		point_geometries()
		{
			return d_point_geometries;
		}

		/**
		 * Access the reconstructed polyline-geometries.
		 */
		std::vector<ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> > &
		polyline_geometries()
		{
			return d_polyline_geometries;
		}

		/**
		 * Access the reconstruction tree.
		 */
		ReconstructionTree &
		reconstruction_tree()
		{
			return d_reconstruction_tree;
		}

		/**
		 * Increment the reference-count of this instance.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesContrib::non_null_intrusive_ptr.
		 */
		void
		increment_ref_count() const
		{
			++d_ref_count;
		}

		/**
		 * Decrement the reference-count of this instance, and return the new
		 * reference-count.
		 *
		 * Client code should not use this function!
		 *
		 * This function is used by boost::intrusive_ptr and
		 * GPlatesContrib::non_null_intrusive_ptr.
		 */
		ref_count_type
		decrement_ref_count() const
		{
			return --d_ref_count;
		}

	private:

		/**
		 * The reference-count of this instance by intrusive-pointers.
		 */
		mutable ref_count_type d_ref_count;

		/**
		 * The reconstructed point-geometries.
		 */
		std::vector<ReconstructedFeatureGeometry<GPlatesMaths::PointOnSphere> >
				d_point_geometries;

		/**
		 * The reconstructed polyline-geometries.
		 */
		std::vector<ReconstructedFeatureGeometry<GPlatesMaths::PolylineOnSphere> >
				d_polyline_geometries;

		/**
		 * The plate-reconstruction hierarchy of total reconstruction poles which was used
		 * to reconstruct the geometries.
		 */
		ReconstructionTree d_reconstruction_tree;

		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		Reconstruction():
			d_ref_count(0)
		{  }

		// This constructor should never be defined, because we don't want to allow
		// copy-construction.
		Reconstruction(
				const Reconstruction &other);

		// This operator should never be defined, because we don't want/need to allow
		// copy-assignment:  All copying should use the virtual copy-constructor 'clone'
		// (which will in turn use the copy-constructor); all "assignment" should really
		// only be assignment of one intrusive-pointer to another.
		Reconstruction &
		operator=(
				const Reconstruction &);
	};


	inline
	void
	intrusive_ptr_add_ref(
			const Reconstruction *p)
	{
		p->increment_ref_count();
	}


	inline
	void
	intrusive_ptr_release(
			const Reconstruction *p)
	{
		if (p->decrement_ref_count() == 0) {
			delete p;
		}
	}

}

#endif  // GPLATES_MODEL_RECONSTRUCTION_H