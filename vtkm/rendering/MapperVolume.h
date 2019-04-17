//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_MapperVolume_h
#define vtk_m_rendering_MapperVolume_h

#include <vtkm/rendering/Mapper.h>

#include <memory>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT MapperVolume : public Mapper
{
public:
  MapperVolume();

  ~MapperVolume();

  void SetCanvas(vtkm::rendering::Canvas* canvas) override;
  virtual vtkm::rendering::Canvas* GetCanvas() const override;

  virtual void RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                           const vtkm::cont::CoordinateSystem& coords,
                           const vtkm::cont::Field& scalarField,
                           const vtkm::cont::ColorTable&, //colorTable
                           const vtkm::rendering::Camera& camera,
                           const vtkm::Range& scalarRange) override;

  virtual void StartScene() override;
  virtual void EndScene() override;

  vtkm::rendering::Mapper* NewCopy() const override;
  void SetSampleDistance(const vtkm::Float32 distance);
  void SetCompositeBackground(const bool compositeBackground);

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;
};
}
} //namespace vtkm::rendering

#endif //vtk_m_rendering_MapperVolume_h
