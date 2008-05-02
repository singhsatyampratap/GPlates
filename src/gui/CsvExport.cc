/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2008, Geological Survey of Norway
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
#include <iostream>
#include <fstream>

#include <QFileInfo>
#include <QMessageBox>
#include <QString>
#include <QTableWidget>

#include "file-io/ErrorOpeningFileForWritingException.h"
#include "CsvExport.h"

namespace {

} // anonymous namespace


namespace GPlatesGui {
	

	void
	GPlatesGui::CsvExport::export_table(
		QString &filename,
		QTableWidget *table)
	{
		QFileInfo file_info(filename);
		try{	

			std::ofstream os;
			os.exceptions(std::ios::badbit | std::ios::failbit);
			os.open(filename.toStdString().c_str());

			int num_columns = table->columnCount();
			int num_rows = table->rowCount();

			int column_count;
			int row_count;
			QTableWidgetItem *item;
			QString item_string;

			for(row_count = 0 ; row_count < num_rows ; row_count++)
			{
				for (column_count = 0 ; column_count < num_columns  ; column_count++)
				{
					item = table->item(row_count,column_count);
					item_string = item->text();
					os << item_string.toStdString().c_str();
				
					if (column_count < (num_columns - 1)){
						os << ",";
					}
	
				}

				os << std::endl;

			}
		}
		catch(...)
		{
			QString message = QObject::tr("An error occurred while writing to file '%1'")
					.arg(file_info.filePath());
			QMessageBox::critical(0, QObject::tr("Error Saving File"), message,
					QMessageBox::Ok, QMessageBox::Ok);					
		}

		return;

	}


} // namespace GPlatesGui