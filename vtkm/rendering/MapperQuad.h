//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_MapperQuad_h
#define vtk_m_rendering_MapperQuad_h

#include <vtkm/cont/ColorTable.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Mapper.h>

#include <memory>

namespace vtkm
{
namespace rendering
{

/**
 * \brief MapperQuad renderers quad facess from a cell set via ray tracing.
 *        As opposed to breaking quads into two trianges, scalars are
 *        interpolated using all 4 points of the quad resulting in more
 *        accurate interpolation.
 */
class VTKM_RENDERING_EXPORT MapperQuad : public Mapper
{
public:
  MapperQuad();

  ~MapperQuad();

  void SetCanvas(vtkm::rendering::Canvas* canvas) override;
  virtual vtkm::rendering::Canvas* GetCanvas() const override;

  void RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                   const vtkm::cont::CoordinateSystem& coords,
                   const vtkm::cont::Field& scalarField,
                   const vtkm::cont::ColorTable& colorTable,
                   const vtkm::rendering::Camera& camera,
                   const vtkm::Range& scalarRange) override;

  virtual void StartScene() override;
  virtual void EndScene() override;
  void SetCompositeBackground(bool on);
  vtkm::rendering::Mapper* NewCopy() const override;

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;

  struct RenderFunctor;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_MapperQuad_h
