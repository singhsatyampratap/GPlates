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

namespace GPlatesDataMining
{
	using namespace GPlatesUtils;
	
	typedef std::vector<OpaqueData> ReducerInputSequence;

	/*	
	*	TODO:
	*	Comments....
	*/
	class LookupReducer
	{
	public:

		/*
		* TODO: comments....
		*/
		inline
		OpaqueData
		operator()(
				ReducerInputSequence::const_iterator input_begin,
				ReducerInputSequence::const_iterator input_end) 
		{
			if(input_begin != input_end)
			{
				return *input_begin;
			}
			else
			{
				return EmptyData;
			}
		}
	};
}
#endif




