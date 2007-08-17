/* $Id$ */

/**
 * \file 
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2003, 2004, 2005, 2006, 2007 The University of Sydney, Australia
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

#include <iomanip>
#include <utility>
#include <algorithm>
#include <sstream>
#include <iterator>
#include "PlatesDataTypes.h"
#include "PlatesParserUtils.h"
#include "InvalidDataException.h"
#include "FileFormatException.h"

namespace 
{
	static 
	std::string
	ReadRestOfLine(
			const GPlatesFileIO::LineBuffer &lb, 
			std::istringstream &iss) 
	{
		std::string buf;
	
		// Handle arbitrary length lines.
		std::getline(iss, buf); 
	
		return buf;
	}
}


const
GPlatesFileIO::PlatesParser::FiniteRotation
GPlatesFileIO::PlatesParser::ParseRotationLine(
		const LineBuffer &lb,
		const std::string &line) 
{

	std::istringstream iss(line);

	/*
	 * This line is composed of:
	 *  - plate id of moving plate
	 *  - time of rotation (Millions of years ago)
	 *  - latitude of Euler pole
	 *  - longitude of Euler pole
	 *  - rotation angle (degrees)
	 *  - plate id of fixed plate
	 *  - comment (begins with '!', continues to end of line)
	 */
	plate_id_t moving_plate, fixed_plate;
	fpdata_t time, lat, lon, angle;
	std::string comment;

	moving_plate =
	 attemptToReadPlateID(lb, iss, "plate id of moving plate");
	time = attemptToReadFloat(lb, iss, "time of rotation");

	lat = attemptToReadFloat(lb, iss, "latitude of Euler pole");
	if ( ! LatLonPoint::isValidLat(lat)) {

		// not a valid latitude
		std::ostringstream oss;
		oss << "Invalid value (" << lat << ") for latitude found in\n"
		 << lb << ".";

		throw InvalidDataException(oss.str().c_str());
	}

	lon = attemptToReadFloat(lb, iss, "longitude of Euler pole");
	if ( ! LatLonPoint::isValidLon(lon)) {

		// not a valid longitude
		std::ostringstream oss;
		oss << "Invalid value (" << lon << ") for longitude found in\n"
		 << lb << ".";

		throw InvalidDataException(oss.str().c_str());
	}

	angle = attemptToReadFloat(lb, iss, "rotation angle");
	fixed_plate = attemptToReadPlateID(lb, iss, "plate id of fixed plate");

	/*
	 * The rest of the line (after whitespace) is assumed to be a comment.
	 * Eat leading whitespace, then dump the rest into a string.
	 */
	iss >> std::ws;
	comment = ReadRestOfLine(lb, iss);

	// Now, finally, create and return the PLATES data types.
	LatLonPoint euler_pole(lat, lon);
	EulerRotation rot(euler_pole, angle);

	return FiniteRotation(time, moving_plate, fixed_plate, rot, comment);
}


const 
GPlatesFileIO::PlatesParser::BoundaryLatLonPoint
GPlatesFileIO::PlatesParser::LatLonPoint::ParseBoundaryLine(
		const LineBuffer &lb, const std::string &line,
		int expected_plotter_code)
{
	/* 
	 * This line is composed of two fpdata_t (the lat/lon of the point)
	 * and an int (a plotter code).
	 */
	fpdata_t lat, lon;
	PlotterCodes::PlotterCode plotter_code;

	std::istringstream iss(line);

	/*
	 * XXX Bad, bad, bad!  These values should be checked as
	 * they're read in, just as Jimmy had done the first time.
	 * Alas, my crapoid hack requires that I get all the
	 * values up front, so as to 'early-out' when I find a 
	 * terminating point.
	 */	 
	lat = attemptToReadFloat(lb, iss, "latitude of a point");
	lon = attemptToReadFloat(lb, iss, "longitude of a point");
	plotter_code = attemptToReadPlotterCode(lb, iss);

	/*
	 * Test for terminating point.
	 * According to the PLATES data file spec, this has the 
	 * uninformative magical values:
	 *  lat = 99.0, lon = 99.0, plot code = 3 ('skip to')
	 */
	if ((lat == 99.0) && (lon == 99.0) 
		&& (plotter_code == PlotterCodes::PEN_UP)) {

		return BoundaryLatLonPoint(
			LatLonPoint(lat, lon), 
			PlotterCodes::PEN_TERMINATING_POINT,
			lb.lineNum()	
		);
	}

	if ( ! LatLonPoint::isValidLat(lat)) {

		// not a valid latitude
		std::ostringstream oss;
		oss << "Invalid value (" << lat << ") for latitude found in\n"
		 << lb << ".";

		throw InvalidDataException(oss.str().c_str());
	}

	if ( ! LatLonPoint::isValidLon(lon)) {

		// not a valid longitude
		std::ostringstream oss;
		oss << "Invalid value (" << lon << ") for longitude found in\n"
		 << lb << ".";

		throw InvalidDataException(oss.str().c_str());
	}

	if ((expected_plotter_code != PlotterCodes::PEN_EITHER)
		&& (plotter_code != expected_plotter_code)) {

		/*
		 * The plotter code which was read was not
		 * the code which was expected.
		 */
		std::ostringstream oss;
		oss << "Unexpected value (" << plotter_code
		 << ") for plotter code\nfound in " << lb << ".";

		throw InvalidDataException(oss.str().c_str());
	} else {

		// expected_plotter code must be PEN_EITHER
		
		if ((plotter_code != PlotterCodes::PEN_DOWN)
			&& (plotter_code != PlotterCodes::PEN_UP)) {

			/*
			 * The plotter code which was read was not
			 * one of the valid codes (UP=3, DOWN=2).
			 */
			std::ostringstream oss;
			oss << "Unexpected value (" << plotter_code
			 << ") for plotter code\nfound in " << lb << ".";

			throw InvalidDataException(oss.str().c_str());
		}
	}
	
	return BoundaryLatLonPoint(
		LatLonPoint(lat, lon), 
		plotter_code,
		lb.lineNum()	
	);
}


void
GPlatesFileIO::PlatesParser::LatLonPoint::ParseTermBoundaryLine(
		const LineBuffer &lb,
		const std::string &line, int expected_plotter_code) 
{
	/* 
	 * This line is composed of two doubles (the lat/lon of the point)
	 * and an int (a plotter code).
	 */
	fpdata_t lat, lon;
	int plotter_code;

	std::istringstream iss(line);

	lat = attemptToReadFloat(lb, iss, "latitude of a point");
	if (lat != 99.0) {

		/*
		 * The value read was not the expected value of 99.0,
		 * which marks a terminating point.
		 */
		std::ostringstream oss;
		oss << "Invalid value (" << lat << ") for latitude of"
		 << " terminating point\nfound in " << lb << ".";

		throw InvalidDataException(oss.str().c_str());
	}

	lon = attemptToReadFloat(lb, iss, "longitude of a point");
	if (lon != 99.0) {

		/*
		 * The value read was not the expected value of 99.0,
		 * which marks a terminating point.
		 */
		std::ostringstream oss;
		oss << "Invalid value (" << lon << ") for longitude of"
		 << " terminating point\nfound in " << lb << ".";

		throw InvalidDataException(oss.str().c_str());
	}

	plotter_code = attemptToReadPlotterCode(lb, iss);
	if (plotter_code != expected_plotter_code) {

		/*
		 * The plotter code which was read was not
		 * the code which was expected.
		 */
		std::ostringstream oss;
		oss << "Unexpected value (" << plotter_code
		 << ") for plotter code\nfound in " << lb << ".";

		throw InvalidDataException(oss.str().c_str());
	}

	// do not create a point
}


/**
 * Return whether a given value is a valid latitude.
 * For the PLATES format, the valid range of latitudes is [-90.0, 90.0].
 */
bool
GPlatesFileIO::PlatesParser::LatLonPoint::isValidLat(
		const fpdata_t &val) 
{
	return (-90.0 <= val && val <= 90.0);
}


/**
 * Return whether a given value is a valid longitude.
 * For the PLATES formats, we must accept longitudes in the range
 * [-360.0, 360.0].  Note that this is different to the rest of GPlates,
 * which uses the half-open range (-180.0, 180.0].
 */
bool
GPlatesFileIO::PlatesParser::LatLonPoint::isValidLon(
		const fpdata_t &val) 
{
	return (-360.0 <= val && val <= 360.0);
}


GPlatesFileIO::PlatesParser::PolylineHeader
GPlatesFileIO::PlatesParser::PolylineHeader::ParseLines(
		const LineBuffer &lb,
		const std::string &first_line,
		const std::string &second_line) {

	plate_id_t plate_id;
	fpdata_t age_appear, age_disappear;
	size_t num_points;

	ParseSecondLine(lb, second_line, plate_id, age_appear, age_disappear,
		num_points);

	// use GeoTimeInstant from model/ instead of TimeWindow from geo/

	return PolylineHeader(first_line, second_line, plate_id,
		GPlatesModel::GeoTimeInstant(age_appear.dval()), 
		GPlatesModel::GeoTimeInstant(age_disappear.dval()), num_points);
}


void
GPlatesFileIO::PlatesParser::PolylineHeader::ParseSecondLine(
		const LineBuffer &lb,
		const std::string &line,
		plate_id_t &plate_id,
		fpdata_t &age_appear,
		fpdata_t &age_disappear,
		size_t &num_points) 
{

	std::istringstream iss(line);

	// Get the 1st item on the line: the plate id
	plate_id = attemptToReadPlateID(lb, iss, "plate id");

	// Get the 2nd item on the line: the age of appearance
	age_appear = attemptToReadFloat(lb, iss, "age of appearance");

	// Get the 3rd item on the line: the age of disappearance
	age_disappear = attemptToReadFloat(lb, iss, "age of disappearance");

	// Ignore the 4th item on the line: the data type code for ridges
	std::string 
	 datatype = attemptToReadString(lb, iss, "data type code for ridges");

	if (datatype.size() > 2) {

		/*
		 * We have a case where the datatype and its code are not
		 * seperated by whitespace.  So we need to put back the 
		 * numerical part which was read into this string.
		 */
		std::string::size_type i = 2; // this should be where the int starts.
		for ( ; i < datatype.size(); ++i)
			iss.putback(datatype[i]);
	}
	// The stream should now be in a position to read the code number.

	// Ignore the 5th item on the line: the data type code number
	attemptToReadInt(lb, iss, "data type code number");

	// Ignore the 6th item on the line: the conjugate plate id
	attemptToReadPlateID(lb, iss, "conjugate plate id");

	// Ignore the 7th item on the line: the colour code number
	attemptToReadInt(lb, iss, "colour code number");

/*
 * This is not supported in all versions of PLATES, and it serves
 * no purpose anyhow, since there is a magical terminating point 
 * that allows us to find the end of a line string.
 */
#if 0
	// Get the 8th item on the line: the number of points in the polyline
	int number_of_points = attemptToReadInt(lb, iss, "number of points");

	/*
	 * Since a polyline must contain at least two points,
	 * the value of the last item on the line should be an integer >= 2.
	 */
	if (number_of_points < 2) {

		std::ostringstream oss;
		oss << "Invalid value (" << number_of_points
		 << ") for the number of points\nin the polyline header in "
		 << lb << ".";

		throw InvalidDataException(oss.str().c_str());
	}

	// Since it can never be < zero, let's return it as a size_t
	num_points = static_cast< size_t >(number_of_points);
#endif
}
