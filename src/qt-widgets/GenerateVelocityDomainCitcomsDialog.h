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
#ifndef GENERATE_VELOCITY_DOMAIN_CITCOMS_DIALOG_H
#define GENERATE_VELOCITY_DOMAIN_CITCOMS_DIALOG_H

#include <iostream>
#include <QObject>
#include <QDialog>

#include "GenerateVelocityDomainCitcomsDialogUi.h"

#include "GPlatesDialog.h"
#include "InformationDialog.h"
#include "OpenDirectoryDialog.h"

#include "presentation/ViewState.h"


namespace GPlatesQtWidgets
{
	class GenerateVelocityDomainCitcomsDialog: 
		public GPlatesDialog,
		protected Ui_GenerateVelocityDomainCitcomsDialog 
	{
		Q_OBJECT

	public:
		GenerateVelocityDomainCitcomsDialog(
				GPlatesPresentation::ViewState &,
				QWidget *parent_ = NULL);
		
		virtual
		~GenerateVelocityDomainCitcomsDialog()
		{
		//	qDebug() << "destructing mesh dialog";
		}
		
	private Q_SLOTS:

		void 
		gen_mesh();

		void
		set_node_x(int);
		
		void
		set_path();
		
		void
		select_path();
		
		void
		set_file_name_template();
		
	private:
		int d_node_x;
		QString d_path;
		GPlatesPresentation::ViewState &d_view_state;
		InformationDialog *d_help_dialog_resolution;
		InformationDialog *d_help_dialog_output;
		std::string d_file_name_template;
		OpenDirectoryDialog d_open_directory_dialog;
	};
}

#endif

