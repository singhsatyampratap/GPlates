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

#ifndef GPLATES_MODEL_XMLATTRIBUTENAME_H
#define GPLATES_MODEL_XMLATTRIBUTENAME_H

#ifndef GPLATES_ICU_BOOL
#define GPLATES_ICU_BOOL(b) ((b) != 0)
#endif

#include "StringSetSingletons.h"
#include "StringContentTypeGenerator.h"

namespace GPlatesModel {

	class XmlAttributeNameFactory {

	public:
		static
		GPlatesUtils::StringSet &
		instance()
		{
			return StringSetSingletons::xml_attribute_name_instance();
		}

	private:
		XmlAttributeNameFactory();

	};

	typedef StringContentTypeGenerator<XmlAttributeNameFactory> XmlAttributeName;


	/**
	 * This operator is used when XmlAttributeName instances are used as keys in @c std::map.
	 */
	inline
	bool
	operator<(
			const XmlAttributeName &xan1,
			const XmlAttributeName &xan2) {
		return GPLATES_ICU_BOOL(xan1.get() < xan2.get());
	}

}

#endif  // GPLATES_MODEL_XMLATTRIBUTENAME_H
