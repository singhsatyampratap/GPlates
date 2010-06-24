/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, 2009 The University of Sydney, Australia
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

#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRYVISITOR_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRYVISITOR_H

#include "utils/CopyConst.h"
#include "utils/non_null_intrusive_ptr.h"


namespace GPlatesAppLogic
{
	class ReconstructionGeometry;

	// Forward declaration of template visitor class.
	template <class ReconstructionGeometryType>
	class ReconstructionGeometryVisitorBase;


	/**
	 * Typedef for visitor over non-const @a ReconstructionGeometry objects.
	 */
	typedef ReconstructionGeometryVisitorBase<ReconstructionGeometry>
			ReconstructionGeometryVisitor;

	/**
	 * Typedef for visitor over const @a ReconstructionGeometry objects.
	 */
	typedef ReconstructionGeometryVisitorBase<const ReconstructionGeometry>
			ConstReconstructionGeometryVisitor;


	// Forward declarations of ReconstructionGeometry derived types.
	class ReconstructedFeatureGeometry;
	class ReconstructedVirtualGeomagneticPole;
	class ResolvedTopologicalBoundary;
	class ResolvedTopologicalNetwork;


	/**
	 * This class defines an abstract interface for a Visitor to visit reconstruction
	 * geometries.
	 *
	 * See the Visitor pattern (p.331) in Gamma95 for more information on the design and
	 * operation of this class.  This class corresponds to the abstract Visitor class in the
	 * pattern structure.
	 *
	 * @par Notes on the class implementation:
	 *  - All the virtual member "visit" functions have (empty) definitions for convenience, so
	 *    that derivations of this abstract base need only override the "visit" functions which
	 *    interest them.
	 */
	template <class ReconstructionGeometryType>
	class ReconstructionGeometryVisitorBase
	{
	public:
		//! Typedef for @a ReconstructedFeatureGeometry of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				ReconstructionGeometryType, ReconstructedFeatureGeometry>::type
						reconstructed_feature_geometry_type;

		//! Typedef for @a ReconstructedFeatureGeometry of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				ReconstructionGeometryType, ReconstructedVirtualGeomagneticPole>::type
						reconstructed_virtual_geomagnetic_pole_type;

		//! Typedef for @a ResolvedTopologicalBoundary of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				ReconstructionGeometryType, ResolvedTopologicalBoundary>::type
						resolved_topological_boundary_type;

		//! Typedef for @a ResolvedTopologicalNetwork of appropriate const-ness.
		typedef typename GPlatesUtils::CopyConst<
				ReconstructionGeometryType, ResolvedTopologicalNetwork>::type
						resolved_topological_network_type;


		// We'll make this function pure virtual so that the class is abstract.  The class
		// *should* be abstract, but wouldn't be unless we did this, since all the virtual
		// member functions have (empty) definitions.
		virtual
		~ReconstructionGeometryVisitorBase() = 0;

		// Please keep these reconstruction geometry derivations ordered alphabetically.

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_feature_geometry_type> &rfg)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<reconstructed_virtual_geomagnetic_pole_type> &rvgp)
		{  }


		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_boundary_type> &rtb)
		{  }

		/**
		 * Override this function in your own derived class.
		 */
		virtual
		void
		visit(
				const GPlatesUtils::non_null_intrusive_ptr<resolved_topological_network_type> &rtn)
		{  }

	private:

		// This operator should never be defined, because we don't want to allow
		// copy-assignment.
		ReconstructionGeometryVisitorBase &
		operator=(
				const ReconstructionGeometryVisitorBase &);

	};


	template <class ReconstructionGeometryType>
	inline
	ReconstructionGeometryVisitorBase<ReconstructionGeometryType>::~ReconstructionGeometryVisitorBase()
	{  }

}

#endif  // GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRYVISITOR_H