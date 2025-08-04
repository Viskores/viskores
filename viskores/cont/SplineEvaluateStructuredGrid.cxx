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

#include <viskores/cont/ArrayHandleUniformPointCoordinates.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/SplineEvaluateStructuredGrid.h>

namespace viskores
{
namespace cont
{

using UniformCoordsType = viskores::cont::ArrayHandleUniformPointCoordinates;
using Structured2DType = viskores::cont::CellSetStructured<2>;
using Structured3DType = viskores::cont::CellSetStructured<3>;

using RectCoordsType =
  viskores::cont::ArrayHandleCartesianProduct<viskores::cont::ArrayHandle<viskores::FloatDefault>,
                                              viskores::cont::ArrayHandle<viskores::FloatDefault>,
                                              viskores::cont::ArrayHandle<viskores::FloatDefault>>;


viskores::exec::SplineEvaluateStructuredGrid SplineEvaluateStructuredGrid::PrepareForExecution(
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token) const
{
  viskores::cont::ArrayHandle<viskores::FloatDefault> fieldArray;
  this->DataSet.GetField(this->FieldName).GetData().AsArrayHandle(fieldArray);
  auto cellSet = this->DataSet.GetCellSet();
  bool is3D;
  if (cellSet.IsType<Structured2DType>())
    is3D = false;
  else if (cellSet.IsType<Structured3DType>())
    is3D = true;

  if (this->DataSet.GetCoordinateSystem(0).GetData().IsType<UniformCoordsType>())
  {
    std::cout << __FILE__ << " " << __LINE__ << std::endl;

    auto coords = this->DataSet.GetCoordinateSystem(0).GetData().AsArrayHandle<UniformCoordsType>();

    auto origin = coords.GetOrigin();
    auto spacing = coords.GetSpacing();
    auto dims = coords.GetDimensions();

    return viskores::exec::SplineEvaluateStructuredGrid(
      origin, spacing, dims, fieldArray.PrepareForInput(device, token));
  }
  else if (this->DataSet.GetCoordinateSystem(0).GetData().IsType<RectCoordsType>())
  {
    RectCoordsType coords;
    coords = this->DataSet.GetCoordinateSystem(0).GetData().AsArrayHandle<RectCoordsType>();

    return viskores::exec::SplineEvaluateStructuredGrid(coords,
                                                        fieldArray.PrepareForInput(device, token));
  }

  std::cout << __FILE__ << " " << __LINE__ << std::endl;
  return viskores::exec::SplineEvaluateStructuredGrid();
}

} //namespace cont
} //namespace viskores
