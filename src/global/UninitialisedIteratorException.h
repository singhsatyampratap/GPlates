/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
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
 */

#ifndef _GPLATES_GLOBAL_UNINITIALISEDITERATOREXCEPTION_H_
#define _GPLATES_GLOBAL_UNINITIALISEDITERATOREXCEPTION_H_

#include "Exception.h"

namespace GPlatesGlobal
{
	/**
	 * Should be thrown when an attempt is made to dereference an
	 * uninitialised iterator or access the members of a pointed-to-object
	 * through an uninitialised iterator.
	 */
	class UninitialisedIteratorException : public Exception
	{
		public:
			/**
			 * @param msg is a message describing the situation.
			 */
			explicit
			UninitialisedIteratorException(const char *msg)
				: _msg(msg) {  }

			virtual
			~UninitialisedIteratorException() {  }

		protected:
			virtual const char *
			ExceptionName() const {

				return "UninitialisedIteratorException";
			}

			virtual std::string
			Message() const { return _msg; }

		private:
			std::string _msg;
	};
}

#endif  // _GPLATES_GLOBAL_UNINITIALISEDITERATOREXCEPTION_H_