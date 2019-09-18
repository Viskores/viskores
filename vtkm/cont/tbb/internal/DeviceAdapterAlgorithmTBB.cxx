//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/tbb/internal/DeviceAdapterAlgorithmTBB.h>

namespace vtkm
{
namespace cont
{

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB>::ScheduleTask(
  vtkm::exec::tbb::internal::TaskTiling1D& functor,
  vtkm::Id size)
{
  const vtkm::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  ::tbb::blocked_range<vtkm::Id> range(0, size, tbb::TBB_GRAIN_SIZE);

  ::tbb::parallel_for(
    range, [&](const ::tbb::blocked_range<vtkm::Id>& r) { functor(r.begin(), r.end()); });

  if (errorMessage.IsErrorRaised())
  {
    throw vtkm::cont::ErrorExecution(errorString);
  }
}

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagTBB>::ScheduleTask(
  vtkm::exec::tbb::internal::TaskTiling3D& functor,
  vtkm::Id3 size)
{
  static constexpr vtkm::UInt32 TBB_GRAIN_SIZE_3D[3] = { 1, 4, 256 };
  const vtkm::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  //memory is generally setup in a way that iterating the first range
  //in the tightest loop has the best cache coherence.
  ::tbb::blocked_range3d<vtkm::Id> range(0,
                                         size[2],
                                         TBB_GRAIN_SIZE_3D[0],
                                         0,
                                         size[1],
                                         TBB_GRAIN_SIZE_3D[1],
                                         0,
                                         size[0],
                                         TBB_GRAIN_SIZE_3D[2]);
  ::tbb::parallel_for(range, [&](const ::tbb::blocked_range3d<vtkm::Id>& r) {
    for (vtkm::Id k = r.pages().begin(); k != r.pages().end(); ++k)
    {
      for (vtkm::Id j = r.rows().begin(); j != r.rows().end(); ++j)
      {
        const vtkm::Id start = r.cols().begin();
        const vtkm::Id end = r.cols().end();
        functor(start, end, j, k);
      }
    }
  });

  if (errorMessage.IsErrorRaised())
  {
    throw vtkm::cont::ErrorExecution(errorString);
  }
}
}
}
