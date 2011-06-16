/* $Id: DataSelector.cc 11569 2011-05-17 03:19:27Z mchin $ */

/**
 * \file .
 * $Revision: 11569 $
 * $Date: 2011-05-17 13:19:27 +1000 (Tue, 17 May 2011) $
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

#include "CoRegFilterCache.h"
#include "RegionOfInterestFilter.h"

using namespace GPlatesModel;
using namespace GPlatesAppLogic;

bool
GPlatesDataMining::CoRegFilterCache::find(
		const GPlatesDataMining::ConfigurationTableRow& key,
		std::vector<const GPlatesAppLogic::ReconstructedFeatureGeometry*>& values)
{
	boost::optional<std::size_t> weight = boost::none;
	bool hit = false;
	BOOST_FOREACH(const CacheItem& data, d_data)
	{
		if(key.target_fc.handle_ptr() == data.d_key.target_fc.handle_ptr() && 
			key.filter_cfg->filter_name() == data.d_key.filter_cfg->filter_name())
		{
			if(key.filter_cfg  < data.d_key.filter_cfg  || data.d_key.filter_cfg == key.filter_cfg)
			{
				if(!weight || *weight > data.d_value.size())
				{
					weight = data.d_value.size();
					values = data.d_value;
					hit = true;
				}
			}
		}
	}
	return hit;
}


