//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_DescriptiveStatistics_h
#define vtk_m_worklet_DescriptiveStatistics_h

#include <vtkm/cont/Algorithm.h>
#include <vtkm/cont/ArrayCopy.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/ArrayHandleZip.h>

namespace vtkm
{
namespace worklet
{
class DescriptiveStatistics
{
public:
  template <typename T>
  struct StatState
  {
    VTKM_EXEC_CONT
    StatState()
      : n_(0)
      , min_(std::numeric_limits<T>::max())
      , max_(std::numeric_limits<T>::lowest())
      , sum_(0)
      , mean_(0)
      , M2_(0)
      , M3_(0)
      , M4_(0)
    {
    }

    VTKM_EXEC_CONT
    StatState(T value)
      : n_(1)
      , min_(value)
      , max_(value)
      , sum_(value)
      , mean_(value)
      , M2_(0)
      , M3_(0)
      , M4_(0)
    {
    }

    VTKM_EXEC_CONT
    StatState operator+(const StatState<T>& y) const
    {
      const StatState<T>& x = *this;
      StatState result;

      result.n_ = x.n_ + y.n_;

      result.min_ = vtkm::Min(x.min_, y.min_);
      result.max_ = vtkm::Max(x.max_, y.max_);

      // TODO: consider implementing compensated sum
      // https://en.wikipedia.org/wiki/Kahan_summation_algorithm
      result.sum_ = x.sum_ + y.sum_;

      // It is tempting to try to deviate from the literature and calculate
      // mean in each "reduction" from sum and n. This saves one multiplication.
      // However, RESIST THE TEMPTATION!!! This takes us back to the naive
      // algorithm (mean = sum of a bunch of numbers / N) that actually
      // accumulates more error and causes problem when calculating M2
      // (and thus variance).
      // TODO: Verify that FieldStatistics exhibits the same problem since
      // it is using a "parallel" version of the naive algorithm as well.
      // TODO: or better, just deprecate FieldStatistics.
      T delta = y.mean_ - x.mean_;
      result.mean_ = x.mean_ + delta * y.n_ / result.n_;

      T delta2 = delta * delta;
      result.M2_ = x.M2_ + y.M2_ + delta2 * x.n_ * y.n_ / result.n_;

      T delta3 = delta * delta2;
      T n2 = result.n_ * result.n_;
      result.M3_ = x.M3_ + y.M3_;
      result.M3_ += delta3 * x.n_ * y.n_ * (x.n_ - y.n_) / n2;
      result.M3_ += T(3.0) * delta * (x.n_ * y.M2_ - y.n_ * x.M2_) / result.n_;

      T delta4 = delta2 * delta2;
      T n3 = result.n_ * n2;
      result.M4_ = x.M4_ + y.M4_;
      result.M4_ += delta4 * x.n_ * y.n_ * (x.n_ * x.n_ - x.n_ * y.n_ + y.n_ * y.n_) / n3;
      result.M4_ += T(6.0) * delta2 * (x.n_ * x.n_ * y.M2_ + y.n_ * y.n_ * x.M2_) / n2;
      result.M4_ += T(4.0) * delta * (x.n_ * y.M3_ - y.n_ * x.M3_) / result.n_;

      return result;
    }

    VTKM_EXEC_CONT
    T N() const { return this->n_; }

    VTKM_EXEC_CONT
    T Min() const { return this->min_; }

    VTKM_EXEC_CONT
    T Max() const { return this->max_; }

    VTKM_EXEC_CONT
    T Sum() const { return this->sum_; }

    VTKM_EXEC_CONT
    T Mean() const { return this->mean_; }

    VTKM_EXEC_CONT
    T SampleStddev() const { return vtkm::Sqrt(this->SampleVariance()); }

    VTKM_EXEC_CONT
    T PopulationStddev() const { return vtkm::Sqrt(this->PopulationVariance()); }

    VTKM_EXEC_CONT
    T SampleVariance() const
    {
      VTKM_ASSERT(n_ != 1);
      return this->M2_ / (this->n_ - 1);
    }

    VTKM_EXEC_CONT
    T PopulationVariance() const { return this->M2_ / this->n_; }

    VTKM_EXEC_CONT
    T Skewness() const
    {
      if (this->M2_ == 0)
        // Shamelessly swiped from Boost Math
        // The limit is technically undefined, but the interpretation here is clear:
        // A constant dataset has no skewness.
        return T(0);
      else
        return vtkm::Sqrt(this->n_) * this->M3_ / vtkm::Pow(this->M2_, T{ 1.5 });
    }

    VTKM_EXEC_CONT
    T Kurtosis() const
    {
      if (this->M2_ == 0)
        // Shamelessly swiped from Boost Math
        // The limit is technically undefined, but the interpretation here is clear:
        // A constant dataset has no kurtosis.
        return T(0);
      else
        return this->n_ * this->M4_ / (this->M2_ * this->M2_);
    }

  private:
    // GCC4.8 is not happy about initializing data members here.
    T n_;
    T min_;
    T max_;
    T sum_;
    T mean_;
    T M2_;
    T M3_;
    T M4_;
  }; // StatState

  struct MakeStatState
  {
    template <typename T>
    VTKM_EXEC_CONT vtkm::worklet::DescriptiveStatistics::StatState<T> operator()(T value) const
    {
      return vtkm::worklet::DescriptiveStatistics::StatState<T>{ value };
    }
  };

  /// \brief Calculate various summary statistics for the input ArrayHandle
  ///
  /// Reference:
  ///    [1] Wikipeida, parallel algorithm for calculating variance
  ///        http://en.wikipedia.org/wiki/Algorithms_for_calculating_variance#Parallel_algorithm
  ///    [2] Implementation of [1] in the Trust library
  ///        https://github.com/thrust/thrust/blob/master/examples/summary_statistics.cu
  ///    [3] Bennett, Janine, et al. "Numerically stable, single-pass, parallel statistics algorithms."
  ///        2009 IEEE International Conference on Cluster Computing and Workshops. IEEE, 2009.
  template <typename FieldType, typename Storage>
  VTKM_CONT static StatState<FieldType> Run(
    const vtkm::cont::ArrayHandle<FieldType, Storage>& field)
  {
    using Algorithm = vtkm::cont::Algorithm;

    // Essentially a TransformReduce. Do we have that convenience in VTKm?
    auto states = vtkm::cont::make_ArrayHandleTransform(field, MakeStatState{});
    return Algorithm::Reduce(states, StatState<FieldType>{});
  }

  template <typename KeyType, typename ValueType, typename KeyInStorage, typename ValueInStorage>
  VTKM_CONT static auto Run(const vtkm::cont::ArrayHandle<KeyType, KeyInStorage>& keys,
                            const vtkm::cont::ArrayHandle<ValueType, ValueInStorage>& values)
    -> vtkm::cont::ArrayHandleZip<vtkm::cont::ArrayHandle<KeyType>,
                                  vtkm::cont::ArrayHandle<StatState<ValueType>>>
  {
    using Algorithm = vtkm::cont::Algorithm;

    // Make a copy of the input arrays so we don't modify them
    vtkm::cont::ArrayHandle<KeyType> keys_copy;
    vtkm::cont::ArrayCopy(keys, keys_copy);

    vtkm::cont::ArrayHandle<ValueType> values_copy;
    vtkm::cont::ArrayCopy(values, values_copy);

    // Gather values of the same key by sorting them according to keys
    Algorithm::SortByKey(keys_copy, values_copy);

    auto states = vtkm::cont::make_ArrayHandleTransform(values_copy, MakeStatState{});
    vtkm::cont::ArrayHandle<KeyType> keys_out;

    vtkm::cont::ArrayHandle<StatState<ValueType>> results;
    Algorithm::ReduceByKey(keys_copy, states, keys_out, results, vtkm::Add{});

    return vtkm::cont::make_ArrayHandleZip(keys_out, results);
  }
}; // DescriptiveStatistics

} // worklet
} // vtkm
#endif // vtk_m_worklet_DescriptiveStatistics_h
