/*$Id$*/

/**
 * @file
 *
 * Most recent change:
 * $Date$
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

#ifndef _GPLATES_FILEIO_FILEIOEXCEPTION_H_
#define _GPLATES_FILEIO_FILEIOEXCEPTION_H_

#include "global/Exception.h"
 
namespace GPlatesFileIO
{
	/**
	 * The (pure virtual) base class of all file I/O exceptions.
	 */
	class FileIOException : public GPlatesGlobal::Exception
	{
		public:
			virtual
			~FileIOException() {  }
	};
}

#endif  // _GPLATES_FILEIO_FILEIOEXCEPTION_H_
