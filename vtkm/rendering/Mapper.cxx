//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/rendering/Mapper.h>

#include <vtkm/cont/ColorTable.hxx>

namespace vtkm
{
namespace rendering
{

Mapper::~Mapper()
{
}

void Mapper::SetActiveColorTable(const vtkm::cont::ColorTable& colorTable)
{

  constexpr vtkm::Float32 conversionToFloatSpace = (1.0f / 255.0f);

  vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::UInt8, 4>> temp;

  {
    vtkm::cont::ScopedRuntimeDeviceTracker tracker(vtkm::cont::DeviceAdapterTagSerial{});
    colorTable.Sample(1024, temp);
  }

  this->ColorMap.Allocate(1024);
  auto portal = this->ColorMap.GetPortalControl();
  auto colorPortal = temp.GetPortalConstControl();
  for (vtkm::Id i = 0; i < 1024; ++i)
  {
    auto color = colorPortal.Get(i);
    vtkm::Vec<vtkm::Float32, 4> t(color[0] * conversionToFloatSpace,
                                  color[1] * conversionToFloatSpace,
                                  color[2] * conversionToFloatSpace,
                                  color[3] * conversionToFloatSpace);
    portal.Set(i, t);
  }
}

void Mapper::SetLogarithmX(bool l)
{
  this->LogarithmX = l;
}

void Mapper::SetLogarithmY(bool l)
{
  this->LogarithmY = l;
}
}
}
