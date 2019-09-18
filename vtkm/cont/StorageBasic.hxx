//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_StorageBasic_hxx
#define vtk_m_cont_StorageBasic_hxx

#include <vtkm/cont/StorageBasic.h>

#include <limits>

namespace vtkm
{
namespace cont
{
namespace internal
{


template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>::Storage()
  : StorageBasicBase()
{
}

template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>::Storage(const Storage<T, vtkm::cont::StorageTagBasic>& src)
  : StorageBasicBase(src)
{
}

template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>::Storage(
  Storage<T, vtkm::cont::StorageTagBasic>&& src) noexcept : StorageBasicBase(std::move(src))
{
}

template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>::Storage(const T* array, vtkm::Id numberOfValues)
  : StorageBasicBase(const_cast<T*>(array), numberOfValues, sizeof(T))
{
}

template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>::Storage(const T* array,
                                                 vtkm::Id numberOfValues,
                                                 void (*deleteFunction)(void*))
  : StorageBasicBase(const_cast<T*>(array), numberOfValues, sizeof(T), deleteFunction)
{
}

template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>& Storage<T, vtkm::cont::StorageTagBasic>::Storage::
operator=(const Storage<T, vtkm::cont::StorageTagBasic>& src)
{
  return static_cast<Storage<T, vtkm::cont::StorageTagBasic>&>(StorageBasicBase::operator=(src));
}

template <typename T>
Storage<T, vtkm::cont::StorageTagBasic>& Storage<T, vtkm::cont::StorageTagBasic>::Storage::
operator=(Storage<T, vtkm::cont::StorageTagBasic>&& src)
{
  return static_cast<Storage<T, vtkm::cont::StorageTagBasic>&>(
    StorageBasicBase::operator=(std::move(src)));
}


template <typename T>
void Storage<T, vtkm::cont::StorageTagBasic>::Allocate(vtkm::Id numberOfValues)
{
  this->AllocateValues(numberOfValues, sizeof(T));
}

template <typename T>
typename Storage<T, vtkm::cont::StorageTagBasic>::PortalType
Storage<T, vtkm::cont::StorageTagBasic>::GetPortal()
{
  auto v = static_cast<T*>(this->Array);
  return PortalType(v, v + this->NumberOfValues);
}

template <typename T>
typename Storage<T, vtkm::cont::StorageTagBasic>::PortalConstType
Storage<T, vtkm::cont::StorageTagBasic>::GetPortalConst() const
{
  auto v = static_cast<T*>(this->Array);
  return PortalConstType(v, v + this->NumberOfValues);
}

template <typename T>
T* Storage<T, vtkm::cont::StorageTagBasic>::GetArray()
{
  return static_cast<T*>(this->Array);
}

template <typename T>
const T* Storage<T, vtkm::cont::StorageTagBasic>::GetArray() const
{
  return static_cast<T*>(this->Array);
}

template <typename T>
vtkm::Pair<T*, void (*)(void*)> Storage<T, vtkm::cont::StorageTagBasic>::StealArray()
{
  vtkm::Pair<T*, void (*)(void*)> result(static_cast<T*>(this->Array), this->DeleteFunction);
  this->DeleteFunction = nullptr;
  return result;
}

} // namespace internal
}
} // namespace vtkm::cont
#endif
