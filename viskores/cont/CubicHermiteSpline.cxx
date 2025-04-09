//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <viskores/BinaryOperators.h>
#include <viskores/VectorAnalysis.h>
#include <viskores/cont/Algorithm.h>
#include <viskores/cont/ArrayHandleConstant.h>
#include <viskores/cont/CubicHermiteSpline.h>
#include <viskores/exec/CubicHermiteSpline.h>
#include <viskores/worklet/WorkletMapField.h>

namespace viskores
{
namespace cont
{
namespace
{
struct CalcNeighborDistanceWorklet : public viskores::worklet::WorkletMapField
{
  using ControlSignature = void(FieldOut, WholeArrayIn);
  using ExecutionSignature = void(InputIndex, _1, _2);

  template <typename ArrayType>
  VISKORES_EXEC void operator()(const viskores::Id& idx,
                            viskores::FloatDefault& val,
                            const ArrayType& data) const
  {
    if (idx == 0)
      val = 0.0;
    else
      val = viskores::Magnitude(data.Get(idx) - data.Get(idx - 1));
  }
};

struct CalcTangentsWorklet : public viskores::worklet::WorkletMapField
{
  CalcTangentsWorklet(const viskores::Id& numPoints)
    : NumPoints(numPoints)
  {
  }

  using ControlSignature = void(FieldOut, WholeArrayIn, WholeArrayIn);
  using ExecutionSignature = void(InputIndex, _1, _2, _3);

  template <typename TangentArrayType, typename PointArrayType, typename KnotArrayType>
  VISKORES_EXEC void operator()(const viskores::Id& idx,
                            TangentArrayType& tangent,
                            const PointArrayType& points,
                            const KnotArrayType& knots) const
  {
    viskores::Id idx0, idx1;
    if (idx == 0) // Forward difference
    {
      idx0 = 0;
      idx1 = 1;
    }
    else if (idx == NumPoints - 1) // Backward difference
    {
      idx0 = NumPoints - 2;
      idx1 = NumPoints - 1;
    }
    else // central difference
    {
      idx0 = idx - 1;
      idx1 = idx + 1;
    }

    auto dX = points.Get(idx1) - points.Get(idx0);
    auto dT = knots.Get(idx1) - knots.Get(idx0);

    tangent = dX / dT;
  }

  viskores::Id NumPoints;
};
} //anonymous namespace

VISKORES_CONT viskores::Range CubicHermiteSpline::GetParametricRange()
{
  auto n = this->Knots.GetNumberOfValues();
  if (n == 0)
  {
    this->ComputeKnots();
    n = this->Knots.GetNumberOfValues();
  }
  const auto ids = viskores::cont::make_ArrayHandle<viskores::Id>({ 0, n - 1 });
  const std::vector<viskores::FloatDefault> output = viskores::cont::ArrayGetValues(ids, this->Knots);
  return { output[0], output[1] };
}

VISKORES_CONT void CubicHermiteSpline::ComputeKnots()
{
  viskores::Id n = this->Data.GetNumberOfValues();
  this->Knots.Allocate(n);

  viskores::cont::Invoker invoker;

  //uses chord length parameterization.
  invoker(CalcNeighborDistanceWorklet{}, this->Knots, this->Data);
  viskores::FloatDefault sum = viskores::cont::Algorithm::ScanInclusive(this->Knots, this->Knots);

  if (sum == 0.0)
    throw std::invalid_argument("Error: accumulated distance between data is zero.");

  auto divideBy = viskores::cont::make_ArrayHandleConstant(1.0 / sum, this->Knots.GetNumberOfValues());
  viskores::cont::Algorithm::Transform(this->Knots, divideBy, this->Knots, viskores::Product{});
}

VISKORES_CONT void CubicHermiteSpline::ComputeTangents()
{
  viskores::Id n = this->Data.GetNumberOfValues();
  this->Tangents.Allocate(n);

  viskores::cont::Invoker invoker;

  invoker(CalcTangentsWorklet{ n }, this->Tangents, this->Data, this->Knots);
}

viskores::exec::CubicHermiteSpline CubicHermiteSpline::PrepareForExecution(
  viskores::cont::DeviceAdapterId device,
  viskores::cont::Token& token)
{
  viskores::Id n = this->Data.GetNumberOfValues();
  if (n < 2)
    throw viskores::cont::ErrorBadValue("At least two points are required for spline interpolation.");
  if (this->Knots.GetNumberOfValues() == 0)
    this->ComputeKnots();
  if (this->Tangents.GetNumberOfValues() == 0)
    this->ComputeTangents();

  if (n != this->Knots.GetNumberOfValues())
    throw viskores::cont::ErrorBadValue("Number of data points must match the number of knots.");
  if (n != this->Tangents.GetNumberOfValues())
    throw viskores::cont::ErrorBadValue("Number of data points must match the number of tangents.");

  using ExecObjType = viskores::exec::CubicHermiteSpline;

  return ExecObjType(this->Data, this->Knots, this->Tangents, device, token);
}
}
} // namespace viskores::cont
