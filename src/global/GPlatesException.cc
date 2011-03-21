/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2009 The University of Sydney, Australia
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
#include <sstream>

#include "GPlatesException.h"


GPlatesGlobal::Exception::Exception(
		const GPlatesUtils::CallStack::Trace &exception_source)
{
	// Push the location of the thrown exception onto the call stack before
	// saving the call stack as a string.
	// The location will then get popped when we exit this constructor.
	GPlatesUtils::CallStackTracker call_stack_tracker(exception_source);

	generate_call_stack_trace_string();
}


void
GPlatesGlobal::Exception::write(
		std::ostream &os) const
{
	os << exception_name();

	os << ": ";

	// Get derived class to output a message.
	write_message(os);
}


void
GPlatesGlobal::Exception::write_string_message(
		std::ostream &os,
		const std::string &message) const
{
	os << message;
}


void
GPlatesGlobal::Exception::generate_call_stack_trace_string()
{
	std::ostringstream output_string_stream;

	GPlatesUtils::CallStack::instance().write_call_stack_trace(output_string_stream);

	d_call_stack_trace_string = output_string_stream.str();
}


const char *
GPlatesGlobal::Exception::what() const throw()
{
	// Write the message to a string stream.
	std::ostringstream output_string_stream;
	write(output_string_stream);

	// Store the message.
	d_std_exception_what_message = output_string_stream.str();

	// Return a 'const char *' to our stored string.
	return d_std_exception_what_message.c_str();
}


namespace GPlatesGlobal
{
	std::ostream &
	operator <<(
			std::ostream &os,
			const Exception &ex)
	{
		ex.write(os);
		return os;
	}


	QDebug
	operator <<(
			QDebug dbg,
			const Exception &ex)
	{
		std::ostringstream output_string_stream;
		output_string_stream << ex;

		dbg.nospace() << output_string_stream.str().c_str();

		return dbg.space();
	}


	QTextStream &
	operator <<(
			QTextStream &stream,
			const Exception &ex)
	{
		std::ostringstream output_string_stream;
		output_string_stream << ex;

		stream << output_string_stream.str().c_str();

		return stream;
	}
}
