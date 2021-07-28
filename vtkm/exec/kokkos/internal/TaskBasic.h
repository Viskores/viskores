//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_kokkos_internal_TaskBasic_h
#define vtk_m_exec_kokkos_internal_TaskBasic_h

#include <vtkm/exec/TaskBase.h>

//Todo: rename this header to TaskInvokeWorkletDetail.h
#include <vtkm/exec/internal/WorkletInvokeFunctorDetail.h>

namespace vtkm
{
namespace exec
{
namespace kokkos
{
namespace internal
{

template <typename WType, typename IType>
class TaskBasic1D : public vtkm::exec::TaskBase
{
public:
  TaskBasic1D(const WType& worklet, const IType& invocation)
    : Worklet(worklet)
    , Invocation(invocation)
  {
  }

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VTKM_EXEC
  void operator()(vtkm::Id index) const
  {
    vtkm::exec::internal::detail::DoWorkletInvokeFunctor(
      this->Worklet,
      this->Invocation,
      this->Worklet.GetThreadIndices(index,
                                     this->Invocation.OutputToInputMap,
                                     this->Invocation.VisitArray,
                                     this->Invocation.ThreadToOutputMap,
                                     this->Invocation.GetInputDomain()));
  }

private:
  typename std::remove_const<WType>::type Worklet;
  IType Invocation;
};

template <typename WType>
class TaskBasic1D<WType, vtkm::internal::NullType> : public vtkm::exec::TaskBase
{
public:
  explicit TaskBasic1D(const WType& worklet)
    : Worklet(worklet)
  {
  }

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VTKM_EXEC
  void operator()(vtkm::Id index) const { this->Worklet(index); }

private:
  typename std::remove_const<WType>::type Worklet;
};

template <typename WType, typename IType>
class TaskBasic3D : public vtkm::exec::TaskBase
{
public:
  TaskBasic3D(const WType& worklet, const IType& invocation)
    : Worklet(worklet)
    , Invocation(invocation)
  {
  }

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VTKM_EXEC
  void operator()(vtkm::Id3 idx, vtkm::Id flatIdx) const
  {
    vtkm::exec::internal::detail::DoWorkletInvokeFunctor(
      this->Worklet,
      this->Invocation,
      this->Worklet.GetThreadIndices(flatIdx,
                                     idx,
                                     this->Invocation.OutputToInputMap,
                                     this->Invocation.VisitArray,
                                     this->Invocation.ThreadToOutputMap,
                                     this->Invocation.GetInputDomain()));
  }

private:
  typename std::remove_const<WType>::type Worklet;
  IType Invocation;
};

template <typename WType>
class TaskBasic3D<WType, vtkm::internal::NullType> : public vtkm::exec::TaskBase
{
public:
  explicit TaskBasic3D(const WType& worklet)
    : Worklet(worklet)
  {
  }

  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    this->Worklet.SetErrorMessageBuffer(buffer);
  }

  VTKM_EXEC
  void operator()(vtkm::Id3 idx, vtkm::Id) const { this->Worklet(idx); }

private:
  typename std::remove_const<WType>::type Worklet;
};
}
}
}
} // vtkm::exec::kokkos::internal

#endif //vtk_m_exec_kokkos_internal_TaskBasic_h
