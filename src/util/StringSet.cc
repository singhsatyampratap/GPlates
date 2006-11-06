/* $Id$ */

/**
 * \file
 * File specific comments.
 *
 * Most recent change:
 *   $Date$
 *
 * Copyright (C) 2006 The GPlates Consortium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "StringSet.h"


bool
GPlatesUtil::StringSet::SharedIterator::operator==(
		const SharedIterator &other) const
{
	if (d_impl_ptr != other.d_impl_ptr)
	{
		// Since the two instances point at different impls, they cannot be equal.
		return false;
	}
	// else, the two instances point at the same impl (or both have NULL impl-pointers).
	
	if (d_impl_ptr.get() == NULL)
	{
		// Both impl-pointers are NULL.  This means that both instances are uninitialised.
		//
		// We'll implement this function so that all uninitialised instances compare equal
		// so that it's possible to determine whether a given instance may be dereferenced.
		return true;
	}

	return (d_iter == other.d_iter);
}

void
GPlatesUtil::StringSet::SharedIterator::increment_ref_count()
{
	if (d_impl_ptr.get() == NULL)
	{
		// This instance is uninitialised.
		return;
	}
	++(d_iter->d_ref_count);
}


void
GPlatesUtil::StringSet::SharedIterator::decrement_ref_count()
{
	if (d_impl_ptr.get() == NULL)
	{
		// This instance is uninitialised.
		return;
	}
	if (--(d_iter->d_ref_count) == 0)
	{
		// There are no more references to the element in the set.
		d_impl_ptr->collection().erase(d_iter);
	}
}


GPlatesUtil::StringSet::SharedIterator
GPlatesUtil::StringSet::insert(
		const UnicodeString &s)
{
	UnicodeStringAndRefCount elem(s);
	collection_type::iterator iter = d_impl->collection().find(elem);
	if (iter != d_impl->collection().end())
	{
		// The element already exists in the set.
		SharedIterator sh_iter(iter, d_impl);
		return sh_iter;
	}
	else
	{
		std::pair< collection_type::iterator, bool > insertion = d_impl->collection().insert(elem);
		// Now the element exists in the set.
		SharedIterator sh_iter(insertion.first, d_impl);
		return sh_iter;
	}
}