/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008 The University of Sydney, Australia
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

#ifndef GPLATES_GLOBAL_INTRUSIVEPOINTERZEROREFCOUNTEXCEPTION_H
#define GPLATES_GLOBAL_INTRUSIVEPOINTERZEROREFCOUNTEXCEPTION_H

// FIXME:  When the definition of 'write_message' moves to a .cc file, replace this with <iosfwd>.
#include <ostream>
#include "global/PreconditionViolationError.h"


namespace GPlatesGlobal
{
	/**
	 * This is the exception thrown when client code makes an attempt to retrieve an element
	 * from an empty container.
	 */
	class RetrievalFromEmptyContainerException:
			public GPlatesGlobal::PreconditionViolationError
	{
	public:
		/**
		 * When this exception is thrown, presumably in a member function of the object
		 * whose ref-count has been observed to be zero, the parameters to this constructor
		 * should be @c this, @c __FILE__ and @c __LINE__, which indicate the object, the
		 * file name and the line number, respectively.
		 */
		RetrievalFromEmptyContainerException(
				const char *filename_,
				int line_num_):
			d_filename(filename_),
			d_line_num(line_num_)
		{  }

		virtual
		~RetrievalFromEmptyContainerException()
		{  }

	protected:
		virtual
		const char *
		ExceptionName() const
		{
			// FIXME:  This function should really be defined in a .cc file.
			return "RetrievalFromEmptyContainerException";
		}

		virtual
		void
		write_message(
				std::ostream &os) const
		{
			// FIXME:  This function should really be defined in a .cc file.
		}

	private:
		const char *d_filename;
		int d_line_num;
	};
}

#endif  // GPLATES_GLOBAL_INTRUSIVEPOINTERZEROREFCOUNTEXCEPTION_H