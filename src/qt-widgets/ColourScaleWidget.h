/* $Id$ */

/**
 * \file
 * Contains the definition of the class ElidedLabel.
 *
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2011 The University of Sydney, Australia
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
 
#ifndef GPLATES_QTWIDGETS_COLOURSCALEWIDGET_H
#define GPLATES_QTWIDGETS_COLOURSCALEWIDGET_H

#include <boost/weak_ptr.hpp>
#include <QWidget>


namespace GPlatesPresentation
{
	class VisualLayer;
}

namespace GPlatesQtWidgets
{
	class ColourScaleWidget :
			public QWidget
	{
	public:

		ColourScaleWidget(
				QWidget *parent_ = NULL);

		void
		populate(
				const boost::weak_ptr<GPlatesPresentation::VisualLayer> &visual_layer);

	private:

		boost::weak_ptr<GPlatesPresentation::VisualLayer> d_current_visual_layer;
	};
}

#endif  // GPLATES_QTWIDGETS_COLOURSCALEWIDGET_H