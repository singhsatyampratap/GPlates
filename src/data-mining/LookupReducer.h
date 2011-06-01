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
#ifndef GPLATESDATAMINING_LOOKUPREDUCER_H
#define GPLATESDATAMINING_LOOKUPREDUCER_H

#include <vector>
#include <QDebug>
#include <boost/foreach.hpp>
#include <boost/tuple/tuple.hpp>

#include "CoRegReducer.h"
#include "DataMiningUtils.h"

#include <maths/MathsUtils.h>

namespace GPlatesDataMining
{
	class LookupReducer : public CoRegReducer
	{
		typedef std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*> RFGs;
	public:
		class Config : public CoRegReducer::Config
		{
		public:
			bool
			is_same_type(const CoRegReducer::Config* other)
			{
				return dynamic_cast<const LookupReducer::Config*>(other);
			}

			~Config(){ }
		};

		explicit
		LookupReducer(const RFGs& seeds) :
			d_seeds(seeds)
		{ }
		
		virtual
		~LookupReducer(){ }

	protected:
		OpaqueData
		exec(
				ReducerInDataset::const_iterator input_begin,
				ReducerInDataset::const_iterator input_end) ;
	protected:
		RFGs d_seeds;
	};
}
#endif





