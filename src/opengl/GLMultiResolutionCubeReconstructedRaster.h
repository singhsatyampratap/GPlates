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

#ifndef GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERECONSTRUCTEDRASTER_H
#define GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERECONSTRUCTEDRASTER_H

#include <cstddef> // For std::size_t
#include <vector>
#include <boost/optional.hpp>
#include <boost/shared_ptr.hpp>

#include "GLCubeSubdivision.h"
#include "GLMultiResolutionStaticPolygonReconstructedRaster.h"
#include "GLMultiResolutionRaster.h"
#include "GLTexture.h"
#include "GLTextureUtils.h"
#include "OpenGLFwd.h"

#include "maths/MathsFwd.h"
#include "maths/CubeQuadTree.h"
#include "maths/CubeQuadTreeLocation.h"

#include "utils/non_null_intrusive_ptr.h"
#include "utils/ObjectCache.h"
#include "utils/ReferenceCount.h"
#include "utils/SubjectObserverToken.h"


namespace GPlatesOpenGL
{
	class GLRenderer;
	class GLViewport;

	/**
	 * A reconstructed raster rendered into a multi-resolution cube map.
	 */
	class GLMultiResolutionCubeReconstructedRaster :
			public GLMultiResolutionCubeRasterInterface
	{
	private:
		/**
		 * Maintains a tile's texture and source tile cache handle.
		 */
		struct TileTexture
		{
			explicit
			TileTexture(
					GLRenderer &renderer_,
					const GLMultiResolutionStaticPolygonReconstructedRaster::cache_handle_type &source_cache_handle_ =
							GLMultiResolutionStaticPolygonReconstructedRaster::cache_handle_type()) :
				texture(GLTexture::create_as_auto_ptr(renderer_)),
				source_cache_handle(source_cache_handle_)
			{  }

			/**
			 * Clears the source cache.
			 *
			 * Called when 'this' tile texture is returned to the cache (so texture can be reused).
			 */
			void
			returned_to_cache()
			{
				source_cache_handle.reset();
			}

			GLTexture::shared_ptr_type texture;
			GLMultiResolutionStaticPolygonReconstructedRaster::cache_handle_type source_cache_handle;
		};

		/**
		 * Typedef for a cache of tile textures.
		 */
		typedef GPlatesUtils::ObjectCache<TileTexture> tile_texture_cache_type;

	public:
		//! A convenience typedef for a shared pointer to a non-const @a GLMultiResolutionCubeReconstructedRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<GLMultiResolutionCubeReconstructedRaster> non_null_ptr_type;

		//! A convenience typedef for a shared pointer to a const @a GLMultiResolutionCubeReconstructedRaster.
		typedef GPlatesUtils::non_null_intrusive_ptr<const GLMultiResolutionCubeReconstructedRaster> non_null_ptr_to_const_type;

		/**
		 * Typedef for an opaque object that caches a particular tile of this raster.
		 */
		typedef GLMultiResolutionCubeRasterInterface::cache_handle_type cache_handle_type;

		/**
		 * Typedef for a quad tree node.
		 */
		typedef GLMultiResolutionCubeRasterInterface::quad_tree_node_type quad_tree_node_type;


		/**
		 * The default tile dimension is 256.
		 *
		 * This size gives us a small enough tile region on the globe to make good use
		 * of view frustum culling of tiles.
		 */
		static const std::size_t DEFAULT_TILE_TEXEL_DIMENSION = 256;


		/**
		 * Creates a @a GLMultiResolutionCubeReconstructedRaster object.
		 *
		 * @a tile_texel_dimension is the dimension of each square tile texture (returned by @a get_tile_texture).
		 * If it is larger than the maximum supported texture dimension then it will be changed to the maximum.
		 * If 'GL_ARB_texture_non_power_of_two' is not supported then it will be changed to the
		 * next power-of-two (if it is not already a power-of-two).
		 *
		 * If @a cache_tile_textures is true then the tile textures will be cached.
		 */
		static
		non_null_ptr_type
		create(
				GLRenderer &renderer,
				const GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type &source_reconstructed_raster,
				std::size_t tile_texel_dimension = DEFAULT_TILE_TEXEL_DIMENSION,
				bool cache_tile_textures = true)
		{
			return non_null_ptr_type(
					new GLMultiResolutionCubeReconstructedRaster(
							renderer,
							source_reconstructed_raster,
							tile_texel_dimension,
							cache_tile_textures));
		}


		/**
		 * Sets the transform to apply to raster/geometries when rendering into the cube map.
		 */
		virtual
		void
		set_world_transform(
				const GLMatrix &world_transform);


		/**
		 * Returns a subject token that clients can observe to see if they need to update themselves
		 * (such as any cached data we render for them) by getting us to re-render.
		 */
		virtual
		const GPlatesUtils::SubjectToken &
		get_subject_token() const
		{
			// We'll just use the subject token of the raster source - if they don't change then neither do we.
			// If we had two input sources then we'd have to have our own subject token.
			return d_reconstructed_raster->get_subject_token();
		}


		/**
		 * Returns the quad tree root node of the specified cube face.
		 *
		 * Returns boost::none if the source raster does not overlap the specified cube face.
		 */
		virtual
		boost::optional<quad_tree_node_type>
		get_quad_tree_root_node(
				GPlatesMaths::CubeCoordinateFrame::CubeFaceType cube_face);


		/**
		 * Returns the specified child cube quad tree node of specified parent node.
		 *
		 * Returns boost::none if the source raster does not overlap the specified child node.
		 */
		virtual
		boost::optional<quad_tree_node_type>
		get_child_node(
				const quad_tree_node_type &parent_node,
				unsigned int child_x_offset,
				unsigned int child_y_offset);


		/**
		 * Returns the tile texel dimension passed into constructor.
		 */
		virtual
		std::size_t
		get_tile_texel_dimension() const
		{
			return d_tile_texel_dimension;
		}


		/**
		 * Returns the texture internal format that can be used if rendering to a texture as
		 * opposed to the main framebuffer.
		 *
		 * This is the internal format of the texture returned by @a get_tile_texture.
		 */
		GLint
		get_tile_texture_internal_format() const
		{
			// It's the same as our source raster input.
			return d_reconstructed_raster->get_target_texture_internal_format();
		}

	private:
		/**
		 * A node in the quad tree of a cube face.
		 */
		struct CubeQuadTreeNode
		{
			CubeQuadTreeNode(
					const GLTransform::non_null_ptr_to_const_type &view_transform_,
					const GLTransform::non_null_ptr_to_const_type &projection_transform_,
					const tile_texture_cache_type::volatile_object_ptr_type &tile_texture_) :
				d_view_transform(view_transform_),
				d_projection_transform(projection_transform_),
				d_tile_texture(tile_texture_)
			{  }

			//! View transform used to render source raster into current tile.
			GLTransform::non_null_ptr_to_const_type d_view_transform;

			//! Projection transform used to render source raster into current tile.
			GLTransform::non_null_ptr_to_const_type d_projection_transform;

			/**
			 * Keeps tracks of whether the source data has changed underneath us
			 * and we need to reload our texture.
			 */
			mutable GPlatesUtils::ObserverToken d_source_texture_observer_token;

			/**
			 * The texture representation of the raster data for this tile.
			 */
			tile_texture_cache_type::volatile_object_ptr_type d_tile_texture;
		};


		/**
		 * Typedef for a cube quad tree with nodes containing the type @a CubeQuadTreeNode.
		 *
		 * This is what is actually traversed by the user once the cube quad tree has been created.
		 */
		typedef GPlatesMaths::CubeQuadTree<CubeQuadTreeNode> cube_quad_tree_type;


		/**
		 * Implementation of base class node to return to the client.
		 */
		struct QuadTreeNodeImpl :
				public GLMultiResolutionCubeRasterInterface::QuadTreeNode::ImplInterface
		{
			QuadTreeNodeImpl(
					cube_quad_tree_type::node_type &cube_quad_tree_node_,
					GLMultiResolutionCubeReconstructedRaster &multi_resolution_cube_raster_,
					const GPlatesMaths::CubeQuadTreeLocation &cube_quad_tree_location_) :
				cube_quad_tree_node(cube_quad_tree_node_),
				multi_resolution_cube_raster(multi_resolution_cube_raster_),
				cube_quad_tree_location(cube_quad_tree_location_)
			{  }

			/**
			 * We always return false for reconstructed rasters because it's costly to determine
			 * if rendering into the current quad tree node will generate any raster data - and
			 * this effort would be wasted if the client decides to continue traversing down the
			 * quad tree (to get better raster resolution).
			 */
			virtual
			bool
			is_leaf_node() const
			{
				return false;
			}

			/**
			 * Returns texture of tile.
			 */
			virtual
			boost::optional<GLTexture::shared_ptr_to_const_type>
			get_tile_texture(
					GLRenderer &renderer,
					cache_handle_type &cache_handle) const
			{
				return multi_resolution_cube_raster.get_tile_texture(
						renderer,
						cube_quad_tree_node.get_element(),
						cache_handle);
			}


			/**
			 * Reference to the cube quad tree node containing the real data.
			 */
			cube_quad_tree_type::node_type &cube_quad_tree_node;

			/**
			 * Pointer to parent class so can delegate to it.
			 */
			GLMultiResolutionCubeReconstructedRaster &multi_resolution_cube_raster;

			/**
			 * Used to determine location of cube quad tree node so can build view/projection transforms.
			 */
			GPlatesMaths::CubeQuadTreeLocation cube_quad_tree_location;
		};


		/**
		 * The reconstructed raster we are rendering into our cube map.
		 */
		GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type d_reconstructed_raster;


		/**
		 * The number of texels along a tiles edge (horizontal or vertical since it's square).
		 */
		std::size_t d_tile_texel_dimension;

		/**
		 * Cache of tile textures.
		 */
		tile_texture_cache_type::shared_ptr_type d_texture_cache;

		/**
		 * If true then we cache the tile textures.
		 */
		bool d_cache_tile_textures;

		/**
		 * Used to calculate projection transforms for the cube quad tree.
		 */
		GLCubeSubdivision::non_null_ptr_to_const_type d_cube_subdivision;

		/**
		 * The cube quad tree.
		 *
		 * This is what the user will traverse once we've built the cube quad tree raster.
		 */
		cube_quad_tree_type::non_null_ptr_type d_cube_quad_tree;

		/**
		 * The transform to use when rendering into the cube quad tree tiles.
		 */
		GLMatrix d_world_transform;


		//! Constructor.
		GLMultiResolutionCubeReconstructedRaster(
				GLRenderer &renderer,
				const GLMultiResolutionStaticPolygonReconstructedRaster::non_null_ptr_type &source_reconstructed_raster,
				std::size_t tile_texel_dimension,
				bool cache_tile_textures);

		boost::optional<GLTexture::shared_ptr_to_const_type>
		get_tile_texture(
				GLRenderer &renderer,
				const CubeQuadTreeNode &tile,
				cache_handle_type &cache_handle);

		bool
		render_raster_data_into_tile_texture(
				GLRenderer &renderer,
				const CubeQuadTreeNode &tile,
				TileTexture &tile_texture);

		void
		create_tile_texture(
				GLRenderer &renderer,
				const GLTexture::shared_ptr_type &tile_texture);

		/**
		 * Gets our quad tree node impl from the client's tile handle.
		 */
		static
		QuadTreeNodeImpl &
		get_quad_tree_node_impl(
				const quad_tree_node_type &tile)
		{
			return dynamic_cast<QuadTreeNodeImpl &>(tile.get_impl());
		}
	};
}

#endif // GPLATES_OPENGL_GLMULTIRESOLUTIONCUBERECONSTRUCTEDRASTER_H
