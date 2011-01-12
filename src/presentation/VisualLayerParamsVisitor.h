/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$
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
 
#ifndef GPLATES_PRESENTATION_VISUALLAYERPARAMSVISITOR_H
#define GPLATES_PRESENTATION_VISUALLAYERPARAMSVISITOR_H

#include "utils/SetConst.h"


namespace GPlatesPresentation
{
	/**
	 * This class is a base class for visitors that visit VisualLayerParams.
	 * For convenience, typedefs are provided below to cover the const and non-const cases.
	 */
	template<bool Const>
	class VisualLayerParamsVisitorBase
	{
	public:

		// Typedefs to give the supported derivations the appropriate const-ness.

		virtual
		~VisualLayerParamsVisitorBase()
		{  }

	protected:

		VisualLayerParamsVisitorBase()
		{  }
	};


	/**
	 * This is the base class for visitors that visit const VisualLayerParams.
	 */
	typedef VisualLayerParamsVisitorBase<true> ConstVisualLayerParamsVisitor;

	/**
	 * This is the base class for visitors that visit non-const VisualLayerParams.
	 */
	typedef VisualLayerParamsVisitorBase<false> VisualLayerParamsVisitor;

}

#endif // GPLATES_PRESENTATION_VISUALLAYERPARAMSVISITOR_H
