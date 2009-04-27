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

#ifndef GPLATES_GLOBAL_EXCEPTION_H
#define GPLATES_GLOBAL_EXCEPTION_H

#include <iosfwd>
#include <string>

#include "utils/CallStackTracker.h"


// Note: we don't use BOOST_CURRENT_FUNCTION anymore since it can produce some pretty
// verbose output when a function has arguments that are template types.
// The filename and line number are all that are really needed.
#define GPLATES_EXCEPTION_SOURCE \
		GPlatesUtils::CallStack::Trace(__FILE__, __LINE__)


namespace GPlatesGlobal
{
	/**
	 * This is the base class of all exceptions in GPlates.
	 */
	class Exception
	{
		public:
			/**
			 * Constructor collects the current call stack trace as generated
			 * by @a CallStack.
			 * This is done in the constructor since that's where the exception
			 * is generated.
			 * Also adds the location at which exception the is thrown
			 * to the call stack trace.
			 * The location is given by the constructor arguments.
			 *
			 * You can conveniently call like:
			 *     throw Exception(GPLATES_EXCEPTION_SOURCE);
			 */
			Exception(
					const GPlatesUtils::CallStack::Trace &exception_source);

			virtual
			~Exception() {  }

			/**
			 * Write the name and message of an exception into the supplied output
			 * stream @a os.
			 *
			 * It is not intended that these messages be internationalised for users --
			 * they are purely for debugging output when an exception is caught at the
			 * base-most frame of the function call stack.
			 */
			void
			write(
					std::ostream &os) const;

			/**
			 * Returns a string containing the call stack trace to the location
			 * at which this exception was thrown.
			 */
			void
			get_call_stack_trace_string(
					std::string	&call_stack_trace_string) const
			{
				call_stack_trace_string = d_call_stack_trace_string;
			}

		protected:
			/**
			 * @return The name of this Exception.
			 */
			virtual
			const char *
			exception_name() const = 0;

			/**
			 * Derived classes can override this method and write their
			 * special message to the stream @a os.
			 * Default is to write nothing.
			 * This is ok for those exceptions where the exception class name
			 * provides enough description because @a write will output the
			 * exception class name.
			 */
			virtual
			void
			write_message(
					std::ostream &os) const
			{  }

			/**
			 * A convenience method so derived classes that contain only a string message
			 * can call without having to include <ostream> in the header and hence
			 * slow down compilation.
			 * This way it's defined in one place and in a ".cc" file rather than
			 * required each derived class to have a ".cc" file.
			 */
			void
			write_string_message(
					std::ostream &os,
					const std::string &message) const;

		private:
			/**
			 * Stores the call stack trace at the point this exception was thrown.
			 * This has to be stored since the location at which we catch the exception
			 * and retrieve this string is different to the location at which the exception
			 * is thrown. And the call stack trace differs between those two locations.
			 */
			std::string d_call_stack_trace_string;

			/**
			 * Generates the call stack trace string.
			 * Must be called in the constructor since that's where the exception
			 * is thrown and that's where we want to record the call stack trace.
			 */
			void
			generate_call_stack_trace_string();
	};


	/**
	 * Insert a string representation of the exception @a ex into the output stream @a os.
	 */
	inline
	std::ostream &
	operator<<(
			std::ostream &os,
			const Exception &ex)
	{
		ex.write(os);
		return os;
	}
}

#endif  // GPLATES_GLOBAL_EXCEPTION_H
