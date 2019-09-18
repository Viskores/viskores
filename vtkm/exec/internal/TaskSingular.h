//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_internal_TaskSingular_h
#define vtk_m_exec_internal_TaskSingular_h

#include <vtkm/internal/Invocation.h>

#include <vtkm/exec/TaskBase.h>

#include <vtkm/exec/arg/Fetch.h>

//Todo: rename this header to TaskSingularDetail.h
#include <vtkm/exec/internal/WorkletInvokeFunctorDetail.h>

namespace vtkm
{
namespace exec
{
namespace internal
{

// TaskSingular represents an execution pattern for a worklet
// that is best expressed in terms of single dimension iteration space. Inside
// this single dimension no order is preferred.
//
//
template <typename WorkletType, typename InvocationType>
class TaskSingular : public vtkm::exec::TaskBase
{
public:
  VTKM_CONT
  TaskSingular(const WorkletType& worklet,
               const InvocationType& invocation,
               vtkm::Id globalIndexOffset = 0)
    : Worklet(worklet)
    , Invocation(invocation)
    , GlobalIndexOffset(globalIndexOffset)
  {
  }

  VTKM_CONT
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T>
  VTKM_EXEC void operator()(T index) const
  {
    //Todo: rename this function to DoTaskSingular
    detail::DoWorkletInvokeFunctor(
      this->Worklet,
      this->Invocation,
      this->Worklet.GetThreadIndices(index,
                                     this->Invocation.OutputToInputMap,
                                     this->Invocation.VisitArray,
                                     this->Invocation.ThreadToOutputMap,
                                     this->Invocation.GetInputDomain(),
                                     GlobalIndexOffset));
  }

private:
  typename std::remove_const<WorkletType>::type Worklet;
  // This is held by by value so that when we transfer the invocation object
  // over to CUDA it gets properly copied to the device. While we want to
  // hold by reference to reduce the number of copies, it is not possible
  // currently.
  const InvocationType Invocation;
  const vtkm::Id GlobalIndexOffset;
};
}
}
} // vtkm::exec::internal

#endif //vtk_m_exec_internal_TaskSingular_h
