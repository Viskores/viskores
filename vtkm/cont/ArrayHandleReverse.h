//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#ifndef vtk_m_cont_ArrayHandleReverse_h
#define vtk_m_cont_ArrayHandleReverse_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ErrorBadType.h>
#include <vtkm/cont/ErrorBadValue.h>

namespace vtkm
{
namespace cont
{

namespace internal
{

template <typename PortalType>
class VTKM_ALWAYS_EXPORT ArrayPortalReverse
{
public:
  typedef typename PortalType::ValueType ValueType;

  VTKM_EXEC_CONT
  ArrayPortalReverse()
    : portal()
  {
  }

  VTKM_EXEC_CONT
  ArrayPortalReverse(const PortalType& p)
    : portal(p)
  {
  }

  template <typename OtherPortal>
  VTKM_EXEC_CONT ArrayPortalReverse(const ArrayPortalReverse<OtherPortal>& src)
    : portal(src.GetPortal())
  {
  }

  VTKM_EXEC_CONT
  vtkm::Id GetNumberOfValues() const { return this->portal.GetNumberOfValues(); }

  VTKM_EXEC_CONT
  ValueType Get(vtkm::Id index) const
  {
    return this->portal.Get(portal.GetNumberOfValues() - index - 1);
  }

  VTKM_EXEC_CONT
  void Set(vtkm::Id index, const ValueType& value) const
  {
    this->portal.Set(portal.GetNumberOfValues() - index - 1, value);
  }

private:
  PortalType portal;
};
}

template <typename ArrayHandleType>
class StorageTagReverse
{
};

namespace internal
{

template <typename ArrayHandleType>
class Storage<typename ArrayHandleType::ValueType, StorageTagReverse<ArrayHandleType>>
{
public:
  typedef typename ArrayHandleType::ValueType ValueType;
  typedef ArrayPortalReverse<typename ArrayHandleType::PortalControl> PortalType;
  typedef ArrayPortalReverse<typename ArrayHandleType::PortalConstControl> PortalConstType;

  VTKM_CONT
  Storage()
    : valid(false)
  {
  }

  VTKM_CONT
  Storage(const ArrayHandleType& a)
    : array(a)
    , valid(true){};

  VTKM_CONT
  PortalConstType GetPortalConst() const
  {
    VTKM_ASSERT(this->valid);
    return PortalConstType(this->array.GetPortalConstControl());
  }

  VTKM_CONT
  PortalType GetPortal()
  {
    VTKM_ASSERT(this->valid);
    return PortalType(this->array.GetPortalControl());
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const
  {
    VTKM_ASSERT(this->valid);
    return this->array.GetNumberOfValues();
  }

  VTKM_CONT
  void Allocate(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorInternal("ArrayHandleReverse should not be allocated explicitly. ");
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayHandleReverse cannot shrink.");
  }

  VTKM_CONT
  void ReleaseResources()
  {
    // This request is ignored since it is asking to release the resources
    // of the delegate array, which may be used elsewhere. Should the behavior
    // be different?
  }

  VTKM_CONT
  const ArrayHandleType& GetArray() const
  {
    VTKM_ASSERT(this->valid);
    return this->array;
  }

private:
  ArrayHandleType array;
  bool valid;
}; // class storage

template <typename ArrayHandleType, typename Device>
class ArrayTransfer<typename ArrayHandleType::ValueType, StorageTagReverse<ArrayHandleType>, Device>
{
public:
  typedef typename ArrayHandleType::ValueType ValueType;

private:
  typedef StorageTagReverse<ArrayHandleType> StorageTag;
  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

public:
  typedef typename StorageType::PortalType PortalControl;
  typedef typename StorageType::PortalConstType PortalConstControl;

  typedef ArrayPortalReverse<typename ArrayHandleType::template ExecutionTypes<Device>::Portal>
    PortalExecution;
  typedef ArrayPortalReverse<typename ArrayHandleType::template ExecutionTypes<Device>::PortalConst>
    PortalConstExecution;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : array(storage->GetArray())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->array.GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool vtkmNotUsed(updateData))
  {
    return PortalConstExecution(this->array.PrepareForInput(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool vtkmNotUsed(updateData))
  {
    return PortalExecution(this->array.PrepareForInPlace(Device()));
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return PortalExecution(this->array.PrepareForOutput(numberOfValues, Device()));
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType* vtkmNotUsed(storage)) const
  {
    // not need to implement
  }

  VTKM_CONT
  void Shrink(vtkm::Id vtkmNotUsed(numberOfValues))
  {
    throw vtkm::cont::ErrorBadType("ArrayHandleReverse cannot shrink.");
  }

  VTKM_CONT
  void ReleaseResources() { this->array.ReleaseResourcesExecution(); }

private:
  ArrayHandleType array;
};

} // namespace internal

/// \brief Reverse the order of an array, on demand.
///
/// ArrayHandleReverse is a specialization of ArrayHandle. Given an ArrayHandle,
/// it creates a new handle that returns the elements of the array in reverse
/// order (i.e. from end to beginning).
///
template <typename ArrayHandleType>
class ArrayHandleReverse : public vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                                          StorageTagReverse<ArrayHandleType>>

{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS(ArrayHandleReverse,
                             (ArrayHandleReverse<ArrayHandleType>),
                             (vtkm::cont::ArrayHandle<typename ArrayHandleType::ValueType,
                                                      StorageTagReverse<ArrayHandleType>>));

protected:
  typedef vtkm::cont::internal::Storage<ValueType, StorageTag> StorageType;

public:
  ArrayHandleReverse(const ArrayHandleType& handle)
    : Superclass(handle)
  {
  }
};

/// make_ArrayHandleReverse is convenience function to generate an
/// ArrayHandleReverse.
///
template <typename HandleType>
VTKM_CONT ArrayHandleReverse<HandleType> make_ArrayHandleReverse(const HandleType& handle)
{
  return ArrayHandleReverse<HandleType>(handle);
}
}
} // namespace vtkm::cont

#endif // vtk_m_cont_ArrayHandleReverse_h
