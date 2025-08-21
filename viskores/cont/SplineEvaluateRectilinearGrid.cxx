//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/SplineEvaluateRectilinearGrid.h>

namespace viskores
{
namespace cont
{

viskores::exec::SplineEvaluateRectilinearGrid SplineEvaluateRectilinearGrid::PrepareForExecution(
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token) const
{
  using AxisType = viskores::cont::ArrayHandle<viskores::FloatDefault>;
  using RectCoordsType = viskores::cont::ArrayHandleCartesianProduct<AxisType, AxisType, AxisType>;

  if (!this->DataSet.GetCoordinateSystem(0).GetData().IsType<RectCoordsType>())
    throw viskores::cont::ErrorBadType("Coordinates are not rectilinear type.");

  viskores::cont::ArrayHandle<viskores::FloatDefault> fieldArray;
  this->DataSet.GetField(this->FieldName).GetData().AsArrayHandle(fieldArray);

  RectCoordsType coords;
  coords = this->DataSet.GetCoordinateSystem(0).GetData().template AsArrayHandle<RectCoordsType>();

  return viskores::exec::SplineEvaluateRectilinearGrid(coords.PrepareForInput(device, token),
                                                       fieldArray.PrepareForInput(device, token));
}

} //namespace cont
} //namespace viskores
