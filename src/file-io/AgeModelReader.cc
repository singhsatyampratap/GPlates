/**
 * \file
 * $Revision: 15361 $
 * $Date: 2014-07-10 16:30:58 +0200 (Thu, 10 Jul 2014) $
 *
 * Copyright (C) 2014 Geological Survey of Norway
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
#include <QFile>

#include "AgeModelReader.h"
#include "ErrorOpeningFileForReadingException.h"

void
GPlatesFileIO::AgeModelReader::read_file(
		const QString &filename,
		GPlatesAppLogic::AgeModelCollection &model)
{
	QFile file(filename);

	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
	{
		throw ErrorOpeningFileForReadingException(GPLATES_EXCEPTION_SOURCE, filename);
	}

}
