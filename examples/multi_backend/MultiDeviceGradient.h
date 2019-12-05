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

#include "TaskQueue.h"

#include <thread>

using RuntimeTaskQueue = TaskQueue<std::function<void()>>;

/// \brief Construct a MultiDeviceGradient for a given partitioned dataset
///
/// The Policy used with MultiDeviceGradient must include the TBB and CUDA
/// backends.
class MultiDeviceGradient : public vtkm::filter::FilterField<MultiDeviceGradient>
{
public:
  using SupportedTypes = vtkm::List<vtkm::Float32, vtkm::Float64, vtkm::Vec3f_32, vtkm::Vec3f_64>;

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
  VTKM_CONT vtkm::cont::PartitionedDataSet PrepareForExecution(
    const vtkm::cont::PartitionedDataSet&,
    const vtkm::filter::PolicyBase<DerivedPolicy>&);

private:
  bool ComputePointGradient;
  RuntimeTaskQueue Queue;
  std::vector<std::thread> Workers;
};

#ifndef vtk_m_examples_multibackend_MultiDeviceGradient_cxx
extern template vtkm::cont::PartitionedDataSet MultiDeviceGradient::PrepareForExecution<
  vtkm::filter::PolicyDefault>(const vtkm::cont::PartitionedDataSet&,
                               const vtkm::filter::PolicyBase<vtkm::filter::PolicyDefault>&);
#endif

#endif
