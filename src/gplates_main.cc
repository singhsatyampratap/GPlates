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
 *   Hamish Law <hlaw@geosci.usyd.edu.au>
 */

/** 
 * @mainpage GPlates - Interactive Plate Tectonic Reconstructions
 * 
 * @section intro Introduction
 * Welcome to the GPlates Developers' Manual.
 *
 * @section patterns Design Patterns
 * All the references to design patterns in the GPlates documentation 
 * refer to the book <i>Design Patterns</i> by Gamma, Helm, Johnson 
 * and Vlissides.
 * See the References section below for more details.
 *
 * @section contact Contact
 * <b>Email</b>:
 * - Dr. R. Dietmar M&uuml;ller <dietmar (at) geosci.usyd.edu.au>
 * - Stuart Clark <srclark (at) geosci.usyd.edu.au>
 * - James Boyden <jboyden (at) geosci.usyd.edu.au>
 * - Hamish Law <hlaw (at) geosci.usyd.edu.au>
 * - David Symonds <ds (at) geosci.usyd.edu.au>
 * 
 * <b>Snail Mail</b>:
 * - University of Sydney Institute of Marine Science<br>
 *   Edgeworth David Building F05<br>
 *   School of Geosciences<br>
 *   The University of Sydney, NSW 2006<br>
 *   AUSTRALIA<br>
 *
 * @section refs References
 * The following books and articles are either cited in the documentation 
 * or played a significant role in the project:
 * - Foley, J., van Dam, A., Feiner, S., and Hughes, J. (1996)
 *   <i>Computer Graphics: Principles and Practice (2nd Ed.)</i>,
 *   Addison-Wesley.
 * - Gahagan, L. (1999) <i>plates4.0: A User's Manual for the Plates
 *   Project's interactive reconstruction software</i>, The
 *   University of Texas Institute for Geophysics.
 * - Gamma, E., Helm, R., Johnson, R., and Vlissides, J. (1995)
 *   <i>Design Patterns: Elements of Reusable Object-Oriented Software</i>,
 *   Addison-Wesley.
 * - Greiner, B., "Euler Rotations in Plate-Tectonic Reconstructions"
 *   in <i>Computers and Geosciences</i> (1999) No. 25, pp209-216.
 * - Josuttis, N. (1999)
 *   <i>The C++ Standard Library: A Tutorial and Reference</i>,
 *   Addison-Wesley.
 * - Stoustrup, B. (2000)
 *   <i>The C++ Programming Language (3rd Ed.)</i>,
 *   Addison-Wesley.
 */

#include <iostream>
#include <wx/app.h>

#include "global/config.h"
#include "global/Exception.h"

#include "gui/GLFrame.h"
#include "geo/DataGroup.h"
#include "fileio/GPlatesReader.h"
#include "controls/View.h"

namespace 
{
	class GPlatesApp : public wxApp
	{
		public:
			/**
			 * wxWindows equivalent of the traditional main() function.
			 * Commandline arguments are available to all GPlatesApp
			 * functions.
			 */
			bool OnInit();
	};
}

IMPLEMENT_APP(GPlatesApp)

using namespace GPlatesGui;

bool
GPlatesApp::OnInit()
{
	/*
	 * Note that this 'try ... catch' block can only catch exceptions
	 * thrown during the instantiation of the new GLFrame.  It CAN'T
	 * catch exceptions thrown at any later stage.
	 */
	try {
		// NULL => no parent
		/*
		 * Note that '_(str)' is a gettext-style macro alias for
		 * 'wxGetTranslation(str)'.
		 */
		GLFrame* frame = new GLFrame(NULL, _(PACKAGE_STRING), wxSize(640,640));
		
		frame->Show(TRUE);

	} catch (const GPlatesGlobal::Exception& e) {
		std::cerr << "Caught GPlates exception: " << e << std::endl;
		return FALSE;
	} catch (const std::exception& e) {
		std::cerr << "Caught non-GPlates exception: " << e.what() 
			<< std::endl;
		return FALSE;
	} catch (...) {
		std::cerr << "Caught unrecognised exception: " << std::endl;
		return FALSE;
	}
	return TRUE;
}
