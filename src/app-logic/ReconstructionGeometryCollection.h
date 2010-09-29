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
 
#ifndef GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRYCOLLECTION_H
#define GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRYCOLLECTION_H

#include <vector>
#include <boost/iterator/transform_iterator.hpp>
#include <boost/lambda/construct.hpp>
#include <boost/function.hpp>

#include "ReconstructionGeometry.h"
#include "ReconstructionTree.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ReferenceCount.h"


namespace GPlatesAppLogic
{
	class ReconstructionGeometryCollection :
			public GPlatesUtils::ReferenceCount<ReconstructionGeometryCollection>
	{
	public:
		/**
		 * A convenience typedef for a shared pointer to non-const @a ReconstructionGeometryCollection.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<ReconstructionGeometryCollection>
				non_null_ptr_type;

		/**
		 * A convenience typedef for a shared pointer to const @a ReconstructionGeometryCollection.
		 */
		typedef GPlatesUtils::non_null_intrusive_ptr<const ReconstructionGeometryCollection>
				non_null_ptr_to_const_type;


		//! Typedef for a sequence of @a ReconstructionGeometry objects.
		typedef std::vector<ReconstructionGeometry::non_null_ptr_type>
				reconstruction_geometry_seq_type;

		/**
		 * The type used to iterate over the reconstruction geometries.
		 *
		 * Modification of the sequence is prevented but modification of the
		 * @a ReconstructionGeometry objects pointed to by the sequence is permitted.
		 */
		typedef reconstruction_geometry_seq_type::const_iterator iterator;

	private:

		/**
		 * The type of a function that converts a pointer to non-const
		 * ReconstructionGeometry to a pointer to const.
		 */
		typedef boost::function< ReconstructionGeometry::non_null_ptr_to_const_type (
					ReconstructionGeometry::non_null_ptr_type) > make_const_ptr_fn_type;

	public:

		/**
		 * The type used to const_iterate over the reconstruction geometries.
		 */
		typedef boost::transform_iterator<make_const_ptr_fn_type, iterator> const_iterator;


		~ReconstructionGeometryCollection();


		/**
		 * Create a new blank Reconstruction instance.
		 */
		static
		const non_null_ptr_type
		create(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree)
		{
			return non_null_ptr_type(
					new ReconstructionGeometryCollection(reconstruction_tree),
					GPlatesUtils::NullIntrusivePointerHandler());
		}


		/**
		 * Returns the "begin" const_iterator to iterate over the
		 * sequence of @a ReconstructionGeometry::non_null_ptr_to_const_type.
		 */
		const_iterator
		begin() const
		{
			return boost::make_transform_iterator(
					d_reconstruction_geometry_seq.begin(),
					make_const_ptr_fn_type(boost::lambda::constructor<ReconstructionGeometry::non_null_ptr_to_const_type>()));
		}


		/**
		 * Returns the "end" const_iterator to iterate over the
		 * sequence of @a ReconstructionGeometry::non_null_ptr_to_const_type.
		 */
		const_iterator
		end() const
		{
			return boost::make_transform_iterator(
					d_reconstruction_geometry_seq.end(),
					make_const_ptr_fn_type(boost::lambda::constructor<ReconstructionGeometry::non_null_ptr_to_const_type>()));

		}


		/**
		 * Returns the "begin" iterator to iterate over the
		 * sequence of @a ReconstructionGeometry::non_null_ptr_type.
		 */
		iterator
		begin()
		{
			return d_reconstruction_geometry_seq.begin();
		}


		/**
		 * Returns the "end" iterator to iterate over the
		 * sequence of @a ReconstructionGeometry::non_null_ptr_type.
		 */
		iterator
		end()
		{
			return d_reconstruction_geometry_seq.end();
		}


		/**
		 * Adds a @a ReconstructionGeometry to this collection and sets its collection pointer to us.
		 *
		 * Note that @a reconstruction_geometry is expected to have been reconstructed using
		 * the reconstruction tree passed into @a create.
		 *
		 * When 'this' object is destroyed it will set the collection pointers of its
		 * @a ReconstructionGeometry objects to NULL.
		 */
		void
		add_reconstruction_geometry(
				const ReconstructionGeometry::non_null_ptr_type &reconstruction_geometry);


		/**
		 * Return the reconstruction time used to reconstruct all geometries in this collection.
		 */
		const double &
		get_reconstruction_time() const
		{
			return d_reconstruction_tree->get_reconstruction_time();
		}


		/**
		 * Access the reconstruction tree.
		 */
		ReconstructionTree::non_null_ptr_to_const_type
		reconstruction_tree() const
		{
			return d_reconstruction_tree;
		}


		/**
		 * Access the Reconstruction instance which contains this @a ReconstructionGeoemtryCollection.
		 *
		 * Note that this pointer will be NULL if this ReconstructionGeoemtryCollection is not
		 * contained in a Reconstruction.
		 */
		Reconstruction *
		reconstruction() const
		{
			return d_reconstruction_ptr;
		}


		/**
		 * Set the reconstruction pointer.
		 *
		 * This function is intended to be invoked @em only when the ReconstructionGeometry is
		 * sitting in the vector inside the ReconstructionGeometryCollection instance, since even a
		 * copy-construction will reset the value of the reconstruction pointer back to NULL.
		 *
		 * WARNING:  This function should only be invoked by the code which is actually
		 * assigning an ReconstructionGeometry instance into (the vector inside) a
		 * ReconstructionGeometryCollection instance.
		 *
		 * NOTE: This method is const even though it modifies a data member.
		 * This is so this ReconstructionGeometryCollection can be added to a Reconstruction
		 * even if it's const.
		 *
		 * @throws PreconditionViolationError if this method has previously been called
		 * on this object.
		 */
		void
		set_reconstruction_ptr(
				Reconstruction *reconstruction_ptr) const;


		/**
		 * Returns the number of reconstruction geometries in this collection.
		 */
		std::size_t
		size() const
		{
			return d_reconstruction_geometry_seq.size();
		}


		/**
		 * Tests whether the reconstruction geometry collection is empty.
		 */
		bool
		empty() const
		{
			return d_reconstruction_geometry_seq.empty();
		}

	private:
		/**
		 * The plate-reconstruction hierarchy of total reconstruction poles which was used
		 * to reconstruct the geometries.
		 */
		ReconstructionTree::non_null_ptr_to_const_type d_reconstruction_tree;

		/**
		 * The reconstruction geometries.
		 */
		reconstruction_geometry_seq_type d_reconstruction_geometry_seq;

		/**
		 * This is the Reconstruction instance which contains this ReconstructionGeometryCollection.
		 *
		 * Note that we do NOT want this to be any sort of ref-counting pointer, since the
		 * Reconstruction instance which contains this ReconstructionGeometryCollection,
		 * does so using a ref-counting pointer; circularity of ref-counting pointers would
		 * lead to memory leaks.
		 *
		 * Note that this pointer may be NULL.
		 *
		 * This pointer should only @em ever point to a Reconstruction instance
		 * which @em does contain this ReconstructionGeometryCollection inside its vector.
		 * (This is the only way we can guarantee that the Reconstruction instance actually exists,
		 * ie that the pointer is not a dangling pointer.)
		 */
		mutable Reconstruction *d_reconstruction_ptr;


		/**
		 * This constructor should not be public, because we don't want to allow
		 * instantiation of this type on the stack.
		 */
		explicit
		ReconstructionGeometryCollection(
				const ReconstructionTree::non_null_ptr_to_const_type &reconstruction_tree_) :
			d_reconstruction_tree(reconstruction_tree_),
			d_reconstruction_ptr(NULL)
		{  }
	};
}

#endif // GPLATES_APP_LOGIC_RECONSTRUCTIONGEOMETRYCOLLECTION_H
