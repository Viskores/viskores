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

#include <viskores/cont/ArrayCopy.h>
#include <viskores/cont/CellSetStructured.h>
#include <viskores/cont/ErrorBadValue.h>
#include <viskores/cont/SplineEvaluateRectilinearGrid.h>
#include <viskores/cont/internal/ArrayGetMonotonicity.h>

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

  viskores::cont::CoordinateSystem coordSystem = this->DataSet.GetCoordinateSystem();
  RectCoordsType coords = coordSystem.GetData().template AsArrayHandle<RectCoordsType>();
  using Monotonicity = viskores::cont::internal::ArrayMonotonicity;
  if (viskores::cont::internal::ArrayGetStrictMonotonicity(coords.GetFirstArray()) ==
        Monotonicity::NotMonotonic ||
      viskores::cont::internal::ArrayGetStrictMonotonicity(coords.GetSecondArray()) ==
        Monotonicity::NotMonotonic ||
      viskores::cont::internal::ArrayGetStrictMonotonicity(coords.GetThirdArray()) ==
        Monotonicity::NotMonotonic)
  {
    throw viskores::cont::ErrorBadValue("Rectilinear coordinate axes must be strictly monotonic.");
  }

  viskores::cont::ArrayHandle<viskores::FloatDefault> fieldArray;
  viskores::cont::ArrayCopyShallowIfPossible(this->DataSet.GetField(this->FieldName).GetData(),
                                             fieldArray);

  return viskores::exec::SplineEvaluateRectilinearGrid(
    coords, fieldArray, coordSystem.GetBounds(), device, token);
}

} //namespace cont
} //namespace viskores
