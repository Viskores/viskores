//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_interop_internal_TransferToOpenGL_h
#define vtk_m_interop_internal_TransferToOpenGL_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/StorageBasic.h>

#include <vtkm/cont/DeviceAdapterAlgorithm.h>
#include <vtkm/cont/serial/DeviceAdapterSerial.h>
#include <vtkm/cont/tbb/DeviceAdapterTBB.h>

#include <vtkm/interop/BufferState.h>
#include <vtkm/interop/internal/OpenGLHeaders.h>

namespace vtkm
{
namespace interop
{
namespace internal
{

/// \brief smp backend and opengl interop resource management
///
/// \c TransferResource manages any extra memory allocation that is required
///    When binding an implicit array handle to opengl
///
///
class SMPTransferResource : public vtkm::interop::internal::TransferResource
{
public:
  template <typename T>
  SMPTransferResource(T, vtkm::Id numberOfValues)
    : vtkm::interop::internal::TransferResource()
    , Size(0)
    , TempStorage()
  {
    this->resize<T>(numberOfValues);
  }

  ~SMPTransferResource() {}

  template <typename T>
  void resize(vtkm::Id numberOfValues)
  {
    if (this->Size != numberOfValues)
    {
      this->Size = numberOfValues;
      T* storage = new T[static_cast<std::size_t>(this->Size)];
      this->TempStorage.reset(reinterpret_cast<vtkm::UInt8*>(storage));
    }
  }

  template <typename T>
  vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic> handle(vtkm::Id size) const
  {
    VTKM_ASSERT(this->Size > 0);
    VTKM_ASSERT(this->Size >= size);

    T* storage = reinterpret_cast<T*>(this->TempStorage.get());
    //construct a handle that is a view onto the memory
    return vtkm::cont::make_ArrayHandle(storage, size);
  }

  template <typename T>
  T* as() const
  {
    VTKM_ASSERT(this->Size > 0);
    T* storage = reinterpret_cast<T*>(this->TempStorage.get());
    return storage;
  }

  vtkm::Id Size;
  std::unique_ptr<vtkm::UInt8[]> TempStorage;
};

namespace detail
{

template <class ValueType, class StorageTag, class DeviceAdapterTag>
VTKM_CONT void CopyFromHandle(const vtkm::cont::ArrayHandle<ValueType, StorageTag>& handle,
                              vtkm::interop::BufferState& state,
                              DeviceAdapterTag)
{
  //Generic implementation that will work no matter what. We copy the data
  //in the given handle to storage held by the buffer state.
  const vtkm::Id numberOfValues = handle.GetNumberOfValues();
  const GLsizeiptr size =
    static_cast<GLsizeiptr>(sizeof(ValueType)) * static_cast<GLsizeiptr>(numberOfValues);

  //grab the temporary storage from the buffer resource
  vtkm::interop::internal::SMPTransferResource* resource =
    dynamic_cast<vtkm::interop::internal::SMPTransferResource*>(state.GetResource());

  //Determine if we need to reallocate the buffer
  state.SetSize(size);
  const bool resize = state.ShouldRealloc(size);

  if (resize)
  {
    //Allocate the memory and set it as GL_DYNAMIC_DRAW draw
    glBufferData(state.GetType(), size, 0, GL_DYNAMIC_DRAW);
    state.SetCapacity(size);

    //If we have an existing resource reallocate it to fit our new size
    if (resource)
    {
      resource->resize<ValueType>(numberOfValues);
    }
  }

  //if we don't have a valid resource make a new one. We do this after the
  //resize check so we don't double allocate when resource == nullptr and
  //resize == true
  if (!resource)
  {
    resource = new vtkm::interop::internal::SMPTransferResource(ValueType(), numberOfValues);
    state.SetResource(resource);
  }

  using Algorithm = vtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;
  auto resourceHandle = resource->handle<ValueType>(numberOfValues);
  Algorithm::Copy(handle, resourceHandle);

  //copy into opengl buffer
  glBufferSubData(state.GetType(), 0, size, resource->as<ValueType>());
}

template <class ValueType, class DeviceAdapterTag>
VTKM_CONT void CopyFromHandle(
  const vtkm::cont::ArrayHandle<ValueType, vtkm::cont::StorageTagBasic>& handle,
  vtkm::interop::BufferState& state,
  DeviceAdapterTag)
{
  //Specialization given that we are use an C allocated array storage tag
  //that allows us to directly hook into the data. We pull the data
  //back to the control environment using PerpareForInput and give an iterator
  //from the portal to OpenGL to upload to the rendering system
  //This also works because we know that this class isn't used for cuda interop,
  //instead we are specialized
  const GLsizeiptr size = static_cast<GLsizeiptr>(sizeof(ValueType)) *
    static_cast<GLsizeiptr>(handle.GetNumberOfValues());

  //Determine if we need to reallocate the buffer
  state.SetSize(size);
  const bool resize = state.ShouldRealloc(size);
  if (resize)
  {
    //Allocate the memory and set it as GL_DYNAMIC_DRAW draw
    glBufferData(state.GetType(), size, 0, GL_DYNAMIC_DRAW);

    state.SetCapacity(size);
  }

  //Allocate the memory and set it as static draw and copy into opengl
  const ValueType* memory =
    &(*vtkm::cont::ArrayPortalToIteratorBegin(handle.PrepareForInput(DeviceAdapterTag())));
  glBufferSubData(state.GetType(), 0, size, memory);
}

} //namespace detail

/// \brief Manages transferring an ArrayHandle to opengl .
///
/// \c TransferToOpenGL manages to transfer the contents of an ArrayHandle
/// to OpenGL as efficiently as possible.
///
template <typename ValueType, class DeviceAdapterTag>
class TransferToOpenGL
{
public:
  VTKM_CONT explicit TransferToOpenGL(BufferState& state)
    : State(state)
  {
    if (!this->State.HasType())
    {
      this->State.DeduceAndSetType(ValueType());
    }
  }

  template <typename StorageTag>
  VTKM_CONT void Transfer(const vtkm::cont::ArrayHandle<ValueType, StorageTag>& handle) const
  {
    //make a buffer for the handle if the user has forgotten too
    if (!glIsBuffer(*this->State.GetHandle()))
    {
      glGenBuffers(1, this->State.GetHandle());
    }

    //bind the buffer to the given buffer type
    glBindBuffer(this->State.GetType(), *this->State.GetHandle());

    //transfer the data.
    //the primary concern that we have at this point is data locality and
    //the type of storage. Our options include using DeviceAdapterAlgorithm::Copy
    //and provide a temporary location for the values to reside before we give it
    //to openGL this works for all storage types.
    //
    //Second option is to call PrepareForInput and get a PortalConst in the
    //execution environment.
    //if we are StorageTagBasic this would allow us the ability to grab
    //the raw memory value and copy those, which we know are valid and remove
    //a unneeded copy.
    //
    //The end result is that we have CopyFromHandle which does number two
    //from StorageTagBasic, and does the DeviceAdapterAlgorithm::Copy for everything
    //else
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
