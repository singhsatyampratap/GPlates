/* $Id$ */

/**
 * @file 
 * Contains the definition of the CptColourPalette class.
 *
 * Most recent change:
 *   $Date$
 * 
 * Copyright (C) 2009, 2010 The University of Sydney, Australia
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

#ifndef GPLATES_GUI_CPTCOLOURPALETTE_H
#define GPLATES_GUI_CPTCOLOURPALETTE_H

#include <vector>
#include <boost/foreach.hpp>
#include <boost/optional.hpp>
#include <QString>

#include "Colour.h"
#include "ColourPalette.h"

#include "maths/Real.h"


namespace GPlatesGui
{
	/**
	 * When rendering a colour scale, it is possible to annotate the particular
	 * z-slice with either the formatted z-values or a user-defined label.
	 *
	 * These types of annotations are used in regular CPT files.
	 */
	namespace ColourScaleAnnotation
	{
		enum Type
		{
			NONE,	/**< Corresponds to the absence of the A flag in a CPT file */
			LOWER,	/**< Corresponds to the L option */
			UPPER,	/**< Corresponds to the U option */
			BOTH	/**< Correpsonds to the B option */
		};
	}


	/**
	 * A colour slice specifies a gradient of colour between two real values.
	 *
	 * These are used to store entries from regular CPT files.
	 */
	class ColourSlice
	{
	public:

		typedef GPlatesMaths::Real value_type;

		ColourSlice(
				value_type lower_value_,
				boost::optional<Colour> lower_colour_,
				value_type upper_value_,
				boost::optional<Colour> upper_colour_,
				ColourScaleAnnotation::Type annotation_,
				boost::optional<QString> label_);

		bool
		can_handle(
				value_type value) const;

		boost::optional<Colour>
		get_colour(
				value_type value) const;

		value_type
		lower_value() const
		{
			return d_lower_value;
		}

		void
		set_lower_value(
				value_type lower_value_)
		{
			d_lower_value = lower_value_;
		}

		value_type
		upper_value() const
		{
			return d_upper_value;
		}

		void
		set_upper_value(
				value_type upper_value_)
		{
			d_upper_value = upper_value_;
		}

		const boost::optional<Colour> &
		lower_colour() const
		{
			return d_lower_colour;
		}

		void
		set_lower_colour(
				const boost::optional<Colour> &lower_colour_)
		{
			d_lower_colour = lower_colour_;
		}

		const boost::optional<Colour> &
		upper_colour() const
		{
			return d_upper_colour;
		}

		void
		set_upper_colour(
				const boost::optional<Colour> &upper_colour_)
		{
			d_upper_colour = upper_colour_;
		}

		ColourScaleAnnotation::Type
		annotation() const
		{
			return d_annotation;
		}
		
		void
		set_annotation(
				ColourScaleAnnotation::Type annotation_)
		{
			d_annotation = annotation_;
		}

		const boost::optional<QString> &
		label() const
		{
			return d_label;
		}

		void
		set_label(
				const boost::optional<QString> &label_)
		{
			d_label = label_;
		}

	private:

		value_type d_lower_value, d_upper_value;
		boost::optional<Colour> d_lower_colour, d_upper_colour;
		ColourScaleAnnotation::Type d_annotation;
		boost::optional<QString> d_label;
	};


	bool
	operator<(
			ColourSlice::value_type value,
			const ColourSlice &colour_slice);


	bool
	operator>(
			ColourSlice::value_type value,
			const ColourSlice &colour_slice);


	/**
	 * ColourEntry stores a mapping from one value to one colour.
	 *
	 * These are used to store entries from categorical CPT files.
	 *
	 * In the unspecialised version of ColourEntry, the label is used as the value
	 * that is mapped to the colour.
	 */
	template<typename T>
	class ColourEntry
	{
	public:

		typedef T value_type;
		
		ColourEntry(
				int key_,
				Colour colour_,
				const T &label_) :
			d_key(key_),
			d_colour(colour_),
			d_label(label_)
		{
		}

		bool
		can_handle(
				const T &value) const
		{
			return d_label == value;
		}

		// This is not a useless duplicate of colour(). CptColourPalette expects a
		// get_colour() method to calculate the colour for a given value, while
		// colour() is the accessor to the instance variable.
		const Colour &
		get_colour(
				const T &value) const
		{
			return d_colour;
		}

		int
		key() const
		{
			return d_key;
		}

		void
		set_key(
				int key_)
		{
			d_key = key;
		}

		const Colour &
		colour() const
		{
			return d_colour;
		}

		void
		set_colour(
				const Colour &colour_)
		{
			d_colour = colour_;
		}

		const T &
		label() const
		{
		}

		void
		set_label(
				const T &label_)
		{
			d_label = label_;
		}

	private:

		int d_key;
		Colour d_colour;
		T d_label;
	};


	/**
	 * In the specialisation of ColourEntry for int, the integer key is used as the
	 * value that is mapped to the colour, and the label is used as a text label
	 * for rendering purposes.
	 */
	template<>
	class ColourEntry<int>
	{
	public:

		typedef int value_type;

		ColourEntry(
				int key_,
				Colour colour_,
				const QString &label_) :
			d_key(key_),
			d_colour(colour_),
			d_label(label_)
		{
		}

		bool
		can_handle(
				int value) const
		{
			return d_key == value;
		}

		// This is not a useless duplicate of colour(). CptColourPalette expects a
		// get_colour() method to calculate the colour for a given value, while
		// colour() is the accessor to the instance variable.
		const Colour &
		get_colour(
				int value) const
		{
			return d_colour;
		}

		int
		key() const
		{
			return d_key;
		}

		void
		set_key(
				int key_)
		{
			d_key = key_;
		}

		const Colour &
		colour() const
		{
			return d_colour;
		}

		void
		set_colour(
				const Colour &colour_)
		{
			d_colour = colour_;
		}

		const QString &
		label() const
		{
			return d_label;
		}

		void
		set_label(
				const QString &label_)
		{
			d_label = label_;
		}

	private:

		int d_key;
		Colour d_colour;
		QString d_label;
	};


	bool
	operator<(
			int value,
			const ColourEntry<int> &colour_entry);

	
	bool
	operator>(
			int value,
			const ColourEntry<int> &colour_entry);


	namespace CptColourPaletteInternals
	{
		// A helper traits class to resolve the right parameter for get_colour().
		template<typename T>
		struct Traits
		{
			typedef const T & value_type;
		};

		template<>
		struct Traits<int>
		{
			typedef int value_type;
		};
	}


	/**
	 * CptColourPalette stores the in-memory representation of a CPT file, whether
	 * regular or categorical. It is, essentially, a sequence of the in-memory
	 * representations of lines successfully parsed from a CPT file.
	 *
	 * For regular CPT files, the template parameter EntryType is ColourSlice,
	 * which stores the upper and lower values of a z-slice and their associated
	 * colour.
	 *
	 * For categorical CPT files, the template parameter EntryType is ColourEntry<T>,
	 * which stores one key and its associated colour and label.
	 *
	 * A description of a "regular" CPT file can be found at
	 * http://gmt.soest.hawaii.edu/gmt/doc/gmt/html/GMT_Docs/node69.html 
	 *
	 * A description of a "categorical" CPT file can be found at
	 * http://gmt.soest.hawaii.edu/gmt/doc/gmt/html/GMT_Docs/node68.html 
	 */
	template<class EntryType>
	class CptColourPalette :
			public ColourPalette<typename EntryType::value_type>
	{
	public:

		typedef typename CptColourPaletteInternals::Traits<typename EntryType::value_type>::value_type value_type;

		CptColourPalette() :
			d_rgb_colour_model(true)
		{
		}

		/**
		 * Adds an entry to the colour palette.
		 *
		 * Entries for regular CPT files and categorical CPT files where the value
		 * type is int should be added in increasing order otherwise the background
		 * and foreground colours are likely to be applied incorrectly.
		 */
		void
		add_entry(
				const EntryType &entry)
		{
			d_entries.push_back(entry);
		}

		/**
		 * Sets the background colour, used for values that go before the first entry.
		 *
		 * This colour is ignored for categorical CPT files where the value type is
		 * not int.
		 */
		void
		set_background_colour(
				const Colour &colour)
		{
			d_background_colour = colour;
		}

		/**
		 * Sets the foreground colour, used for values that go after the last entry.
		 *
		 * This colour is ignored for categorical CPT files where the value type is
		 * not int.
		 */
		void
		set_foreground_colour(
				const Colour &colour)
		{
			d_foreground_colour = colour;
		}

		/**
		 * Sets the NaN colour, used for values that are:
		 *  - NaN
		 *  - not present, and
		 *  - values not covered by entries in the CPT file or the background/
		 *    foreground colours.
		 */
		void
		set_nan_colour(
				const Colour &colour)
		{
			d_nan_colour = colour;
		}

		/**
		 * For regular CPT files, this sets whether colours with three components are
		 * interpreted as RGB or HSV, for both colour slices and FBN lines.
		 *
		 * For categorical CPT files, this setting is only used for FBN lines.
		 */
		void
		set_rgb_colour_model(
				bool rgb_colour_model)
		{
			d_rgb_colour_model = rgb_colour_model;
		}

		/**
		 * @see set_rgb_colour_model().
		 */
		bool
		is_rgb_colour_model() const
		{
			return d_rgb_colour_model;
		}

		/**
		 * Retrieves a Colour based on the @a value given.
		 */
		virtual
		boost::optional<Colour>
		get_colour(
				value_type value) const
		{
			if (d_entries.empty())
			{
				return d_nan_colour;
			}

			// See if we should use background colour.
			if (use_background_colour(value))
			{
				// Use NaN colour if background colour is not set.
				if (d_background_colour)
				{
					return d_background_colour;
				}
				else
				{
					return d_nan_colour;
				}
			}

			// See if we should use foreground colour.
			if (use_foreground_colour(value))
			{
				// Use NaN colour if foreground colour is not set.
				if (d_foreground_colour)
				{
					return d_foreground_colour;
				}
				else
				{
					return d_nan_colour;
				}
			}

			// Else try and find an entry that accepts the value, else return NaN colour.
			BOOST_FOREACH(EntryType entry, d_entries)
			{
				if (entry.can_handle(value))
				{
					return entry.get_colour(value);
				}
			}

			return d_nan_colour;
		}

	protected:

		virtual
		bool
		use_background_colour(
				value_type value) const = 0;

		virtual
		bool
		use_foreground_colour(
				value_type value) const = 0;

	private:

		std::vector<EntryType> d_entries;

		boost::optional<Colour> d_background_colour;
		boost::optional<Colour> d_foreground_colour;
		boost::optional<Colour> d_nan_colour;

		/**
		 * True if the colour model in this CPT file is RGB.
		 * If false, the colour model is HSV.
		 */
		bool d_rgb_colour_model;
	};


	template<class EntryType>
	bool
	CptColourPalette<EntryType>::use_background_colour(
			value_type value) const
	{
		// By default, background colour is used if value comes before first slice.
		return value < d_entries.front();
	}


	template<class EntryType>
	bool
	CptColourPalette<EntryType>::use_foreground_colour(
			value_type value) const
	{
		// By default, foreground colour is used if value comes after last slice.
		return value > d_entries.back();
	}


	/**
	 * A colour palette that stores entries from a regular CPT file.
	 */
	class RegularCptColourPalette :
			public CptColourPalette<ColourSlice>
	{
	protected:

		virtual
		bool
		use_background_colour(
				const ColourSlice::value_type &value) const
		{
			// Use default.
			return CptColourPalette<ColourSlice>::use_background_colour(value);
		}

		virtual
		bool
		use_foreground_colour(
				const ColourSlice::value_type &value) const
		{
			// Use default.
			return CptColourPalette<ColourSlice>::use_foreground_colour(value);
		}
	};


	/**
	 * A colour palette that stores entries from a categorical CPT file.
	 */
	template<typename T>
	class CategoricalCptColourPalette :
			public CptColourPalette<ColourEntry<T> >
	{
	protected:

		virtual
		bool
		use_background_colour(
				const T &value) const
		{
			// Do not use background colour. For categorical CPT files whose value type is
			// not int, we use the label as the value type, and there is no requirement
			// that the labels are presented in sorted order (in fact, there may be no order).
			return false;
		}

		virtual
		bool
		use_foreground_colour(
				const T &value) const
		{
			// Do not use foreground colour. For categorical CPT files whose value type is
			// not int, we use the label as the value type, and there is no requirement
			// that the labels are presented in sorted order (in fact, there may be no order).
			return false;
		}
	};


	/**
	 * Specialisation of CategoricalCptColourPalette for int.
	 *
	 * The specialisation enables the use of the background and foreground colours,
	 * which are not used in the general case for categorical CPT files.
	 */
	template<>
	class CategoricalCptColourPalette<int> :
			public CptColourPalette<ColourEntry<int> >
	{
	protected:

		virtual
		bool
		use_background_colour(
				int value) const
		{
			// Use default.
			return CptColourPalette<ColourEntry<int> >::use_background_colour(value);
		}

		virtual
		bool
		use_foreground_colour(
				int value) const
		{
			// Use default.
			return CptColourPalette<ColourEntry<int> >::use_foreground_colour(value);
		}
	};
}

#endif  /* GPLATES_GUI_CPTCOLOURPALETTE_H */