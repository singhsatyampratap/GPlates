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
#ifndef GPLATESDATAMINING_MEDIANREDUCER_H
#define GPLATESDATAMINING_MEDIANREDUCER_H

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
	class MedianReducer
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
			std::vector<double> array;
			DataMiningUtils::convert_to_double_vector(
					input_begin, 
					input_end, 
					array);
			/* 
			* For even length vector, this function will return "upper" median. 
			* If we want the traditional median of even length vector, 
			* we need to run nth_element twice.
			*/
			std::vector<double>::iterator first = array.begin();
			std::vector<double>::iterator last = array.end();
			std::vector<double>::iterator middle = first + (last - first) / 2;
			std::nth_element(first, middle, last); 
			return OpaqueData(*middle);
		}
	};
}
#endif





