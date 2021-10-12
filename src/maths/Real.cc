/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007, 2008 The University of Sydney, Australia
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

#include <sstream>
#ifdef HAVE_PYTHON
# include <boost/python.hpp>
#endif

#include "Real.h"
#include "HighPrecision.h"
#include "FunctionDomainException.h"

/*
 * FIXME: the value below was just a guess. Discover what this value should be.
 * 
 * According to:
 *  http://www.cs.berkeley.edu/~demmel/cs267/lecture21/lecture21.html
 * and
 *  http://www.ma.utexas.edu/documentation/lapack/node73.html
 * the machine epsilon for an IEEE 754-compliant machine is about 1.2e-16.
 *
 * According to these documents, the machine epsilon (aka "macheps") is
 * half the distance between 1 and the next largest fp value.
 *
 * Not only do I wish to allow for rounding errors due to the limits of
 * floating-point precision, I also wish to allow for a small accumulation
 * of such rounding errors.
 *
 * If macheps is 1.2e-16, then I -guessed- that it might be a good idea to
 * allow a flexibility of about two orders of magnitude, ie. 1.0e-14.
 *
 * The situations where such flexibility might be really important would
 * occur when deviations outside the epsilon could cause exceptions to be
 * thrown.  For example, say we are rotating a unitvector by multiplication
 * with a matrix.  That's 9 fpmuls and 6 fpadds to perform the rotation,
 * and then 3 fpmuls and 2 fpadds to check the magnitude of the resulting
 * unitvector.  That's a lot of error that can accumulate, and that's only
 * a _single_ rotation.  Of course, bear in mind that I have no background
 * in numerical analysis (yet...) so this is all just handwaving.  End rant.
 *
 * Update, 2004-02-05: well, it seems 1.0e-14 is too strict... so let's try
 * 1.0e-12, I guess...  I really need to do this stuff properly. --JB
 */
#define EXPONENT 12
#define RE_EPS(y) (1.0##e##-##y)  // Yes, we *really do* need all those hashes.
#define REAL_EPSILON(y) RE_EPS(y)  // Yes, we *really do* need this extra macro.


const unsigned
GPlatesMaths::Real::High_Precision = EXPONENT + 6;

const double
GPlatesMaths::Real::Epsilon = REAL_EPSILON(EXPONENT);

const double
GPlatesMaths::Real::Negative_Epsilon = -REAL_EPSILON(EXPONENT);


const GPlatesMaths::Real
GPlatesMaths::sqrt(
		const Real &r)
{
	if (isNegative(r)) {
		// The value of 'r' is not strictly valid as the argument to 'sqrt'.  Let's find
		// out if it's almost valid (in which case, we'll be lenient).
		if (r < 0.0) {
			// Even allowing some flexibility of comparison, the value of 'r' is
			// negative, which falls outside the domain of 'sqrt'.

			std::ostringstream oss("function 'sqrt' invoked with invalid argument ");
			oss << r;
			throw FunctionDomainException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
		} else {
			// It was almost valid.  Let's be lenient and pretend the value was exactly
			// zero.  We'll return the sqrt of zero, which is zero.
			// FIXME:  We should log that we "corrected" this here.
			return Real(0.0);
		}
	}

	// Else, the value of 'r' is valid as the argument to 'sqrt'.
	return Real(std::sqrt(r.dval()));
}


const GPlatesMaths::Real
GPlatesMaths::asin(
		const Real &r)
{
	if (isLessThanMinusOne(r)) {
		// The value of 'r' is not strictly valid as the argument to 'asin'.  Let's find
		// out if it's almost valid (in which case, we'll be lenient).
		if (r < -1.0) {
			// Even allowing some flexibility of comparison, the value of 'r' is less
			// than minus one, which falls outside the domain of 'asin'.

			std::ostringstream oss("function 'asin' invoked with invalid argument ");
			oss << r;
			throw FunctionDomainException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
		} else {
			// It was almost valid.  Let's be lenient and pretend the value was exactly
			// minus one.  We'll return the asin of minus one, which is minus pi on
			// two.
			// FIXME:  We should log that we "corrected" this here.
			std::cerr << "Corrected asin(" << HighPrecision<Real>(r) << ") to asin(-1)." << std::endl;
			return Real(-GPlatesMaths::PI_2);
		}
	}

	if (isGreaterThanOne(r)) {
		// The value of 'r' is not strictly valid as the argument to 'asin'.  Let's find
		// out if it's almost valid (in which case, we'll be lenient).
		if (r > 1.0) {
			// Even allowing some flexibility of comparison, the value of 'r' is
			// greater than one, which falls outside the domain of 'asin'.

			std::ostringstream oss("function 'asin' invoked with invalid argument ");
			oss << r;
			throw FunctionDomainException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
		} else {
			// It was almost valid.  Let's be lenient and pretend the value was exactly
			// one.  We'll return the asin of one, which is pi on two.
			// FIXME:  We should log that we "corrected" this here.
			std::cerr << "Corrected asin(" << HighPrecision<Real>(r) << ") to asin(1)." << std::endl;
			return Real(GPlatesMaths::PI_2);
		}
	}

	// Else, the value of 'r' is valid as the argument to 'asin'.
	return Real(std::asin(r.dval()));
}


const GPlatesMaths::Real
GPlatesMaths::acos(
		const Real &r)
{
	if (isLessThanMinusOne(r)) {
		// The value of 'r' is not strictly valid as the argument to 'acos'.  Let's find
		// out if it's almost valid (in which case, we'll be lenient).
		if (r < -1.0) {
			// Even allowing some flexibility of comparison, the value of 'r' is less
			// than minus one, which falls outside the domain of 'acos'.

			std::ostringstream oss("function 'acos' invoked with invalid argument ");
			oss << r;
			throw FunctionDomainException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
		} else {
			// It was almost valid.  Let's be lenient and pretend the value was exactly
			// minus one.  We'll return the acos of minus one, which is pi.
			// FIXME:  We should log that we "corrected" this here.
			std::cerr << "Corrected acos(" << HighPrecision<Real>(r) << ") to acos(-1)." << std::endl;
			return Real(GPlatesMaths::PI);
		}
	}

	if (isGreaterThanOne(r)) {
		// The value of 'r' is not strictly valid as the argument to 'acos'.  Let's find
		// out if it's almost valid (in which case, we'll be lenient).
		if (r > 1.0) {
			// Even allowing some flexibility of comparison, the value of 'r' is
			// greater than one, which falls outside the domain of 'acos'.

			std::ostringstream oss("function 'acos' invoked with invalid argument ");
			oss << r;
			throw FunctionDomainException(GPLATES_EXCEPTION_SOURCE,
					oss.str().c_str());
		} else {
			// It was almost valid.  Let's be lenient and pretend the value was exactly
			// one.  We'll return the acos of one, which is zero.
			// FIXME:  We should log that we "corrected" this here.
			std::cerr << "Corrected acos(" << HighPrecision<Real>(r) << ") to acos(1)." << std::endl;
			return Real(0.0);
		}
	}

	// Else, the value of 'r' is valid as the argument to 'acos'.
	return Real(std::acos(r.dval()));
}


const GPlatesMaths::Real
GPlatesMaths::atan2(
		const Real &y,
		const Real &x)
{
	// 0.0 is the only real floating-point value for which exact equality comparison is valid.
	if ((y == 0.0) && (x == 0.0)) {
		// Recall that we've defined atan2(0, 0) to be equal to zero.
		return Real(0.0);
	}
	return Real(std::atan2(y.dval(), x.dval()));
}


#ifdef HAVE_PYTHON
/**
 * Here begin the Python wrappers
 */


using namespace boost::python;


void
GPlatesMaths::export_Real()
{
	to_python_converter<GPlatesMaths::Real, GPlatesMaths::Real>();
	implicitly_convertible<double, GPlatesMaths::Real>();
}
#endif
