//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_MapperGlyphVector_h
#define vtk_m_rendering_MapperGlyphVector_h

#include <vtkm/rendering/GlyphType.h>
#include <vtkm/rendering/MapperGlyphBase.h>

namespace vtkm
{
namespace rendering
{

/// @brief A mapper that produces oriented glyphs.
///
/// This mapper is meant to be used with 3D vector fields. The glyphs are oriented in
/// the direction of the vector field. The glyphs can be optionally sized based on the
/// magnitude of the field.
class VTKM_RENDERING_EXPORT MapperGlyphVector : public vtkm::rendering::MapperGlyphBase
{
public:
  MapperGlyphVector();

  ~MapperGlyphVector();

  /// @brief Specify the shape of the glyphs.
  vtkm::rendering::GlyphType GetGlyphType() const;
  /// @copydoc GetGlyphType
  void SetGlyphType(vtkm::rendering::GlyphType glyphType);

  vtkm::rendering::Mapper* NewCopy() const override;

protected:
  vtkm::rendering::GlyphType GlyphType;

  void RenderCellsImpl(const vtkm::cont::UnknownCellSet& cellset,
                       const vtkm::cont::CoordinateSystem& coords,
                       const vtkm::cont::Field& scalarField,
                       const vtkm::cont::ColorTable& colorTable,
                       const vtkm::rendering::Camera& camera,
                       const vtkm::Range& scalarRange,
                       const vtkm::cont::Field& ghostField) override;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_MapperGlyphVector_h
