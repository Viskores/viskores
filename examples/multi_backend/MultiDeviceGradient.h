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

#include <vtkm/filter/Filter.h>

#include "TaskQueue.h"

#include <thread>

using RuntimeTaskQueue = TaskQueue<std::function<void()>>;

/// \brief Construct a MultiDeviceGradient for a given partitioned dataset
///
/// The Policy used with MultiDeviceGradient must include the TBB and CUDA
/// backends.
class MultiDeviceGradient : public vtkm::filter::Filter
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
  VTKM_CONT vtkm::cont::PartitionedDataSet DoExecutePartitions(
    const vtkm::cont::PartitionedDataSet& inData) override;

  // All filters must override this method. Our implementation will just wrap this in
  // a partitioned data set.
  VTKM_CONT vtkm::cont::DataSet DoExecute(const vtkm::cont::DataSet& inData) override;

private:
  bool ComputePointGradient;
  RuntimeTaskQueue Queue;
  std::vector<std::thread> Workers;
};

#endif
