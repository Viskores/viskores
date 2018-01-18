//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_rendering_MapperConnectivity_h
#define vtk_m_rendering_MapperConnectivity_h

#include <vtkm/cont/ColorTable.h>
#include <vtkm/rendering/CanvasRayTracer.h>
#include <vtkm/rendering/Mapper.h>
#include <vtkm/rendering/View.h>

namespace vtkm
{
namespace rendering
{

class VTKM_RENDERING_EXPORT MapperConnectivity : public Mapper
{
public:
  MapperConnectivity();

  ~MapperConnectivity();
  void SetSampleDistance(const vtkm::Float32&);
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
  void CreateDefaultView();

protected:
  vtkm::Float32 SampleDistance;
  CanvasRayTracer* CanvasRT;
};
}
} //namespace vtkm::rendering
#endif //vtk_m_rendering_SceneRendererVolume_h
