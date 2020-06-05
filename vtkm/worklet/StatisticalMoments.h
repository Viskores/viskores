//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_StatisticalMoments_h
#define vtk_m_worklet_StatisticalMoments_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayHandleTransform.h>

namespace vtkm
{
namespace worklet
{
namespace detail
{

// TODO: find a better name
// TODO: move it out of detail namespace?
template <typename T>
struct StatState
{
  StatState() = default;
  StatState(T value)
    : n(1)
    , min(value)
    , max(value)
    , sum(value)
    , mean(value)
  {
  }

  VTKM_EXEC_CONT
  StatState operator+(const StatState<T>& y) const
  {
    const StatState<T>& x = *this;
    StatState result;

    result.n = x.n + y.n;

    result.min = vtkm::Min(x.min, y.min);
    result.max = vtkm::Max(x.max, y.max);

    result.sum = x.sum + y.sum;
    // We calculate mean in each "reduction" from sum and n
    // this saves one multiplication and hopefully we don't
    // accumulate more error this way.
    result.mean = result.sum / result.n;

    T delta = y.mean - x.mean;
    T delta2 = delta * delta;
    result.M2 = x.M2 + y.M2 + delta2 * x.n * y.n / result.n;

    T delta3 = delta * delta2;
    T n2 = result.n * result.n;
    result.M3 = x.M3 + y.M3;
    result.M3 += delta3 * x.n * y.n * (x.n - y.n) / n2;
    result.M3 += T{ 3.0 } * delta * (x.n * y.M2 - y.n * x.M2) / result.n;

    T delta4 = delta * delta3;
    T n3 = result.n * n2;
    result.M4 = x.M4 + y.M4;
    result.M4 += delta4 * x.n * y.n * (x.n * x.n - x.n * y.n + y.n * y.n) / n3;
    result.M4 += T{ 6.0 } * delta2 * (x.n * x.n * y.M2 + y.n * y.n * x.M2) / n2;
    result.M4 += T{ 4.0 } * delta * (x.n * y.M3 - y.n * x.M3) / result.n;

    return result;
  }

  VTKM_CONT
  T variance() const { return this->M2 / (this->n - 1); }

  VTKM_CONT
  T variance_n() const { return this->M2 / this->n; }

  VTKM_CONT
  T skewness() const { return vtkm::Sqrt(this->n) * this->M3 / vtkm::Pow(this->M2, T{ 1.5 }); }

  VTKM_CONT
  T kurtosis() const { return this->n * this->M4 / (this->M2 * this->M2); }

  // TODO: higher moments, raw v.s. central moments
  T n = T{};
  T min = std::numeric_limits<T>::max();
  T max = std::numeric_limits<T>::min();
  T sum = T{};
  T mean = T{};
  T M2 = T{};
  T M3 = T{};
  T M4 = T{};
}; // StatState

struct MakeStatState
{
  template <typename T>
  VTKM_EXEC_CONT StatState<T> operator()(T value) const
  {
    return StatState<T>{ value };
  }
};
} // detail

class StatisticalMoments
{
public:
  template <typename FieldType, typename Storage>
  VTKM_CONT static detail::StatState<FieldType> Run(
    vtkm::cont::ArrayHandle<FieldType, Storage> field)
  {
    using Algorithm = vtkm::cont::Algorithm;

    // TODO: the original FieldStatistics sorts the field first and do the reduction,
    // this supposedly reduce the amount of numerical error. Find out if it is universally
    // true.
    // Essentially a TransformReduce. Do we have that convenience in VTKm?
    auto states = vtkm::cont::make_ArrayHandleTransform(field, detail::MakeStatState{});
    return Algorithm::Reduce(states, detail::StatState<FieldType>{});
  }
}; // StatisticalMoments

} // worklet
} // vtkm
#endif // vtk_m_worklet_StatisticalMoments_h
