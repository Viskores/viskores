//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <vtkm/cont/serial/internal/DeviceAdapterAlgorithmSerial.h>

namespace vtkm
{
namespace cont
{

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagSerial>::ScheduleTask(
  vtkm::exec::serial::internal::TaskTiling1D& functor,
  vtkm::Id size)
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  const vtkm::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  const vtkm::Id iterations = size / 1024;
  vtkm::Id index = 0;
  for (vtkm::Id i = 0; i < iterations; ++i)
  {
    functor(index, index + 1024);
    index += 1024;
  }
  functor(index, size);

  if (errorMessage.IsErrorRaised())
  {
    throw vtkm::cont::ErrorExecution(errorString);
  }
}

void DeviceAdapterAlgorithm<vtkm::cont::DeviceAdapterTagSerial>::ScheduleTask(
  vtkm::exec::serial::internal::TaskTiling3D& functor,
  vtkm::Id3 size)
{
  VTKM_LOG_SCOPE_FUNCTION(vtkm::cont::LogLevel::Perf);

  const vtkm::Id MESSAGE_SIZE = 1024;
  char errorString[MESSAGE_SIZE];
  errorString[0] = '\0';
  vtkm::exec::internal::ErrorMessageBuffer errorMessage(errorString, MESSAGE_SIZE);
  functor.SetErrorMessageBuffer(errorMessage);

  for (vtkm::Id k = 0; k < size[2]; ++k)
  {
    for (vtkm::Id j = 0; j < size[1]; ++j)
    {
      functor(0, size[0], j, k);
    }
  }

  if (errorMessage.IsErrorRaised())
  {
    throw vtkm::cont::ErrorExecution(errorString);
  }
}
}
}
