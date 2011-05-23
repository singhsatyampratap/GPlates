/* $Id$ */

/**
 * \file 
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

#ifndef GPLATES_APP_LOGIC_LAYERPROXYUTILS_H
#define GPLATES_APP_LOGIC_LAYERPROXYUTILS_H

#include <utility>
#include <vector>
#include <boost/optional.hpp>
#include <boost/type_traits/remove_pointer.hpp>

#include "LayerProxyVisitor.h"

#include "global/GPlatesAssert.h"
#include "global/PreconditionViolationError.h"

#include "utils/CopyConst.h"
#include "utils/non_null_intrusive_ptr.h"
#include "utils/SafeBool.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesAppLogic
{
	namespace LayerProxyUtils
	{
		///////////////
		// Interface //
		///////////////


		/**
		 * Determines if the @a LayerProxy object pointed to by
		 * @a layer_proxy_ptr is of type LayerProxyDerivedType.
		 *
		 * Example usage:
		 *   LayerProxy::non_null_ptr_type layer_proxy_ptr = ...;
		 *   boost::optional<const ReconstructLayerProxy *> proxy =
		 *        get_layer_proxy_derived_type<const ReconstructLayerProxy>(
		 *              layer_proxy_ptr);
		 *
		 * If type matches then returns pointer to derived type otherwise returns false.
		 */
		template <class LayerProxyDerivedType, typename LayerProxyPointer>
		boost::optional<LayerProxyDerivedType *>
		get_layer_proxy_derived_type(
				LayerProxyPointer layer_proxy_ptr);


		/**
		 * Searches a sequence of @a LayerProxy objects for
		 * a certain type derived from @a LayerProxy and appends any found
		 * to @a layer_proxy_derived_type_seq.
		 *
		 * Template parameter 'LayerProxyForwardIter' contains pointers to
		 * @a LayerProxy objects (or anything that behaves like a pointer).
		 *
		 * NOTE: Template parameter 'ContainerOfLayerProxyDerivedType' *must*
		 * be a standard container supporting the 'push_back()' method and must contain
		 * pointers to a const or non-const type derived from @a LayerProxy.
		 * It is this exact type that the caller is effectively searching for.
		 *
		 * Returns true if any found from input sequence.
		 */
		template <typename LayerProxyForwardIter,
				class ContainerOfLayerProxyDerivedType>
		bool
		get_layer_proxy_derived_type_sequence(
				LayerProxyForwardIter layer_proxies_begin,
				LayerProxyForwardIter layer_proxies_end,
				ContainerOfLayerProxyDerivedType &layer_proxy_derived_type_seq);


		/**
		 * A useful class for derived @a LayerProxy classes to use to keep track of changes
		 * to their input layer proxies.
		 *
		 * References, and observes for changes, an input layer proxy.
		 */
		template <class LayerProxyType>
		class InputLayerProxy
		{
		public:
			explicit
			InputLayerProxy(
					const typename LayerProxyType::non_null_ptr_type &input_layer_proxy) :
				d_input_layer_proxy(input_layer_proxy)
			{  }


			/**
			 * Returns the input layer proxy wrapped by this object.
			 */
			const typename LayerProxyType::non_null_ptr_type &
			get_input_layer_proxy() const
			{
				return d_input_layer_proxy;
			}


			/**
			 * Sets a new input layer proxy wrapped by this object.
			 *
			 * This also makes the caller out-of-date with respect to the new input layer proxy.
			 */
			void
			set_input_layer_proxy(
					const typename LayerProxyType::non_null_ptr_type &input_layer_proxy)
			{
				d_input_layer_proxy = input_layer_proxy;
				d_input_layer_proxy_observer_token.reset();
			}


			/**
			 * Returns true if the caller is up-to-date with respect to this input layer proxy.
			 */
			bool
			is_up_to_date() const
			{
				return d_input_layer_proxy->get_subject_token().is_observer_up_to_date(
						d_input_layer_proxy_observer_token);
			}


			/**
			 * Makes the caller up-to-date with respect to this input layer proxy.
			 */
			void
			set_up_to_date()
			{
				d_input_layer_proxy->get_subject_token().update_observer(
						d_input_layer_proxy_observer_token);
			}

		private:
			typename LayerProxyType::non_null_ptr_type d_input_layer_proxy;
			GPlatesUtils::ObserverToken d_input_layer_proxy_observer_token;
		};


		/**
		 * A wrapper around @a InputLayerProxy to make it optional.
		 */
		template <class LayerProxyType>
		class OptionalInputLayerProxy :
				public GPlatesUtils::SafeBool<OptionalInputLayerProxy<LayerProxyType> >
		{
		public:
			//! Default constructor stores no input layer proxy.
			explicit
			OptionalInputLayerProxy() :
				d_is_none_and_up_to_date(true)
			{  }

			//! Constructor to store an input layer proxy.
			explicit
			OptionalInputLayerProxy(
					const typename LayerProxyType::non_null_ptr_type &input_layer_proxy) :
				d_optional_input_layer_proxy(input_layer_proxy),
				d_is_none_and_up_to_date(false)
			{  }


			/**
			 * Use 'if (proxy)' to test if the wrapped input layer proxy is set (not boost::none).
			 */
			bool
			boolean_test() const
			{
				return d_optional_input_layer_proxy;
			}


			/**
			 * Returns the input layer proxy wrapped by this object.
			 *
			 * NOTE: Be sure to use 'if (proxy)' to test that an input layer proxy is wrapped
			 * before calling this method.
			 */
			const typename LayerProxyType::non_null_ptr_type &
			get_input_layer_proxy() const
			{
				GPlatesGlobal::Assert<GPlatesGlobal::PreconditionViolationError>(
						d_optional_input_layer_proxy,
						GPLATES_ASSERTION_SOURCE);
				return d_optional_input_layer_proxy.get().get_input_layer_proxy();
			}


			/**
			 * Returns the input layer proxy wrapped by this object as a boost::optional.
			 */
			boost::optional<typename LayerProxyType::non_null_ptr_type>
			get_optional_input_layer_proxy() const
			{
				if (!d_optional_input_layer_proxy)
				{
					return boost::none;
				}

				return d_optional_input_layer_proxy.get().get_input_layer_proxy();
			}


			/**
			 * Sets a new input layer proxy wrapped by this object.
			 *
			 * This also makes the caller out-of-date with respect to the new input layer proxy
			 * (if setting a new input layer proxy as opposed to setting it to boost::none).
			 */
			void
			set_input_layer_proxy(
					const boost::optional<typename LayerProxyType::non_null_ptr_type> &optional_input_layer_proxy = boost::none)
			{
				if (d_optional_input_layer_proxy)
				{
					if (!optional_input_layer_proxy)
					{
						d_optional_input_layer_proxy = boost::none;
						// We just reset an existing input layer proxy so the client needs
						// to know it's out-of-date.
						d_is_none_and_up_to_date = false;
						return;
					}

					d_optional_input_layer_proxy.get().set_input_layer_proxy(optional_input_layer_proxy.get());
				}
				else if (optional_input_layer_proxy)
				{
					d_optional_input_layer_proxy = InputLayerProxy<LayerProxyType>(optional_input_layer_proxy.get());
				}
			}


			/**
			 * Returns true if the caller is up-to-date with respect to this input layer proxy.
			 */
			bool
			is_up_to_date() const
			{
				if (!d_optional_input_layer_proxy)
				{
					return d_is_none_and_up_to_date;
				}

				return d_optional_input_layer_proxy.get().is_up_to_date();
			}


			/**
			 * Makes the caller up-to-date with respect to this input layer proxy.
			 */
			void
			set_up_to_date()
			{
				if (!d_optional_input_layer_proxy)
				{
					d_is_none_and_up_to_date = true;
					return;
				}

				d_optional_input_layer_proxy.get().set_up_to_date();
			}

		private:
			boost::optional<InputLayerProxy<LayerProxyType> > d_optional_input_layer_proxy;

			/**
			 * This flag is only used if @a d_optional_input_layer_proxy is boost::none
			 * in which case if it's false it means the input layer proxy has recently been
			 * set to none and is out-of-date because the client has not yet called @a set_up_to_date.
			 */
			bool d_is_none_and_up_to_date;
		};


		/**
		 * A convenience class that wraps a std::vector of @a InputLayerProxy and
		 * adds/removes input layer proxies.
		 */
		template <class LayerProxyType>
		class InputLayerProxySequence
		{
		public:
			//! Typedef for a sequence of @a InputLayerProxy objects.
			typedef std::vector<InputLayerProxy<LayerProxyType> > seq_type;


			//! Get the 'const' sequence of input layer proxies.
			const seq_type &
			get_input_layer_proxies() const
			{
				return d_seq;
			}

			//! Get the 'non-const' sequence of input layer proxies.
			seq_type &
			get_input_layer_proxies()
			{
				return d_seq;
			}


			/**
			 * Adds the specified input layer proxy to the sequence.
			 */
			void
			add_input_layer_proxy(
					const typename LayerProxyType::non_null_ptr_type &input_layer_proxy)
			{
				d_seq.push_back(InputLayerProxy<LayerProxyType>(input_layer_proxy));
			}

			/**
			 * Removes the specified input layer proxy from the sequence.
			 */
			void
			remove_input_layer_proxy(
					const typename LayerProxyType::non_null_ptr_type &input_layer_proxy)
			{
				// Search and erase the input layer proxy from our sequence.
				typename seq_type::iterator input_layer_proxy_iter = d_seq.begin();
				const typename seq_type::iterator input_layer_proxy_end = d_seq.end();
				for ( ; input_layer_proxy_iter != input_layer_proxy_end; ++input_layer_proxy_iter)
				{
					if (input_layer_proxy_iter->get_input_layer_proxy() == input_layer_proxy)
					{
						d_seq.erase(input_layer_proxy_iter);
						return;
					}
				}
			}
			
		private:
			seq_type d_seq;
		};


		////////////////////
		// Implementation //
		////////////////////


		/**
		 * Template visitor class to find instances of a class derived from @a LayerProxy.
		 */
		template <class LayerProxyDerivedType>
		class LayerProxyDerivedTypeFinder :
				public LayerProxyVisitorBase<
						typename GPlatesUtils::CopyConst<
								LayerProxyDerivedType, LayerProxy>::type >
		{
		public:
			//! Typedef for base class type.
			typedef LayerProxyVisitorBase<
					typename GPlatesUtils::CopyConst<
							LayerProxyDerivedType, LayerProxy>::type > base_class_type;

			/**
			 * Convenience typedef for the template parameter which is a type derived from @a LayerProxy.
			 */
			typedef LayerProxyDerivedType layer_proxy_derived_type;

			//! Convenience typedef for sequence of pointers to layer proxy derived type.
			typedef std::vector<layer_proxy_derived_type *> container_type;

			// Bring base class visit methods into scope of current class.
			using base_class_type::visit;

			//! Returns a sequence of layer proxies of type LayerProxyDerivedType
			const container_type &
			get_layer_proxy_type_sequence() const
			{
				return d_found_layer_proxies;
			}

			virtual
			void
			visit(
					const GPlatesUtils::non_null_intrusive_ptr<layer_proxy_derived_type> &layer_proxy)
			{
				d_found_layer_proxies.push_back(layer_proxy.get());
			}

		private:
			container_type d_found_layer_proxies;
		};


		template <class LayerProxyDerivedType, typename LayerProxyPointer>
		boost::optional<LayerProxyDerivedType *>
		get_layer_proxy_derived_type(
				LayerProxyPointer layer_proxy_ptr)
		{
			// Type of finder class.
			typedef LayerProxyDerivedTypeFinder<LayerProxyDerivedType>
					layer_proxy_derived_type_finder_type;

			layer_proxy_derived_type_finder_type layer_proxy_derived_type_finder;

			// Visit LayerProxy.
			layer_proxy_ptr->accept_visitor(layer_proxy_derived_type_finder);

			// Get the sequence of any found LayerProxy derived types.
			// Can only be one as most though.
			const typename layer_proxy_derived_type_finder_type::container_type &
					derived_type_seq = layer_proxy_derived_type_finder.get_layer_proxy_type_sequence();

			if (derived_type_seq.empty())
			{
				// Return false since found no derived type.
				return boost::none;
			}

			return derived_type_seq.front();
		}


		template <typename LayerProxyForwardIter,
				class ContainerOfLayerProxyDerivedType>
		bool
		get_layer_proxy_derived_type_sequence(
				LayerProxyForwardIter layer_proxies_begin,
				LayerProxyForwardIter layer_proxies_end,
				ContainerOfLayerProxyDerivedType &layer_proxy_derived_type_seq)
		{
			// We're expecting a container of pointers so get the type being pointed at.
			typedef typename boost::remove_pointer<
					typename ContainerOfLayerProxyDerivedType::value_type>::type
							recon_geom_derived_type;

			// Type of finder class.
			typedef LayerProxyDerivedTypeFinder<recon_geom_derived_type>
					layer_proxy_derived_type_finder_type;

			layer_proxy_derived_type_finder_type layer_proxy_derived_type_finder;

			// Visit each LayerProxy in the input sequence.
			for (LayerProxyForwardIter layer_proxy_iter = layer_proxies_begin;
				layer_proxy_iter != layer_proxies_end;
				++layer_proxy_iter)
			{
				(*layer_proxy_iter)->accept_visitor(layer_proxy_derived_type_finder);
			}

			// Get the sequence of any found LayerProxy derived types.
			const typename layer_proxy_derived_type_finder_type::container_type &
					derived_type_seq = layer_proxy_derived_type_finder.get_layer_proxy_type_sequence();

			// Append to the end of the output sequence of derived types.
			std::copy(
					derived_type_seq.begin(),
					derived_type_seq.end(),
					std::back_inserter(layer_proxy_derived_type_seq));

			// Return true if found any derived types.
			return !derived_type_seq.empty();
		}
	}
}

#endif // GPLATES_APP_LOGIC_LAYERPROXYUTILS_H