//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_MapperPoint_h
#define vtk_m_rendering_MapperPoint_h

#include <vtkm/Deprecated.h>
#include <vtkm/cont/ColorTable.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Mapper.h>

#include <memory>

namespace vtkm
{
namespace rendering
{

/// @brief This mapper renders points from a cell set.
///
/// This mapper can natively create points from
/// vertex cell shapes as well as use the points
/// defined by a coordinate system.
class VTKM_RENDERING_EXPORT MapperPoint : public Mapper
{
public:
  MapperPoint();

  ~MapperPoint();

  void SetCanvas(vtkm::rendering::Canvas* canvas) override;
  virtual vtkm::rendering::Canvas* GetCanvas() const override;

  /// @brief Specify the elements the points will be associated with.
  ///
  /// The point mapper will place visible points over locations specified by either the points
  /// or the cells of a mesh.
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
  VTKM_DEPRECATED(2.2, "Use SetUseCells or SetAssociation.")
  void UseCells();
  VTKM_DEPRECATED(2.2, "Use SetUsePoints or SetAssociation.")
  void UseNodes();

  ///
  /// @brief Render points using a variable radius based on the scalar field.
  ///
  /// The default is false.
  void UseVariableRadius(bool useVariableRadius);

  /// @brief Set a base raidus for all points.
  ///
  /// If a radius is never specified the default heuristic is used.
  void SetRadius(const vtkm::Float32& radius);

  /// When using a variable raidus for all points, the
  /// radius delta controls how much larger and smaller
  /// radii become based on the scalar field. If the delta
  /// is 0 all points will have the same radius. If the delta
  /// is 0.5 then the max/min scalar values would have a radii
  /// of base +/- base * 0.5.
  void SetRadiusDelta(const vtkm::Float32& delta);

  void SetCompositeBackground(bool on);
  vtkm::rendering::Mapper* NewCopy() const override;

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;

  struct RenderFunctor;

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

#endif //vtk_m_rendering_MapperPoint_h
