//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtkm_interop_cuda_internal_TransferToOpenGL_h
#define vtkm_interop_cuda_internal_TransferToOpenGL_h

#include <vtkm/cont/ErrorBadAllocation.h>
#include <vtkm/cont/ErrorExecution.h>

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/cuda/internal/MakeThrustIterator.h>

#include <vtkm/interop/internal/TransferToOpenGL.h>

// Disable warnings we check vtkm for but Thrust does not.
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/copy.h>
#include <thrust/device_ptr.h>
#include <thrust/system/cuda/execution_policy.h>
#include <vtkm/exec/cuda/internal/ExecutionPolicy.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm
{
namespace interop
{
namespace internal
{

/// \brief cuda backend and opengl interop resource management
///
/// \c TransferResource manages cuda resource binding for a given buffer
///
///
class CudaTransferResource : public vtkm::interop::internal::TransferResource
{
public:
  CudaTransferResource()
    : vtkm::interop::internal::TransferResource()
  {
    this->Registered = false;
  }

  ~CudaTransferResource()
  {
    //unregister the buffer
    if (this->Registered)
    {
      cudaGraphicsUnregisterResource(this->CudaResource);
    }
  }

  bool IsRegistered() const { return Registered; }

  void Register(GLuint* handle)
  {
    if (this->Registered)
    {
      //If you don't release the cuda resource before re-registering you
      //will leak memory on the OpenGL side.
      cudaGraphicsUnregisterResource(this->CudaResource);
    }

    this->Registered = true;
    cudaError_t cError =
      cudaGraphicsGLRegisterBuffer(&this->CudaResource, *handle, cudaGraphicsMapFlagsWriteDiscard);
    if (cError != cudaSuccess)
    {
      throw vtkm::cont::ErrorExecution("Could not register the OpenGL buffer handle to CUDA.");
    }
  }

  void Map()
  {
    //map the resource into cuda, so we can copy it
    cudaError_t cError = cudaGraphicsMapResources(1, &this->CudaResource);
    if (cError != cudaSuccess)
    {
      throw vtkm::cont::ErrorBadAllocation(
        "Could not allocate enough memory in CUDA for OpenGL interop.");
    }
  }

  template <typename ValueType>
  ValueType* GetMappedPoiner(vtkm::Int64 desiredSize)
  {
    //get the mapped pointer
    std::size_t cuda_size;
    ValueType* pointer = nullptr;
    cudaError_t cError =
      cudaGraphicsResourceGetMappedPointer((void**)&pointer, &cuda_size, this->CudaResource);

    if (cError != cudaSuccess)
    {
      throw vtkm::cont::ErrorExecution("Unable to get pointers to CUDA memory for OpenGL buffer.");
    }

    //assert that cuda_size is the same size as the buffer we created in OpenGL
    VTKM_ASSERT(cuda_size >= static_cast<std::size_t>(desiredSize));
    return pointer;
  }

  void UnMap() { cudaGraphicsUnmapResources(1, &this->CudaResource); }

private:
  bool Registered;
  cudaGraphicsResource_t CudaResource;
};

/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible.
///
template <typename ValueType>
class TransferToOpenGL<ValueType, vtkm::cont::DeviceAdapterTagCuda>
{
  using DeviceAdapterTag = vtkm::cont::DeviceAdapterTagCuda;

public:
  VTKM_CONT explicit TransferToOpenGL(BufferState& state)
    : State(state)
    , Resource(nullptr)
  {
    if (!this->State.HasType())
    {
      this->State.DeduceAndSetType(ValueType());
    }

    this->Resource =
      dynamic_cast<vtkm::interop::internal::CudaTransferResource*>(state.GetResource());
    if (!this->Resource)
    {
      vtkm::interop::internal::CudaTransferResource* cudaResource =
        new vtkm::interop::internal::CudaTransferResource();

      //reset the resource to be a cuda resource
      this->State.SetResource(cudaResource);
      this->Resource = cudaResource;
    }
  }

  template <typename StorageTag>
  VTKM_CONT void Transfer(vtkm::cont::ArrayHandle<ValueType, StorageTag>& handle) const
  {
    //make a buffer for the handle if the user has forgotten too
    if (!glIsBuffer(*this->State.GetHandle()))
    {
      glGenBuffers(1, this->State.GetHandle());
    }

    //bind the buffer to the given buffer type
    glBindBuffer(this->State.GetType(), *this->State.GetHandle());

    //Determine if we need to reallocate the buffer
    const vtkm::Int64 size =
      static_cast<vtkm::Int64>(sizeof(ValueType)) * handle.GetNumberOfValues();
    this->State.SetSize(size);
    const bool resize = this->State.ShouldRealloc(size);
    if (resize)
    {
      //Allocate the memory and set it as GL_DYNAMIC_DRAW draw
      glBufferData(this->State.GetType(), static_cast<GLsizeiptr>(size), 0, GL_DYNAMIC_DRAW);

      this->State.SetCapacity(size);
    }

    if (!this->Resource->IsRegistered() || resize)
    {
      //register the buffer as being used by cuda. This needs to be done everytime
      //we change the size of the buffer. That is why we only change the buffer
      //size as infrequently as possible
      this->Resource->Register(this->State.GetHandle());
    }

    this->Resource->Map();

    ValueType* beginPointer = this->Resource->GetMappedPoiner<ValueType>(size);

    //get the device pointers
    auto portal = handle.PrepareForInput(DeviceAdapterTag());

    //Copy the data into memory that opengl owns, since we can't
    //give memory from cuda to opengl

    //Perhaps a direct call to thrust copy should be wrapped in a vtkm
    //compatble function
    ::thrust::copy(ThrustCudaPolicyPerThread,
                   vtkm::cont::cuda::internal::IteratorBegin(portal),
                   vtkm::cont::cuda::internal::IteratorEnd(portal),
                   thrust::cuda::pointer<ValueType>(beginPointer));
    cudaStreamSynchronize(cudaStreamPerThread);

    //unmap the resource
    this->Resource->UnMap();
  }

private:
  vtkm::interop::BufferState& State;
  vtkm::interop::internal::CudaTransferResource* Resource;
};
}
}
} //namespace vtkm::interop::cuda::internal

#endif
