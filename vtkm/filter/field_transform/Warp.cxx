//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/StaticAssert.h>
#include <vtkm/TypeTraits.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleRuntimeVec.h>
#include <vtkm/cont/ErrorBadValue.h>
#include <vtkm/filter/field_transform/Warp.h>
#include <vtkm/worklet/WorkletMapField.h>

#include <type_traits>

namespace
{

struct WarpWorklet : vtkm::worklet::WorkletMapField
{
  vtkm::FloatDefault ScaleFactor;

  VTKM_CONT explicit WarpWorklet(vtkm::FloatDefault scale)
    : ScaleFactor(scale)
  {
  }

  using ControlSignature = void(FieldIn pointCoordinates,
                                FieldIn directions,
                                FieldIn scales,
                                FieldOut result);

  template <typename PointType, typename DirectionType, typename ScaleType, typename ResultType>
  VTKM_EXEC void operator()(const PointType& point,
                            const DirectionType& direction,
                            ScaleType scale,
                            ResultType& result) const
  {
    vtkm::IdComponent numComponents = result.GetNumberOfComponents();
    VTKM_ASSERT(point.GetNumberOfComponents() == numComponents);
    VTKM_ASSERT(direction.GetNumberOfComponents() == numComponents);

    result = direction;
    result *= scale * this->ScaleFactor;
    result += point;
  }
};

// The warp filter operates on 3 arrays: coordiantes, directions, and scale factors. Rather than
// try to satisfy every possible array type we expect, which would add to a lot of possibilities
// (especially because we add the constant varieties), we will just extract components as either
// `vtkm::Float32` or `vtkm::Float64`. That way for each we just need just 6 combinations. We can
// do this by extracting arrays by components using `UnknownArrayHandle`'s
// `ExtractArrayFromComponents`.
template <typename Functor>
VTKM_CONT void CastAndCallExtractedArrayFloats(const vtkm::cont::UnknownArrayHandle& array,
                                               Functor&& functor)
{
  if (array.IsBaseComponentType<vtkm::Float32>())
  {
    functor(array.ExtractArrayFromComponents<vtkm::Float32>());
  }
  else if (array.IsBaseComponentType<vtkm::Float64>())
  {
    functor(array.ExtractArrayFromComponents<vtkm::Float64>());
  }
  else
  {
    // Array is not float. Copy it to a float array and call the functor.
    vtkm::cont::ArrayHandleRuntimeVec<vtkm::FloatDefault> arrayCopy{
      array.GetNumberOfComponentsFlat()
    };
    vtkm::cont::ArrayCopy(array, arrayCopy);

    // We could call the functor directly on arrayCopy. But that would add a third
    // type of array. We would like to limit it to 2 types. Thus, stuff the known
    // array into its own `UnknownArrayHandle` and get an extracted array that will
    // match the others.
    vtkm::cont::UnknownArrayHandle arrayCopyContainer = arrayCopy;
    functor(arrayCopyContainer.ExtractArrayFromComponents<vtkm::FloatDefault>());
  }
}

template <typename T1, typename T2>
struct BiggerTypeImpl
{
  VTKM_STATIC_ASSERT(
    (std::is_same<typename vtkm::TypeTraits<T1>::NumericTag, vtkm::TypeTraitsRealTag>::value));
  VTKM_STATIC_ASSERT(
    (std::is_same<typename vtkm::TypeTraits<T2>::NumericTag, vtkm::TypeTraitsRealTag>::value));
  VTKM_STATIC_ASSERT((std::is_same<typename vtkm::TypeTraits<T1>::DimensionalityTag,
                                   vtkm::TypeTraitsScalarTag>::value));
  VTKM_STATIC_ASSERT((std::is_same<typename vtkm::TypeTraits<T2>::DimensionalityTag,
                                   vtkm::TypeTraitsScalarTag>::value));
  using type = std::conditional_t<(sizeof(T1) > sizeof(T2)), T1, T2>;
};
template <typename T1, typename T2>
using BiggerType = typename BiggerTypeImpl<T1, T2>::type;

template <typename CoordinateType, typename DirectionType, typename ScalarFactorType>
VTKM_CONT vtkm::cont::UnknownArrayHandle ComputeWarp(
  const vtkm::cont::Invoker& invoke,
  const vtkm::cont::ArrayHandleRecombineVec<CoordinateType>& points,
  const vtkm::cont::ArrayHandleRecombineVec<DirectionType>& directions,
  const vtkm::cont::ArrayHandleRecombineVec<ScalarFactorType>& scales,
  vtkm::FloatDefault scaleFactor)
{
  vtkm::IdComponent numComponents = points.GetNumberOfComponents();
  if (directions.GetNumberOfComponents() != numComponents)
  {
    throw vtkm::cont::ErrorBadValue(
      "Number of components for points and directions does not agree.");
  }

  if (scales.GetNumberOfComponents() != 1)
  {
    throw vtkm::cont::ErrorBadValue("ScaleField must be scalars, but they are not.");
  }
  auto scalarFactorsComponents = scales.GetComponentArray(0);

  using ResultType = BiggerType<BiggerType<CoordinateType, DirectionType>, ScalarFactorType>;
  vtkm::cont::ArrayHandleRuntimeVec<ResultType> result{ numComponents };

  invoke(WarpWorklet{ scaleFactor }, points, directions, scalarFactorsComponents, result);

  return result;
}

template <typename CoordinateType, typename DirectionType>
VTKM_CONT vtkm::cont::UnknownArrayHandle ComputeWarp(
  const vtkm::cont::Invoker& invoke,
  const vtkm::cont::ArrayHandleRecombineVec<CoordinateType>& points,
  const vtkm::cont::ArrayHandleRecombineVec<DirectionType>& directions,
  const vtkm::cont::UnknownArrayHandle& scales,
  vtkm::FloatDefault scaleFactor)
{
  vtkm::cont::UnknownArrayHandle result;
  auto functor = [&](auto concrete) {
    result = ComputeWarp(invoke, points, directions, concrete, scaleFactor);
  };
  CastAndCallExtractedArrayFloats(scales, functor);
  return result;
}

template <typename CoordinateType>
VTKM_CONT vtkm::cont::UnknownArrayHandle ComputeWarp(
  const vtkm::cont::Invoker& invoke,
  const vtkm::cont::ArrayHandleRecombineVec<CoordinateType>& points,
  const vtkm::cont::UnknownArrayHandle& directions,
  const vtkm::cont::UnknownArrayHandle& scales,
  vtkm::FloatDefault scaleFactor)
{
  vtkm::cont::UnknownArrayHandle result;
  auto functor = [&](auto concrete) {
    result = ComputeWarp(invoke, points, concrete, scales, scaleFactor);
  };
  CastAndCallExtractedArrayFloats(directions, functor);
  return result;
}

VTKM_CONT vtkm::cont::UnknownArrayHandle ComputeWarp(
  const vtkm::cont::Invoker& invoke,
  const vtkm::cont::UnknownArrayHandle& points,
  const vtkm::cont::UnknownArrayHandle& directions,
  const vtkm::cont::UnknownArrayHandle& scales,
  vtkm::FloatDefault scaleFactor)
{
  vtkm::cont::UnknownArrayHandle result;
  auto functor = [&](auto concrete) {
    result = ComputeWarp(invoke, concrete, directions, scales, scaleFactor);
  };
  CastAndCallExtractedArrayFloats(points, functor);
  return result;
}

} // anonymous namespace

namespace vtkm
{
namespace filter
{
namespace field_transform
{

//-----------------------------------------------------------------------------
VTKM_CONT Warp::Warp()
{
  this->SetOutputFieldName("Warp");
  this->SetUseCoordinateSystemAsField(0, true);
  this->SetActiveField(1, "direction", vtkm::cont::Field::Association::Points);
  this->SetActiveField(2, "scale", vtkm::cont::Field::Association::Points);
}

//-----------------------------------------------------------------------------
VTKM_CONT vtkm::cont::DataSet Warp::DoExecute(const vtkm::cont::DataSet& inDataSet)
{
  vtkm::cont::Field pointField = this->GetFieldFromDataSet(0, inDataSet);
  vtkm::cont::UnknownArrayHandle points = pointField.GetData();

  vtkm::cont::UnknownArrayHandle directions;
  if (this->GetUseConstantDirection())
  {
    directions = vtkm::cont::make_ArrayHandleConstant(this->GetConstantDirection(),
                                                      points.GetNumberOfValues());
  }
  else
  {
    directions = this->GetFieldFromDataSet(1, inDataSet).GetData();
  }

  vtkm::cont::UnknownArrayHandle scaleFactors;
  if (this->GetUseScaleField())
  {
    scaleFactors = this->GetFieldFromDataSet(2, inDataSet).GetData();
  }
  else
  {
    scaleFactors =
      vtkm::cont::make_ArrayHandleConstant<vtkm::FloatDefault>(1, points.GetNumberOfValues());
  }

  vtkm::cont::UnknownArrayHandle warpedPoints =
    ComputeWarp(this->Invoke, points, directions, scaleFactors, this->ScaleFactor);

  if (this->GetChangeCoordinateSystem())
  {
    auto fieldMapper = [](vtkm::cont::DataSet& out, const vtkm::cont::Field& fieldToPass) {
      out.AddField(fieldToPass);
    };
    return this->CreateResultCoordinateSystem(
      inDataSet, inDataSet.GetCellSet(), this->GetOutputFieldName(), warpedPoints, fieldMapper);
  }
  else
  {
    return this->CreateResultField(
      inDataSet, this->GetOutputFieldName(), pointField.GetAssociation(), warpedPoints);
  }
}

} // namespace field_transform
} // namespace filter
} // namespace vtkm
