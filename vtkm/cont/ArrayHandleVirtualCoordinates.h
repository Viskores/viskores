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
#ifndef vtk_m_cont_ArrayHandleVirtualCoordinates_h
#define vtk_m_cont_ArrayHandleVirtualCoordinates_h

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>
#include <vtkm/cont/VirtualObjectHandle.h>
#include <vtkm/cont/internal/DeviceAdapterListHelpers.h>
#include <vtkm/cont/internal/DynamicTransform.h>

#include <vtkm/VecTraits.h>
#include <vtkm/VirtualObjectBase.h>

#include <memory>
#include <type_traits>

namespace vtkm
{
namespace cont
{

namespace internal
{

//=============================================================================
class VTKM_ALWAYS_EXPORT CoordinatesPortalBase : public VirtualObjectBase
{
public:
  VTKM_EXEC_CONT virtual vtkm::Vec<vtkm::FloatDefault, 3> Get(vtkm::Id i) const = 0;
  VTKM_EXEC_CONT virtual void Set(vtkm::Id i,
                                  const vtkm::Vec<vtkm::FloatDefault, 3>& val) const = 0;
};

template <typename PortalType, typename ValueType>
class VTKM_ALWAYS_EXPORT CoordinatesPortalImpl : public CoordinatesPortalBase
{
public:
  VTKM_CONT CoordinatesPortalImpl() = default;
  VTKM_CONT explicit CoordinatesPortalImpl(const PortalType& portal)
    : Portal(portal)
  {
  }

  VTKM_CONT void SetPortal(const PortalType& portal) { this->Portal = portal; }

  VTKM_EXEC_CONT vtkm::Vec<vtkm::FloatDefault, 3> Get(vtkm::Id i) const override
  {
    auto val = this->Portal.Get(i);
    return { static_cast<vtkm::FloatDefault>(val[0]),
             static_cast<vtkm::FloatDefault>(val[1]),
             static_cast<vtkm::FloatDefault>(val[2]) };
  }

  VTKM_EXEC_CONT void Set(vtkm::Id i, const vtkm::Vec<vtkm::FloatDefault, 3>& val) const override
  {
    using VecType = typename PortalType::ValueType;
    using ComponentType = typename vtkm::VecTraits<VecType>::ComponentType;

    this->Portal.Set(i,
                     { static_cast<ComponentType>(val[0]),
                       static_cast<ComponentType>(val[1]),
                       static_cast<ComponentType>(val[2]) });
  }

private:
  PortalType Portal;
};

template <typename PortalType>
class VTKM_ALWAYS_EXPORT CoordinatesPortalImpl<PortalType, void*> : public CoordinatesPortalBase
{
public:
  VTKM_CONT CoordinatesPortalImpl() = default;
  VTKM_CONT explicit CoordinatesPortalImpl(const PortalType&) {}
  VTKM_CONT void SetPortal(const PortalType&) {}
  VTKM_EXEC_CONT vtkm::Vec<vtkm::FloatDefault, 3> Get(vtkm::Id) const override { return {}; }
  VTKM_EXEC_CONT void Set(vtkm::Id, const vtkm::Vec<vtkm::FloatDefault, 3>&) const override {}
};

template <typename PortalType>
class VTKM_ALWAYS_EXPORT CoordinatesPortal
  : public CoordinatesPortalImpl<PortalType, typename PortalType::ValueType>
{
public:
  VTKM_CONT CoordinatesPortal() = default;
  VTKM_CONT explicit CoordinatesPortal(const PortalType& portal)
    : CoordinatesPortalImpl<PortalType, typename PortalType::ValueType>(portal)
  {
  }
};

template <typename PortalType>
class VTKM_ALWAYS_EXPORT CoordinatesPortalConst : public CoordinatesPortalBase
{
public:
  VTKM_CONT CoordinatesPortalConst() = default;
  VTKM_CONT explicit CoordinatesPortalConst(const PortalType& portal)
    : Portal(portal)
  {
  }

  VTKM_CONT void SetPortal(const PortalType& portal) { this->Portal = portal; }

  VTKM_EXEC_CONT vtkm::Vec<vtkm::FloatDefault, 3> Get(vtkm::Id i) const override
  {
    auto val = this->Portal.Get(i);
    return { static_cast<vtkm::FloatDefault>(val[0]),
             static_cast<vtkm::FloatDefault>(val[1]),
             static_cast<vtkm::FloatDefault>(val[2]) };
  }

  VTKM_EXEC_CONT void Set(vtkm::Id, const vtkm::Vec<vtkm::FloatDefault, 3>&) const override {}

private:
  PortalType Portal;
};

class VTKM_ALWAYS_EXPORT ArrayPortalVirtualCoordinates
{
public:
  using ValueType = vtkm::Vec<vtkm::FloatDefault, 3>;

  VTKM_EXEC_CONT ArrayPortalVirtualCoordinates()
    : NumberOfValues(0)
    , VirtualPortal(nullptr)
  {
  }

  VTKM_EXEC_CONT ArrayPortalVirtualCoordinates(vtkm::Id numberOfValues,
                                               const CoordinatesPortalBase* virtualPortal)
    : NumberOfValues(numberOfValues)
    , VirtualPortal(virtualPortal)
  {
  }

  VTKM_EXEC_CONT vtkm::Id GetNumberOfValues() const { return this->NumberOfValues; }

  VTKM_EXEC_CONT ValueType Get(vtkm::Id i) const { return this->VirtualPortal->Get(i); }

  VTKM_EXEC_CONT void Set(vtkm::Id i, const ValueType& val) const
  {
    this->VirtualPortal->Set(i, val);
  }

private:
  vtkm::Id NumberOfValues;
  const CoordinatesPortalBase* VirtualPortal;
};

//=============================================================================
class VTKM_ALWAYS_EXPORT CoordinatesArrayHandleBase
{
public:
  using Portal = ArrayPortalVirtualCoordinates;
  using PortalConst = ArrayPortalVirtualCoordinates;

  virtual ~CoordinatesArrayHandleBase() = default;

  VTKM_CONT virtual vtkm::Id GetNumberOfValues() const = 0;

  VTKM_CONT virtual Portal GetPortalControl() = 0;
  VTKM_CONT virtual PortalConst GetPortalConstControl() = 0;
  VTKM_CONT virtual void Allocate(vtkm::Id numberOfValues) = 0;
  VTKM_CONT virtual void Shrink(vtkm::Id numberOfValues) = 0;
  VTKM_CONT virtual void ReleaseResources() = 0;
  VTKM_CONT virtual void ReleaseResourcesExecution() = 0;

  VTKM_CONT virtual PortalConst PrepareForInput(vtkm::cont::DeviceAdapterId deviceId) = 0;
  VTKM_CONT virtual Portal PrepareForOutput(vtkm::Id numberOfValues,
                                            vtkm::cont::DeviceAdapterId deviceId) = 0;
  VTKM_CONT virtual Portal PrepareForInPlace(vtkm::cont::DeviceAdapterId deviceId) = 0;
};

template <typename ArrayHandleType>
class VTKM_ALWAYS_EXPORT CoordinatesArrayHandleArrayWrapper : public CoordinatesArrayHandleBase
{
public:
  VTKM_CONT explicit CoordinatesArrayHandleArrayWrapper(const ArrayHandleType& array)
    : Array(array)
  {
  }

  VTKM_CONT const ArrayHandleType& GetArray() const { return this->Array; }

protected:
  ArrayHandleType Array;
};

template <typename ArrayHandleType, typename DeviceList>
class VTKM_ALWAYS_EXPORT CoordinatesArrayHandle
  : public CoordinatesArrayHandleArrayWrapper<ArrayHandleType>
{
public:
  static_assert(std::is_same<DeviceList, vtkm::cont::DeviceAdapterListTagCommon>::value, "error");

  using Portal = CoordinatesArrayHandleBase::Portal;
  using PortalConst = CoordinatesArrayHandleBase::PortalConst;

  VTKM_CONT explicit CoordinatesArrayHandle(const ArrayHandleType& array)
    : CoordinatesArrayHandleArrayWrapper<ArrayHandleType>(array)
  {
  }

  VTKM_CONT vtkm::Id GetNumberOfValues() const override { return this->Array.GetNumberOfValues(); }

  VTKM_CONT Portal GetPortalControl() override
  {
    this->ControlPortal.SetPortal(this->Array.GetPortalControl());
    return Portal(this->GetNumberOfValues(), &this->ControlPortal);
  }

  VTKM_CONT PortalConst GetPortalConstControl() override
  {
    this->ControlConstPortal.SetPortal(this->Array.GetPortalConstControl());
    return PortalConst(this->GetNumberOfValues(), &this->ControlConstPortal);
  }

  VTKM_CONT void Allocate(vtkm::Id numberOfValues) override
  {
    this->Array.Allocate(numberOfValues);
  }

  VTKM_CONT void Shrink(vtkm::Id numberOfValues) override { this->Array.Shrink(numberOfValues); }

  VTKM_CONT void ReleaseResources() override { this->Array.ReleaseResources(); }

  VTKM_CONT void ReleaseResourcesExecution() override { this->Array.ReleaseResourcesExecution(); }

  VTKM_CONT PortalConst PrepareForInput(vtkm::cont::DeviceAdapterId deviceId) override
  {
    PortalConst portal;
    vtkm::cont::internal::FindDeviceAdapterTagAndCall(
      deviceId, DeviceList(), PrepareForInputFunctor(), this, portal);
    return portal;
  }

  VTKM_CONT Portal PrepareForOutput(vtkm::Id numberOfValues,
                                    vtkm::cont::DeviceAdapterId deviceId) override
  {
    Portal portal;
    vtkm::cont::internal::FindDeviceAdapterTagAndCall(
      deviceId, DeviceList(), PrepareForOutputFunctor(), this, numberOfValues, portal);
    return portal;
  }

  VTKM_CONT Portal PrepareForInPlace(vtkm::cont::DeviceAdapterId deviceId) override
  {
    Portal portal;
    vtkm::cont::internal::FindDeviceAdapterTagAndCall(
      deviceId, DeviceList(), PrepareForInPlaceFunctor(), this, portal);
    return portal;
  }

private:
  struct PrepareForInputFunctor
  {
    template <typename DeviceAdapter>
    VTKM_CONT void operator()(DeviceAdapter device,
                              CoordinatesArrayHandle* instance,
                              PortalConst& ret) const
    {
      auto portal = instance->Array.PrepareForInput(device);
      instance->DevicePortalHandle.Reset(new CoordinatesPortalConst<decltype(portal)>(portal),
                                         true,
                                         vtkm::ListTagBase<DeviceAdapter>());
      ret = PortalConst(portal.GetNumberOfValues(),
                        instance->DevicePortalHandle.PrepareForExecution(device));
    }
  };

  struct PrepareForOutputFunctor
  {
    template <typename DeviceAdapter>
    VTKM_CONT void operator()(DeviceAdapter device,
                              CoordinatesArrayHandle* instance,
                              vtkm::Id numberOfValues,
                              Portal& ret) const
    {
      auto portal = instance->Array.PrepareForOutput(numberOfValues, device);
      instance->DevicePortalHandle.Reset(
        new CoordinatesPortal<decltype(portal)>(portal), true, vtkm::ListTagBase<DeviceAdapter>());
      ret = Portal(numberOfValues, instance->DevicePortalHandle.PrepareForExecution(device));
    }
  };

  struct PrepareForInPlaceFunctor
  {
    template <typename DeviceAdapter>
    VTKM_CONT void operator()(DeviceAdapter device,
                              CoordinatesArrayHandle* instance,
                              Portal& ret) const
    {
      auto portal = instance->Array.PrepareForInPlace(device);
      instance->DevicePortalHandle.Reset(
        new CoordinatesPortal<decltype(portal)>(portal), true, vtkm::ListTagBase<DeviceAdapter>());
      ret = Portal(instance->Array.GetNumberOfValues(),
                   instance->DevicePortalHandle.PrepareForExecution(device));
    }
  };

  CoordinatesPortal<typename ArrayHandleType::PortalControl> ControlPortal;
  CoordinatesPortalConst<typename ArrayHandleType::PortalConstControl> ControlConstPortal;
  vtkm::cont::VirtualObjectHandle<CoordinatesPortalBase> DevicePortalHandle;
};

//=============================================================================
struct VTKM_ALWAYS_EXPORT StorageTagVirtualCoordinates
{
};

template <>
class Storage<vtkm::Vec<vtkm::FloatDefault, 3>, StorageTagVirtualCoordinates>
{
public:
  using ValueType = vtkm::Vec<vtkm::FloatDefault, 3>;
  using PortalType = CoordinatesArrayHandleBase::Portal;
  using PortalConstType = CoordinatesArrayHandleBase::PortalConst;

  VTKM_CONT Storage() = default;

  template <typename ArrayHandleType, typename DeviceList>
  VTKM_CONT explicit Storage(const ArrayHandleType& array, DeviceList)
    : Array(new CoordinatesArrayHandle<ArrayHandleType, DeviceList>(array))
  {
  }

  VTKM_CONT PortalType GetPortal() { return this->Array->GetPortalControl(); }

  VTKM_CONT PortalConstType GetPortalConst() const { return this->Array->GetPortalConstControl(); }

  VTKM_CONT vtkm::Id GetNumberOfValues() const { return this->Array->GetNumberOfValues(); }

  VTKM_CONT void Allocate(vtkm::Id numberOfValues) { this->Array->Allocate(numberOfValues); }

  VTKM_CONT void Shrink(vtkm::Id numberOfValues) { this->Array->Shrink(numberOfValues); }

  VTKM_CONT void ReleaseResources() { this->Array->ReleaseResources(); }

  VTKM_CONT CoordinatesArrayHandleBase* GetVirtualArray() const { return this->Array.get(); }

private:
  std::shared_ptr<CoordinatesArrayHandleBase> Array;
};

//=============================================================================
template <typename DeviceAdapter>
class ArrayTransfer<vtkm::Vec<vtkm::FloatDefault, 3>, StorageTagVirtualCoordinates, DeviceAdapter>
{
public:
  using ValueType = vtkm::Vec<vtkm::FloatDefault, 3>;
  using StorageType = vtkm::cont::internal::Storage<ValueType, StorageTagVirtualCoordinates>;

  using PortalControl = typename StorageType::PortalType;
  using PortalConstControl = typename StorageType::PortalConstType;
  using PortalExecution = CoordinatesArrayHandleBase::Portal;
  using PortalConstExecution = CoordinatesArrayHandleBase::PortalConst;

  VTKM_CONT
  ArrayTransfer(StorageType* storage)
    : Array(storage->GetVirtualArray())
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfValues() const { return this->Array->GetNumberOfValues(); }

  VTKM_CONT
  PortalConstExecution PrepareForInput(bool)
  {
    return this->Array->PrepareForInput(DeviceAdapter());
  }

  VTKM_CONT
  PortalExecution PrepareForInPlace(bool)
  {
    return this->Array->PrepareForInPlace(DeviceAdapter());
  }

  VTKM_CONT
  PortalExecution PrepareForOutput(vtkm::Id numberOfValues)
  {
    return this->Array->PrepareForOutput(numberOfValues, DeviceAdapter());
  }

  VTKM_CONT
  void RetrieveOutputData(StorageType*) const
  {
    // Implementation of this method should be unnecessary.
  }

  VTKM_CONT
  void Shrink(vtkm::Id numberOfValues) { this->Array->Shrink(numberOfValues); }

  // ArrayTransfer should only be capable of releasing resources in the execution
  // environment
  VTKM_CONT
  void ReleaseResources() { this->Array->ReleaseResourcesExecution(); }

private:
  CoordinatesArrayHandleBase* Array;
};

} // internal

//=============================================================================
class VTKM_ALWAYS_EXPORT ArrayHandleVirtualCoordinates
  : public ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>, internal::StorageTagVirtualCoordinates>
{
public:
  VTKM_ARRAY_HANDLE_SUBCLASS_NT(
    ArrayHandleVirtualCoordinates,
    (ArrayHandle<vtkm::Vec<vtkm::FloatDefault, 3>, internal::StorageTagVirtualCoordinates>));

  template <typename StorageTag, typename DeviceList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG>
  explicit ArrayHandleVirtualCoordinates(
    const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>, StorageTag>& array,
    DeviceList devices = DeviceList())
    : Superclass(typename Superclass::StorageType(array, devices))
  {
  }

  template <typename StorageTag, typename DeviceList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST_TAG>
  explicit ArrayHandleVirtualCoordinates(
    const vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>, StorageTag>& array,
    DeviceList devices = DeviceList())
    : Superclass(typename Superclass::StorageType(array, devices))
  {
  }

  template <typename ArrayHandleType>
  bool IsType() const
  {
    return this->GetArrayHandleWrapper<ArrayHandleType>() != nullptr;
  }

  template <typename ArrayHandleType>
  bool IsSameType(const ArrayHandleType&) const
  {
    return this->GetArrayHandleWrapper<ArrayHandleType>() != nullptr;
  }

  template <typename ArrayHandleType>
  const ArrayHandleType Cast() const
  {
    auto wrapper = this->GetArrayHandleWrapper<ArrayHandleType>();
    if (!wrapper)
    {
      throw vtkm::cont::ErrorBadType("dynamic cast failed");
    }
    return ArrayHandleType(wrapper->GetArray());
  }

private:
  template <typename ArrayHandleType>
  struct WrapperType
  {
    VTKM_IS_ARRAY_HANDLE(ArrayHandleType);

    using ValueType = typename ArrayHandleType::ValueType;
    using StorageTag = typename ArrayHandleType::StorageTag;
    using BaseArrayHandleType = vtkm::cont::ArrayHandle<ValueType, StorageTag>;
    using Type = internal::CoordinatesArrayHandleArrayWrapper<BaseArrayHandleType>;
  };

  template <typename ArrayHandleType>
  VTKM_CONT const typename WrapperType<ArrayHandleType>::Type* GetArrayHandleWrapper() const
  {
    auto va = this->GetStorage().GetVirtualArray();
    return dynamic_cast<const typename WrapperType<ArrayHandleType>::Type*>(va);
  }
};

template <typename Functor, typename... Args>
void CastAndCall(const vtkm::cont::ArrayHandleVirtualCoordinates& coords,
                 Functor&& f,
                 Args&&... args)
{
  if (coords.IsType<vtkm::cont::ArrayHandleUniformPointCoordinates>())
  {
    f(coords.Cast<vtkm::cont::ArrayHandleUniformPointCoordinates>(), std::forward<Args>(args)...);
  }
  else
  {
    f(coords, std::forward<Args>(args)...);
  }
}

template <typename Functor, typename... Args>
void CastAndCall(const typename vtkm::cont::ArrayHandleVirtualCoordinates::Superclass& coords,
                 Functor&& f,
                 Args&&... args)
{
  CastAndCall(static_cast<const vtkm::cont::ArrayHandleVirtualCoordinates&>(coords),
              std::forward<Functor>(f),
              std::forward<Args>(args)...);
}
}
} // vtkm::cont

#ifdef VTKM_CUDA

// Cuda seems to have a bug where it expects the template class VirtualObjectTransfer
// to be instantiated in a consistent order among all the translation units of an
// executable. Failing to do so results in random crashes and incorrect results.
// We workaroud this issue by explicitly instantiating VirtualObjectTransfer for
// all the portal types here.

#include <vtkm/cont/cuda/internal/VirtualObjectTransferCuda.h>

#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCartesianProduct.h>
#include <vtkm/cont/ArrayHandleCompositeVector.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleUniformPointCoordinates.h>

namespace vtkm
{
namespace cont
{
namespace internal
{

template <typename ArrayHandleType>
struct CudaPortalTypes
{
  using PortalConst = typename ArrayHandleType::template ExecutionTypes<
    vtkm::cont::DeviceAdapterTagCuda>::PortalConst;
  using Portal =
    typename ArrayHandleType::template ExecutionTypes<vtkm::cont::DeviceAdapterTagCuda>::Portal;
};

using CudaPortalsBasicF32 = CudaPortalTypes<vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float32, 3>>>;
using CudaPortalsBasicF64 = CudaPortalTypes<vtkm::cont::ArrayHandle<vtkm::Vec<vtkm::Float64, 3>>>;
using CudaPortalsUniformPointCoordinates =
  CudaPortalTypes<vtkm::cont::ArrayHandleUniformPointCoordinates>;
using CudaPortalsRectilinearCoords = CudaPortalTypes<
  vtkm::cont::ArrayHandleCartesianProduct<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                          vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                          vtkm::cont::ArrayHandle<vtkm::FloatDefault>>>;
using CudaPortalsCompositeCoords = CudaPortalTypes<
  vtkm::cont::ArrayHandleCompositeVector<vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                         vtkm::cont::ArrayHandle<vtkm::FloatDefault>,
                                         vtkm::cont::ArrayHandle<vtkm::FloatDefault>>>;
}
}
} // vtkm::cont::internal

VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(vtkm::cont::internal::CoordinatesPortalConst<
                                     vtkm::cont::internal::CudaPortalsBasicF32::PortalConst>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  vtkm::cont::internal::CoordinatesPortal<vtkm::cont::internal::CudaPortalsBasicF32::Portal>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(vtkm::cont::internal::CoordinatesPortalConst<
                                     vtkm::cont::internal::CudaPortalsBasicF64::PortalConst>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  vtkm::cont::internal::CoordinatesPortal<vtkm::cont::internal::CudaPortalsBasicF64::Portal>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  vtkm::cont::internal::CoordinatesPortalConst<
    vtkm::cont::internal::CudaPortalsUniformPointCoordinates::PortalConst>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  vtkm::cont::internal::CoordinatesPortal<
    vtkm::cont::internal::CudaPortalsUniformPointCoordinates::Portal>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  vtkm::cont::internal::CoordinatesPortalConst<
    vtkm::cont::internal::CudaPortalsRectilinearCoords::PortalConst>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(vtkm::cont::internal::CoordinatesPortal<
                                     vtkm::cont::internal::CudaPortalsRectilinearCoords::Portal>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  vtkm::cont::internal::CoordinatesPortalConst<
    vtkm::cont::internal::CudaPortalsCompositeCoords::PortalConst>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(vtkm::cont::internal::CoordinatesPortal<
                                     vtkm::cont::internal::CudaPortalsCompositeCoords::Portal>);

#endif // VTKM_CUDA

#endif // vtk_m_cont_ArrayHandleVirtualCoordinates_h
