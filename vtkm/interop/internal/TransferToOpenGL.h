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
#ifndef vtk_m_interop_internal_TransferToOpenGL_h
#define vtk_m_interop_internal_TransferToOpenGL_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageBasic.h>
#include <vtkm/cont/DeviceAdapterAlgorithm.h>

#include <vtkm/interop/internal/OpenGLHeaders.h>
#include <vtkm/interop/BufferState.h>


namespace vtkm {
namespace interop {
namespace internal {

namespace detail
{

template<class ValueType, class StorageTag, class DeviceAdapterTag>
VTKM_CONT_EXPORT
void CopyFromHandle(
  vtkm::cont::ArrayHandle<ValueType, StorageTag>& handle,
  vtkm::interop::BufferState& state,
  DeviceAdapterTag)
{
  //Generic implementation that will work no matter what. We copy the data
  //in the given handle to a temporary handle using the basic storage tag.
  //We then ensure the data is available in the control environment by
  //synchronizing the control array. Last, we steal the array and pass the
  //iterator to the rendering system
  const vtkm::Id numberOfValues = handle.GetNumberOfValues();
  const GLsizeiptr size =
          static_cast<GLsizeiptr>(sizeof(ValueType)) *
          static_cast<GLsizeiptr>(numberOfValues);

  //Copy the data from its specialized Storage container to a basic heap alloc
  ValueType* temporaryStorage = new ValueType[numberOfValues];

#ifdef VTKM_MSVC
  #pragma warning(disable:4244)
  #pragma warning(disable:4996)
#endif
  handle.CopyInto(temporaryStorage, DeviceAdapterTag());
#ifdef VTKM_MSVC
  #pragma warning(default:4996)
  #pragma warning(default:4244)
#endif

  //Determine if we need to reallocate the buffer
  state.SetSize(size);
  const bool resize = state.ShouldRealloc(size);
  if( resize )
  {
    //Allocate the memory and set it as GL_DYNAMIC_DRAW draw
    glBufferData(state.GetType(), size, 0, GL_DYNAMIC_DRAW);

    state.SetCapacity(size);
  }

  //copy into opengl buffer
  glBufferSubData(state.GetType(),0,size,temporaryStorage);

  delete[] temporaryStorage;
}

template<class ValueType, class DeviceAdapterTag>
VTKM_CONT_EXPORT
void CopyFromHandle(
  vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagBasic>& handle,
  vtkm::interop::BufferState& state,
  DeviceAdapterTag)
{
  //Specialization given that we are use an C allocated array storage tag
  //that allows us to directly hook into the data. We pull the data
  //back to the control environment using PerpareForInput and give an iterator
  //from the portal to OpenGL to upload to the rendering system
  //This also works because we know that this class isn't used for cuda interop,
  //instead we are specialized
  const GLsizeiptr size =
      static_cast<GLsizeiptr>(sizeof(ValueType)) *
          static_cast<GLsizeiptr>(handle.GetNumberOfValues());

  //Determine if we need to reallocate the buffer
  state.SetSize(size);
  const bool resize = state.ShouldRealloc(size);
  if( resize )
  {
    //Allocate the memory and set it as GL_DYNAMIC_DRAW draw
    glBufferData(state.GetType(), size, 0, GL_DYNAMIC_DRAW);

    state.SetCapacity(size);
  }

  //Allocate the memory and set it as static draw and copy into opengl
  const ValueType* memory = &(*vtkm::cont::ArrayPortalToIteratorBegin(
                                handle.PrepareForInput(DeviceAdapterTag())));
  glBufferSubData(state.GetType(),0,size,memory);

}

} //namespace detail

/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible.
///
template<typename ValueType, class DeviceAdapterTag>
class TransferToOpenGL
{
public:
  VTKM_CONT_EXPORT explicit TransferToOpenGL(BufferState& state):
    State(state)
  {
    if( !this->State.HasType() )
    {
      this->State.DeduceAndSetType( ValueType() );
    }
  }

  template< typename StorageTag >
  VTKM_CONT_EXPORT
  void Transfer (
    vtkm::cont::ArrayHandle<ValueType, StorageTag>& handle) const
  {
  //make a buffer for the handle if the user has forgotten too
  if(!glIsBuffer(*this->State.GetHandle()))
  {
    glGenBuffers(1, this->State.GetHandle());
  }

  //bind the buffer to the given buffer type
  glBindBuffer(this->State.GetType(), *this->State.GetHandle());

  //transfer the data.
  //the primary concern that we have at this point is data locality and
  //the type of storage. Our options include using CopyInto and provide a
  //temporary location for the values to reside before we give it to openGL
  //this works for all storage types.
  //
  //Second option is to call PrepareForInput and get a PortalConst in the
  //exection environment.
  //if we are StorageTagBasic this would allow us the ability to grab
  //the raw memory value and copy those, which we know are valid and remove
  //a unneeded copy.
  //
  //The end result is that we have CopyFromHandle which does number two
  //from StorageTagBasic, and does the CopyInto for everything else
  detail::CopyFromHandle(handle, this->State, DeviceAdapterTag());
  }
private:
  vtkm::interop::BufferState& State;
};

}
}
} //namespace vtkm::interop::internal

//-----------------------------------------------------------------------------
// These includes are intentionally placed here after the declaration of the
// TransferToOpenGL class, so that people get the correct device adapter
/// specializations if they exist.
#if VTKM_DEVICE_ADAPTER == VTKM_DEVICE_ADAPTER_CUDA
#include <vtkm/interop/cuda/internal/TransferToOpenGL.h>
#endif


#endif //vtk_m_interop_internal_TransferToOpenGL_h
