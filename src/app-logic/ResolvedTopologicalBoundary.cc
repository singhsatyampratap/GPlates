/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
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

#include "ResolvedTopologicalBoundary.h"
#include "ReconstructionGeometryVisitor.h"

#include "model/WeakObserverVisitor.h"

#include "global/AssertionFailureException.h"
#include "global/GPlatesAssert.h"

#include "utils/GeometryCreationUtils.h"


const GPlatesModel::FeatureHandle::weak_ref
GPlatesAppLogic::ResolvedTopologicalBoundary::get_feature_ref() const
{
	if (is_valid()) {
		return feature_handle_ptr()->reference();
	} else {
		return GPlatesModel::FeatureHandle::weak_ref();
	}
}


void
GPlatesAppLogic::ResolvedTopologicalBoundary::accept_visitor(
		ConstReconstructionGeometryVisitor &visitor) const
{
	visitor.visit(this->get_non_null_pointer_to_const());
}


void
GPlatesAppLogic::ResolvedTopologicalBoundary::accept_visitor(
		ReconstructionGeometryVisitor &visitor)
{
	visitor.visit(this->get_non_null_pointer());
}


void
GPlatesAppLogic::ResolvedTopologicalBoundary::accept_weak_observer_visitor(
		GPlatesModel::WeakObserverVisitor<GPlatesModel::FeatureHandle> &visitor)
{
	visitor.visit_resolved_topological_boundary(*this);
}