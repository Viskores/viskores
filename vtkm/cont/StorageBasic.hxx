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
T* Storage<T, vtkm::cont::StorageTagBasic>::StealArray()
{
  this->DeleteFunction = nullptr;
  return static_cast<T*>(this->Array);
}

} // namespace internal
}
} // namespace vtkm::cont
