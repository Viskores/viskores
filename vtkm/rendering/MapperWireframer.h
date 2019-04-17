//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_rendering_MapperWireframer_h
#define vtk_m_rendering_MapperWireframer_h

#include <memory>

#include <vtkm/cont/ColorTable.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/Field.h>
#include <vtkm/rendering/Camera.h>
#include <vtkm/rendering/Canvas.h>
#include <vtkm/rendering/Mapper.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT MapperWireframer : public Mapper
{
public:
  VTKM_CONT
  MapperWireframer();
  virtual ~MapperWireframer();

  virtual vtkm::rendering::Canvas* GetCanvas() const override;
  virtual void SetCanvas(vtkm::rendering::Canvas* canvas) override;

  bool GetShowInternalZones() const;
  void SetShowInternalZones(bool showInternalZones);
  void SetCompositeBackground(bool on);

  bool GetIsOverlay() const;
  void SetIsOverlay(bool isOverlay);

  virtual void StartScene() override;
  virtual void EndScene() override;

  virtual void RenderCells(const vtkm::cont::DynamicCellSet& cellset,
                           const vtkm::cont::CoordinateSystem& coords,
                           const vtkm::cont::Field& scalarField,
                           const vtkm::cont::ColorTable& colorTable,
                           const vtkm::rendering::Camera& camera,
                           const vtkm::Range& scalarRange) override;

  virtual vtkm::rendering::Mapper* NewCopy() const override;

private:
  struct InternalsType;
  std::shared_ptr<InternalsType> Internals;
}; // class MapperWireframer
}
} // namespace vtkm::rendering
#endif // vtk_m_rendering_MapperWireframer_h
