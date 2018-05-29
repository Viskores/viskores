//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================

#ifndef vtk_m_cont_internal_DeviceAdapterAtomicArrayImplementation_h
#define vtk_m_cont_internal_DeviceAdapterAtomicArrayImplementation_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayPortalToIterators.h>
#include <vtkm/cont/StorageBasic.h>

#include <vtkm/internal/Configure.h>
#include <vtkm/internal/Windows.h>

#include <vtkm/Types.h>

namespace vtkm
{
namespace cont
{

/// \brief Class providing a device-specific atomic interface.
///
/// The class provide the actual implementation used by vtkm::exec::AtomicArray.
/// A serial default implementation is provided. But each device will have a different
/// implementation.
///
/// Serial requires no form of atomicity
///
template <typename T, typename DeviceTag>
class DeviceAdapterAtomicArrayImplementation
{
  using PortalType =
    typename vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic>::template ExecutionTypes<
      DeviceTag>::Portal;
  using IteratorsType = vtkm::cont::ArrayPortalToIterators<PortalType>;
  IteratorsType Iterators;

public:
  VTKM_CONT
  DeviceAdapterAtomicArrayImplementation(
    vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagBasic> handle)
    : Iterators(IteratorsType(handle.PrepareForInPlace(DeviceTag())))
  {
  }

  T Add(vtkm::Id index, const T& value) const
  {
    T* lockedValue;
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL > 0
    using IteratorType = typename vtkm::cont::ArrayPortalToIterators<PortalType>::IteratorType;
    typename IteratorType::pointer temp =
      &(*(Iterators.GetBegin() + static_cast<std::ptrdiff_t>(index)));
    lockedValue = temp;
    return this->vtkmAtomicAdd(lockedValue, value);
#else
    lockedValue = (Iterators.GetBegin() + index);
    return this->vtkmAtomicAdd(lockedValue, value);
#endif
  }

  T CompareAndSwap(vtkm::Id index, const T& newValue, const T& oldValue) const
  {
    T* lockedValue;
#if defined(_ITERATOR_DEBUG_LEVEL) && _ITERATOR_DEBUG_LEVEL > 0
    using IteratorType = typename vtkm::cont::ArrayPortalToIterators<PortalType>::IteratorType;
    typename IteratorType::pointer temp =
      &(*(Iterators.GetBegin() + static_cast<std::ptrdiff_t>(index)));
    lockedValue = temp;
    return this->vtkmCompareAndSwap(lockedValue, newValue, oldValue);
#else
    lockedValue = (Iterators.GetBegin() + index);
    return this->vtkmCompareAndSwap(lockedValue, newValue, oldValue);
#endif
  }

private:
#if defined(VTKM_MSVC) //MSVC atomics
  vtkm::Int32 vtkmAtomicAdd(vtkm::Int32* address, const vtkm::Int32& value) const
  {
    return InterlockedExchangeAdd(reinterpret_cast<volatile long*>(address), value);
  }

  vtkm::Int64 vtkmAtomicAdd(vtkm::Int64* address, const vtkm::Int64& value) const
  {
    return InterlockedExchangeAdd64(reinterpret_cast<volatile long long*>(address), value);
  }

  vtkm::Int32 vtkmCompareAndSwap(vtkm::Int32* address,
                                 const vtkm::Int32& newValue,
                                 const vtkm::Int32& oldValue) const
  {
    return InterlockedCompareExchange(
      reinterpret_cast<volatile long*>(address), newValue, oldValue);
  }

  vtkm::Int64 vtkmCompareAndSwap(vtkm::Int64* address,
                                 const vtkm::Int64& newValue,
                                 const vtkm::Int64& oldValue) const
  {
    return InterlockedCompareExchange64(
      reinterpret_cast<volatile long long*>(address), newValue, oldValue);
  }

#else //gcc built-in atomics

  vtkm::Int32 vtkmAtomicAdd(vtkm::Int32* address, const vtkm::Int32& value) const
  {
    return __sync_fetch_and_add(address, value);
  }

  vtkm::Int64 vtkmAtomicAdd(vtkm::Int64* address, const vtkm::Int64& value) const
  {
    return __sync_fetch_and_add(address, value);
  }

  vtkm::Int32 vtkmCompareAndSwap(vtkm::Int32* address,
                                 const vtkm::Int32& newValue,
                                 const vtkm::Int32& oldValue) const
  {
    return __sync_val_compare_and_swap(address, oldValue, newValue);
  }

  vtkm::Int64 vtkmCompareAndSwap(vtkm::Int64* address,
                                 const vtkm::Int64& newValue,
                                 const vtkm::Int64& oldValue) const
  {
    return __sync_val_compare_and_swap(address, oldValue, newValue);
  }

#endif
};
}
} // end namespace vtkm::cont

#endif // vtk_m_cont_internal_DeviceAdapterAtomicArrayImplementation_h
