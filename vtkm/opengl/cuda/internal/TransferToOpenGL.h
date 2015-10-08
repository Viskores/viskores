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
#ifndef vtkm_opengl_cuda_internal_TransferToOpenGL_h
#define vtkm_opengl_cuda_internal_TransferToOpenGL_h

#include <vtkm/cont/ErrorExecution.h>
#include <vtkm/cont/ErrorControlBadAllocation.h>

#include <vtkm/cont/cuda/internal/DeviceAdapterTagCuda.h>
#include <vtkm/cont/cuda/internal/MakeThrustIterator.h>

#include <vtkm/opengl/internal/TransferToOpenGL.h>

// Disable warnings we check vtkm for but Thrust does not.
VTKM_THIRDPARTY_PRE_INCLUDE
#include <thrust/copy.h>
#include <thrust/device_ptr.h>
#include <thrust/system/cuda/execution_policy.h>
VTKM_THIRDPARTY_POST_INCLUDE

namespace vtkm {
namespace opengl {
namespace internal {

/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible.
///
template<typename ValueType>
class TransferToOpenGL<ValueType, vtkm::cont::DeviceAdapterTagCuda>
{
  typedef vtkm::cont::DeviceAdapterTagCuda DeviceAdapterTag;
public:
  VTKM_CONT_EXPORT TransferToOpenGL():
  Type( vtkm::opengl::internal::BufferTypePicker( ValueType() ) )
  {}

  VTKM_CONT_EXPORT explicit TransferToOpenGL(GLenum type):
  Type(type)
  {}

  GLenum GetType() const { return this->Type; }

  template< typename StorageTag >
  VTKM_CONT_EXPORT
  void Transfer (
    vtkm::cont::ArrayHandle<ValueType, StorageTag> &handle,
    GLuint& openGLHandle ) const
  {
  //construct a cuda resource handle
  cudaGraphicsResource_t cudaResource;
  cudaError_t cError;

  //make a buffer for the handle if the user has forgotten too
  if(!glIsBuffer(openGLHandle))
    {
    glGenBuffers(1,&openGLHandle);
    }

  //bind the buffer to the given buffer type
  glBindBuffer(this->Type, openGLHandle);

  //Allocate the memory and set it as GL_DYNAMIC_DRAW draw
  const vtkm::Id size = static_cast<vtkm::Id>(sizeof(ValueType))* handle.GetNumberOfValues();
  glBufferData(this->Type, size, 0, GL_DYNAMIC_DRAW);

  //register the buffer as being used by cuda
  cError = cudaGraphicsGLRegisterBuffer(&cudaResource,
                                        openGLHandle,
                                        cudaGraphicsMapFlagsWriteDiscard);
  if(cError != cudaSuccess)
    {
    throw vtkm::cont::ErrorExecution(
            "Could not register the OpenGL buffer handle to CUDA.");
    }

  //map the resource into cuda, so we can copy it
  cError =cudaGraphicsMapResources(1,&cudaResource);
  if(cError != cudaSuccess)
    {
    throw vtkm::cont::ErrorControlBadAllocation(
            "Could not allocate enough memory in CUDA for OpenGL interop.");
    }

  //get the mapped pointer
  std::size_t cuda_size;
  ValueType* beginPointer=NULL;
  cError = cudaGraphicsResourceGetMappedPointer((void **)&beginPointer,
                                       &cuda_size,
                                       cudaResource);

  if(cError != cudaSuccess)
    {
    throw vtkm::cont::ErrorExecution(
            "Unable to get pointers to CUDA memory for OpenGL buffer.");
    }

  //assert that cuda_size is the same size as the buffer we created in OpenGL
  VTKM_ASSERT_CONT(cuda_size == size);

  //get the device pointers
  typedef vtkm::cont::ArrayHandle<ValueType, StorageTag> HandleType;
  typedef typename HandleType::template
                ExecutionTypes<DeviceAdapterTag>::PortalConst PortalType;
  PortalType portal = handle.PrepareForInput(DeviceAdapterTag());

  //Copy the data into memory that opengl owns, since we can't
  //give memory from cuda to opengl

  //Perhaps a direct call to thrust copy should be wrapped in a vtkm
  //compatble function
  ::thrust::copy(
                thrust::cuda::par,
                vtkm::cont::cuda::internal::IteratorBegin(portal),
                vtkm::cont::cuda::internal::IteratorEnd(portal),
                thrust::cuda::pointer<ValueType>(beginPointer));

  //unmap the resource
  cudaGraphicsUnmapResources(1, &cudaResource);

  //unregister the buffer
  cudaGraphicsUnregisterResource(cudaResource);

  }
private:
  GLenum Type;
};



}
}
} //namespace vtkm::opengl::cuda::internal


#endif

