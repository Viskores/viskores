//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_MapperGlyphBase_h
#define vtk_m_rendering_MapperGlyphBase_h

#include <vtkm/Deprecated.h>
#include <vtkm/cont/ColorTable.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Mapper.h>

#include <memory>

namespace vtkm
{
namespace rendering
{

class CanvasRayTracer;

/// @brief Base class for glyph mappers.
///
/// Glyph mappers place 3D icons at various places in the mesh. The icons are
/// placed based on the location of points or cells in the mesh.
class VTKM_RENDERING_EXPORT MapperGlyphBase : public Mapper
{
public:
  MapperGlyphBase();

  virtual ~MapperGlyphBase();

  void SetCanvas(vtkm::rendering::Canvas* canvas) override;
  virtual vtkm::rendering::Canvas* GetCanvas() const override;

  /// @brief Specify the elements the glyphs will be associated with.
  ///
  /// The glyph mapper will place glyphs over locations specified by either the points
  /// or the cells of a mesh. The glyph may also be oriented by a scalar field with the
  /// same association.
  virtual vtkm::cont::Field::Association GetAssociation() const;
  /// @copydoc GetAssociation
  virtual void SetAssociation(vtkm::cont::Field::Association association);
  /// @copydoc GetAssociation
  virtual bool GetUseCells() const;
  /// @copydoc GetAssociation
  virtual void SetUseCells();
  /// @copydoc GetAssociation
  virtual bool GetUsePoints() const;
  /// @copydoc GetAssociation
  virtual void SetUsePoints();
  VTKM_DEPRECATED(2.2, "Use GetUsePoints() or GetAssociation().")
  virtual bool GetUseNodes() const;
  VTKM_DEPRECATED(2.2, "Use SetUsePoints() or SetAssociation().")
  virtual void SetUseNodes();

  // These options do not seem to be supported yet.
  // I'm not sure why you would need UseStride. Just use Stride = 1.
  virtual bool GetUseStride() const;
  virtual void SetUseStride(bool on);
  virtual vtkm::Id GetStride() const;
  virtual void SetStride(vtkm::Id stride);

  /// @brief Specify the size of each glyph (before scaling).
  ///
  /// If the base size is not set to a positive value, it is automatically sized with a heuristic
  /// based off the bounds of the geometry.
  virtual vtkm::Float32 GetBaseSize() const;
  /// @copydoc GetBaseSize
  virtual void SetBaseSize(vtkm::Float32 size);

  /// @brief Specify whether to scale the glyphs by a field.
  virtual bool GetScaleByValue() const;
  /// @copydoc GetScaleByValue
  virtual void SetScaleByValue(bool on);

  /// @brief Specify the range of values to scale the glyphs.
  ///
  /// When `ScaleByValue` is on, the glyphs will be scaled proportionally to the field
  /// magnitude. The `ScaleDelta` determines how big and small they get. For a `ScaleDelta`
  /// of one, the smallest field values will have glyphs of zero size and the maximum field
  /// values will be twice the base size. A `ScaleDelta` of 0.5 will result in glyphs sized
  /// in the range of 0.5 times the base size to 1.5 times the base size. `ScaleDelta` outside
  /// the range [0, 1] is undefined.
  virtual vtkm::Float32 GetScaleDelta() const;
  /// @copydoc GetScaleDelta
  virtual void SetScaleDelta(vtkm::Float32 delta);

  virtual void SetCompositeBackground(bool on);

protected:
  virtual vtkm::cont::DataSet FilterPoints(const vtkm::cont::UnknownCellSet& cellSet,
                                           const vtkm::cont::CoordinateSystem& coords,
                                           const vtkm::cont::Field& scalarField) const;


  vtkm::rendering::CanvasRayTracer* Canvas = nullptr;
  bool CompositeBackground = true;

  vtkm::cont::Field::Association Association = vtkm::cont::Field::Association::Points;

  bool UseStride = false;
  vtkm::Id Stride = 1;

  bool ScaleByValue = false;
  vtkm::Float32 BaseSize = -1.f;
  vtkm::Float32 ScaleDelta = 0.5f;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_MapperGlyphBase_h
