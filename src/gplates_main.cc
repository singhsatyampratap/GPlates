/* $Id$ */

/**
 * \file 
 * $Revision$
 * $Date$ 
 * 
 * Copyright (C) 2006 The University of Sydney, Australia
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

#include "gui/View.h"
#include "gui-qt/QtInterface.h"
#include "application/Presenter.h"

int
main(
        int argc,
        char *argv[])
{
    // create the two ends of the application and glue them together
    GPlatesView::View<GPlatesView::QtInterface> &view = GPlatesView::View<GPlatesView::QtInterface>::get_view();
    GPlatesPresenter::Presenter &presenter = GPlatesPresenter::Presenter::get_presenter();
    // FIXME: we'll need to be careful on shutdown as presenter will disappear before view
    view.attach_presenter(presenter);
    // hand over control to the interface
    return view.main();
}
