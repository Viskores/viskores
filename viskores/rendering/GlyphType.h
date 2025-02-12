//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_rendering_GlyphType_h
#define viskores_rendering_GlyphType_h

namespace viskores
{
namespace rendering
{

/// @brief Glyph shapes supported by glyphing mappers.
enum struct GlyphType
{
  Arrow,
  Axes,
  Cube,
  Quad,
  Sphere,
};

}
}

#endif
