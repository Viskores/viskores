//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_cuda_internal_TaskStrided_h
#define vtk_m_exec_cuda_internal_TaskStrided_h

#include <vtkm/exec/TaskBase.h>

#include <vtkm/cont/cuda/internal/CudaAllocator.h>

//Todo: rename this header to TaskInvokeWorkletDetail.h
#include <vtkm/exec/internal/WorkletInvokeFunctorDetail.h>

namespace vtkm
{
namespace exec
{
namespace cuda
{
namespace internal
{

template <typename WType>
void TaskStridedSetErrorBuffer(void* w, const vtkm::exec::internal::ErrorMessageBuffer& buffer)
{
  using WorkletType = typename std::remove_cv<WType>::type;
  WorkletType* const worklet = static_cast<WorkletType*>(w);
  worklet->SetErrorMessageBuffer(buffer);
}

class TaskStrided : public vtkm::exec::TaskBase
{
public:
  void SetErrorMessageBuffer(const vtkm::exec::internal::ErrorMessageBuffer& buffer)
  {
    (void)buffer;
    this->SetErrorBufferFunction(this->WPtr, buffer);
  }

protected:
  void* WPtr = nullptr;

  using SetErrorBufferSignature = void (*)(void*, const vtkm::exec::internal::ErrorMessageBuffer&);
  SetErrorBufferSignature SetErrorBufferFunction = nullptr;
};

template <typename WType, typename IType>
class TaskStrided1D : public TaskStrided
{
public:
  TaskStrided1D(const WType& worklet, const IType& invocation, vtkm::Id globalIndexOffset = 0)
    : TaskStrided()
    , Worklet(worklet)
    , Invocation(invocation)
    , GlobalIndexOffset(globalIndexOffset)
  {
    this->SetErrorBufferFunction = &TaskStridedSetErrorBuffer<WType>;
    //Bind the Worklet to void*
    this->WPtr = (void*)&this->Worklet;
  }

  VTKM_EXEC
  void operator()(vtkm::Id start, vtkm::Id end, vtkm::Id inc) const
  {
    for (vtkm::Id index = start; index < end; index += inc)
    {
      //Todo: rename this function to DoTaskInvokeWorklet
      vtkm::exec::internal::detail::DoWorkletInvokeFunctor(
        this->Worklet,
        this->Invocation,
        this->Worklet.GetThreadIndices(index,
                                       this->Invocation.OutputToInputMap,
                                       this->Invocation.VisitArray,
                                       this->Invocation.ThreadToOutputMap,
                                       this->Invocation.GetInputDomain(),
                                       this->GlobalIndexOffset));
    }
  }

private:
  typename std::remove_const<WType>::type Worklet;
  // This is held by by value so that when we transfer the invocation object
  // over to CUDA it gets properly copied to the device. While we want to
  // hold by reference to reduce the number of copies, it is not possible
  // currently.
  const IType Invocation;
  const vtkm::Id GlobalIndexOffset;
};

template <typename WType>
class TaskStrided1D<WType, vtkm::internal::NullType> : public TaskStrided
{
public:
  TaskStrided1D(WType& worklet)
    : TaskStrided()
    , Worklet(worklet)
  {
    this->SetErrorBufferFunction = &TaskStridedSetErrorBuffer<WType>;
    //Bind the Worklet to void*
    this->WPtr = (void*)&this->Worklet;
  }

  VTKM_EXEC
  void operator()(vtkm::Id start, vtkm::Id end, vtkm::Id inc) const
  {
    for (vtkm::Id index = start; index < end; index += inc)
    {
      this->Worklet(index);
    }
  }

private:
  typename std::remove_const<WType>::type Worklet;
};

template <typename WType, typename IType>
class TaskStrided3D : public TaskStrided
{
public:
  TaskStrided3D(const WType& worklet, const IType& invocation, vtkm::Id globalIndexOffset = 0)
    : TaskStrided()
    , Worklet(worklet)
    , Invocation(invocation)
    , GlobalIndexOffset(globalIndexOffset)
  {
    this->SetErrorBufferFunction = &TaskStridedSetErrorBuffer<WType>;
    //Bind the Worklet to void*
    this->WPtr = (void*)&this->Worklet;
  }

  VTKM_EXEC
  void operator()(vtkm::Id start, vtkm::Id end, vtkm::Id inc, vtkm::Id j, vtkm::Id k) const
  {
    vtkm::Id3 index(start, j, k);
    for (vtkm::Id i = start; i < end; i += inc)
    {
      index[0] = i;
      //Todo: rename this function to DoTaskInvokeWorklet
      vtkm::exec::internal::detail::DoWorkletInvokeFunctor(
        this->Worklet,
        this->Invocation,
        this->Worklet.GetThreadIndices(index,
                                       this->Invocation.OutputToInputMap,
                                       this->Invocation.VisitArray,
                                       this->Invocation.ThreadToOutputMap,
                                       this->Invocation.GetInputDomain(),
                                       this->GlobalIndexOffset));
    }
  }

private:
  typename std::remove_const<WType>::type Worklet;
  // This is held by by value so that when we transfer the invocation object
  // over to CUDA it gets properly copied to the device. While we want to
  // hold by reference to reduce the number of copies, it is not possible
  // currently.
  const IType Invocation;
  const vtkm::Id GlobalIndexOffset;
};

template <typename WType>
class TaskStrided3D<WType, vtkm::internal::NullType> : public TaskStrided
{
public:
  TaskStrided3D(WType& worklet)
    : TaskStrided()
    , Worklet(worklet)
  {
    this->SetErrorBufferFunction = &TaskStridedSetErrorBuffer<WType>;
    //Bind the Worklet to void*
    this->WPtr = (void*)&this->Worklet;
  }

  VTKM_EXEC
  void operator()(vtkm::Id start, vtkm::Id end, vtkm::Id inc, vtkm::Id j, vtkm::Id k) const
  {
    vtkm::Id3 index(start, j, k);
    for (vtkm::Id i = start; i < end; i += inc)
    {
      index[0] = i;
      this->Worklet(index);
    }
  }

private:
  typename std::remove_const<WType>::type Worklet;
};
}
}
}
} // vtkm::exec::cuda::internal

#endif //vtk_m_exec_cuda_internal_TaskStrided_h
