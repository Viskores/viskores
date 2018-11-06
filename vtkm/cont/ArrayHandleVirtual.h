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
#ifndef vtk_m_cont_ArrayHandleVirtual_h
#define vtk_m_cont_ArrayHandleVirtual_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/internal/DeviceAdapterTag.h>

#include <vtkm/cont/StorageVirtual.h>

#include <memory>

namespace vtkm
{
namespace cont
{

/// Specialization of ArrayHandle for virtual storage.
template <typename T>
class VTKM_ALWAYS_EXPORT ArrayHandle<T, ::vtkm::cont::StorageTagVirtual>
  : public vtkm::cont::internal::ArrayHandleBase
{
public:
  using StorageTag = vtkm::cont::StorageTagVirtual;
  using StorageType = vtkm::cont::internal::Storage<void, vtkm::cont::StorageTagVirtual>;

  using ValueType = T;

  using PortalControl = vtkm::ArrayPortalRef<T>;
  using PortalConstControl = vtkm::ArrayPortalRef<T>;

  template <typename Device>
  struct ExecutionTypes
  {
    using Portal = vtkm::ArrayPortalRef<T>;
    using PortalConst = vtkm::ArrayPortalRef<T>;
  };

  ///construct an invlaid virtual array handle that has a nullptr storage
  ArrayHandle()
    : Storage(nullptr){};

  ///Move existing shared_ptr of vtkm::cont::StorageVirtual to be
  ///owned by this ArrayHandleVirtual.
  ///This is generally how derived class construct a valid ArrayHandleVirtual
  template <typename DerivedStorage>
  explicit ArrayHandle(std::shared_ptr<DerivedStorage>&& storage) noexcept
    : Storage(std::move(storage))
  {
    using is_base = std::is_base_of<vtkm::cont::StorageVirtual, DerivedStorage>;
    static_assert(is_base::value,
                  "Storage for ArrayHandleVirtual needs to derive from vtkm::cont::StorageVirual");
  }

  ///Move existing unique_ptr of vtkm::cont::StorageVirtual to be
  ///owned by this ArrayHandleVirtual.
  ///This is how a derived class construct a valid ArrayHandleVirtual
  template <typename DerivedStorage>
  explicit ArrayHandle(std::unique_ptr<DerivedStorage>&& storage) noexcept
    : Storage(std::move(storage))
  {
    using is_base = std::is_base_of<vtkm::cont::StorageVirtual, DerivedStorage>;
    static_assert(is_base::value,
                  "Storage for ArrayHandleVirtual needs to derive from vtkm::cont::StorageVirual");
  }

  ///copy another existing virtual array handle
  ArrayHandle(const ArrayHandle<T, vtkm::cont::StorageTagVirtual>& src) = default;

  ///move from one virtual array handle to another
  ArrayHandle(ArrayHandle<T, vtkm::cont::StorageTagVirtual>&& src) noexcept
    : Storage(std::move(src.Storage))
  {
  }

  VTKM_CONT ArrayHandle<T, vtkm::cont::StorageTagVirtual>& operator=(
    const ArrayHandle<T, vtkm::cont::StorageTagVirtual>& src) = default;
  VTKM_CONT ArrayHandle<T, vtkm::cont::StorageTagVirtual>& operator=(
    ArrayHandle<T, vtkm::cont::StorageTagVirtual>&& src) noexcept
  {
    this->Storage = std::move(src.Storage);
    return *this;
  }

  /// Returns true if this array's storage matches the type passed in.
  ///
  template <typename ArrayHandleType>
  VTKM_CONT bool IsType() const
  {
    VTKM_IS_ARRAY_HANDLE(ArrayHandleType);
    //We need to go long the way to find the StorageType
    //as StorageType is private on lots of derived ArrayHandles
    //See Issue #314

    using VT = typename ArrayHandleType::ValueType;
    static_assert(
      std::is_same<VT, T>::value,
      "ArrayHandleVirtual<ValueType> can only be casted to an ArrayHandle of the same ValueType.");

    using ST = typename ArrayHandleType::StorageTag;
    return this->Storage->IsType(typeid(vtkm::cont::internal::Storage<VT, ST>));
  }

  /// Returns a view on the internal storage of the ArrayHandleVirtual
  ///
  VTKM_CONT const StorageType* GetStorage() const { return this->Storage.get(); }

  /// Returns a new instance of an ArrayHandleVirtual with the same storage
  ///
  VTKM_CONT ArrayHandle<T, ::vtkm::cont::StorageTagVirtual> NewInstance() const
  {
    return (this->Storage)
      ? ArrayHandle<T, ::vtkm::cont::StorageTagVirtual>(this->Storage->NewInstance())
      : ArrayHandle<T, ::vtkm::cont::StorageTagVirtual>();
  }

  // Return a ArrayPortalRef that wraps the real virtual portal. We need a stack object for
  // the following reasons:
  // 1. Device Adapter algorithms only support const AH<T,S>& and not const AH<T,S>*
  // 2. Devices will want to get the length of a portal before execution, but for CUDA
  //  we can't ask this information of the portal as it only valid on the device, instead
  //  we have to store this information also in the ref wrapper
  vtkm::ArrayPortalRef<T> PrepareForInput(vtkm::cont::DeviceAdapterId devId) const
  {
    return make_ArrayPortalRef(
      static_cast<const vtkm::ArrayPortalVirtual<T>*>(this->Storage->PrepareForInput(devId)),
      this->GetNumberOfValues());
  }

  vtkm::ArrayPortalRef<T> PrepareForOutput(vtkm::Id numberOfValues,
                                           vtkm::cont::DeviceAdapterId devId)
  {
    return make_ArrayPortalRef(static_cast<const vtkm::ArrayPortalVirtual<T>*>(
                                 this->Storage->PrepareForOutput(numberOfValues, devId)),
                               numberOfValues);
  }

  vtkm::Id GetNumberOfValues() const { return this->Storage->GetNumberOfValues(); }

  /// Releases any resources being used in the execution environment (that are
  /// not being shared by the control environment).
  ///
  void ReleaseResourcesExecution() { return this->Storage->ReleaseResourcesExecution(); }

  /// Releases all resources in both the control and execution environments.
  ///
  void ReleaseResources() { return this->Storage->ReleaseResources(); }

  /// Get the array portal of the control array.
  /// Since worklet invocations are asynchronous and this routine is a synchronization point,
  /// exceptions maybe thrown for errors from previously executed worklets.
  ///
  PortalControl GetPortalControl()
  {
    return make_ArrayPortalRef(
      static_cast<const vtkm::ArrayPortalVirtual<T>*>(this->Storage->GetPortalControl()),
      this->GetNumberOfValues());
  }

  /// Get the array portal of the control array.
  /// Since worklet invocations are asynchronous and this routine is a synchronization point,
  /// exceptions maybe thrown for errors from previously executed worklets.
  ///
  PortalConstControl GetPortalConstControl() const
  {
    return make_ArrayPortalRef(
      static_cast<const vtkm::ArrayPortalVirtual<T>*>(this->Storage->GetPortalConstControl()),
      this->GetNumberOfValues());
  }

protected:
  std::shared_ptr<StorageType> Storage = nullptr;
};

template <typename T>
using ArrayHandleVirtual = vtkm::cont::ArrayHandle<T, vtkm::cont::StorageTagVirtual>;
}
} //namespace vtkm::cont


#include <vtkm/cont/ArrayHandleAny.h>


#endif
