//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_internal_WorkletInvokeFunctor_h
#define vtk_m_exec_internal_WorkletInvokeFunctor_h

#include <vtkm/internal/Invocation.h>

#include <vtkm/exec/FunctorBase.h>

#include <vtkm/exec/arg/Fetch.h>

#include <vtkm/exec/internal/WorkletInvokeFunctorDetail.h>

namespace vtkm {
namespace exec {
namespace internal {

template<typename WorkletType, typename InvocationType>
class WorkletInvokeFunctor : public vtkm::exec::FunctorBase
{
public:
  VTKM_CONT_EXPORT
  WorkletInvokeFunctor(const WorkletType &worklet,
                       const InvocationType &invocation)
    : Worklet(worklet), Invocation(invocation) {  }

  VTKM_CONT_EXPORT
  void SetErrorMessageBuffer(
      const vtkm::exec::internal::ErrorMessageBuffer &buffer)
  {
    this->FunctorBase::SetErrorMessageBuffer(buffer);
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template<typename T>
  VTKM_EXEC_EXPORT
  void operator()(T index) const
  {
    detail::DoWorkletInvokeFunctor(this->Worklet,
                                   this->Invocation,
                                   this->Worklet.GetThreadIndices(
                                     index, this->Invocation));
  }

private:
  WorkletType Worklet;

  // This is held by by value so that when we transfer the invocation object
  // over to CUDA it gets properly copied to the device. While we want to
  // hold by reference to reduce the number of copies, it is not possible
  // currently.
  const InvocationType Invocation;
};

}
}
} // vtkm::exec::internal

#endif //vtk_m_exec_internal_WorkletInvokeFunctor_h
