/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Author$
 *   $Date$
 * 
 * Copyright (C) 2003 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * Authors:
 *   James Boyden <jboyden@es.usyd.edu.au>
 */

#ifndef _GPLATES_MATHS_INVALIDGREATCIRCLEARCEXCEPTION_H_
#define _GPLATES_MATHS_INVALIDGREATCIRCLEARCEXCEPTION_H_

#include "MathematicalException.h"

namespace GPlatesMaths
{
	/**
	 * The Exception thrown when an attempt is made to create an
	 * invalid great-circle arc.
	 */
	class InvalidGreatCircleArcException
		: public MathematicalException
	{
		public:
			/**
			 * @param msg is a description of the conditions
			 * which cause the invariant to be violated.
			 */
			InvalidGreatCircleArcException(const char *msg)
				: _msg(msg) {  }

			virtual
			~InvalidGreatCircleArcException() {  }

		protected:
			virtual const char *
			ExceptionName() const {

				return "InvalidGreatCircleArcException";
			}

			virtual std::string
			Message() const { return _msg; }

		private:
			std::string _msg;
	};
}

#endif  // _GPLATES_MATHS_INVALIDGREATCIRCLEARCEXCEPTION_H_
