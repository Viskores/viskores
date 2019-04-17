//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_examples_multibackend_MultiDeviceGradient_h
#define vtk_m_examples_multibackend_MultiDeviceGradient_h


#include <vtkm/filter/FilterField.h>
#include <vtkm/filter/FilterTraits.h>

#include "TaskQueue.h"

#include <thread>

using RuntimeTaskQueue = TaskQueue<std::function<void()>>;

/// \brief Construct a MultiDeviceGradient for a given multiblock dataset
///
/// The Policy used with MultiDeviceGradient must include the TBB and CUDA
/// backends.
class MultiDeviceGradient : public vtkm::filter::FilterField<MultiDeviceGradient>
{
public:
  //Construct a MultiDeviceGradient and worker pool
  VTKM_CONT
  MultiDeviceGradient();

  //Needed so that we can shut down the worker pool properly
  VTKM_CONT
  ~MultiDeviceGradient();

  /// When this flag is on (default is off), the gradient filter will provide a
  /// point based gradients, which are significantly more costly since for each
  /// point we need to compute the gradient of each cell that uses it.
  void SetComputePointGradient(bool enable) { ComputePointGradient = enable; }
  bool GetComputePointGradient() const { return ComputePointGradient; }

  /// Will submit each block to a work queue that the threads will
  /// pull work from
  template <typename DerivedPolicy>
  VTKM_CONT vtkm::cont::MultiBlock PrepareForExecution(
    const vtkm::cont::MultiBlock&,
    const vtkm::filter::PolicyBase<DerivedPolicy>&);

private:
  bool ComputePointGradient;
  RuntimeTaskQueue Queue;
  std::vector<std::thread> Workers;
};

namespace vtkm
{
namespace filter
{
template <>
class FilterTraits<MultiDeviceGradient>
{
public:
  struct TypeListTagGradientInputs : vtkm::ListTagBase<vtkm::Float32,
                                                       vtkm::Float64,
                                                       vtkm::Vec<vtkm::Float32, 3>,
                                                       vtkm::Vec<vtkm::Float64, 3>>
  {
  };

  using InputFieldTypeList = TypeListTagGradientInputs;
};
}
} // namespace vtkm::filter


#ifndef vtk_m_examples_multibackend_MultiDeviceGradient_cxx
extern template vtkm::cont::MultiBlock MultiDeviceGradient::PrepareForExecution<
  vtkm::filter::PolicyDefault>(const vtkm::cont::MultiBlock&,
                               const vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>&);
#endif

#endif
