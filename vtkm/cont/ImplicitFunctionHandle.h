//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_ImplicitFunctionHandle_h
#define vtk_m_cont_ImplicitFunctionHandle_h

#include <vtkm/ImplicitFunction.h>
#include <vtkm/cont/VirtualObjectHandle.h>

#ifdef VTKM_NO_DEPRECATED_VIRTUAL
#error "ImplicitFunction with virtual methods is removed. Do not include ImplicitFunctionHeader.h"
#endif

VTKM_DEPRECATED_SUPPRESS_BEGIN

namespace vtkm
{
namespace cont
{

namespace detail
{

// Wrong namespace, but it's only for deprecated code.
template <typename FunctionType>
class VTKM_ALWAYS_EXPORT ImplicitFunctionBaseExecWrapper : public vtkm::ImplicitFunction
{
  FunctionType Function;

public:
  VTKM_CONT ImplicitFunctionBaseExecWrapper(const FunctionType& function)
    : Function(function)
  {
  }

  VTKM_EXEC_CONT virtual ~ImplicitFunctionBaseExecWrapper() noexcept override
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC_CONT virtual Scalar Value(const Vector& point) const override
  {
    return this->Function.Value(point);
  }

  VTKM_EXEC_CONT virtual Vector Gradient(const Vector& point) const override
  {
    return this->Function.Gradient(point);
  }
};

} // vtkm::cont::detail

class VTKM_DEPRECATED(1.6,
                      "ImplicitFunctions with virtual methods are no longer supported. "
                      "Use vtkm::ImplicitFunctionX classes directly.") VTKM_ALWAYS_EXPORT
  ImplicitFunctionHandle : public vtkm::cont::VirtualObjectHandle<vtkm::ImplicitFunction>
{
private:
  using Superclass = vtkm::cont::VirtualObjectHandle<vtkm::ImplicitFunction>;

public:
  ImplicitFunctionHandle() = default;

  template <typename VirtualDerivedType,
            typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST,
            typename = typename std::enable_if<
              std::is_base_of<vtkm::ImplicitFunction, VirtualDerivedType>::value>::type>
  explicit ImplicitFunctionHandle(VirtualDerivedType* function,
                                  bool acquireOwnership = true,
                                  DeviceAdapterList devices = DeviceAdapterList())
    : Superclass(function, acquireOwnership, devices)
  {
  }

  template <typename ImplicitFunctionType,
            typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST>
  explicit ImplicitFunctionHandle(
    vtkm::internal::ImplicitFunctionBase<ImplicitFunctionType>* function,
    bool acquireOwnership = true,
    DeviceAdapterList devices = DeviceAdapterList())
    : Superclass(new detail::ImplicitFunctionBaseExecWrapper<ImplicitFunctionType>(
                   *reinterpret_cast<ImplicitFunctionType*>(function)),
                 true,
                 devices)
  {
    if (acquireOwnership)
    {
      delete function;
    }
  }

  template <typename ImplicitFunctionType,
            typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST>
  VTKM_CONT void Reset(vtkm::internal::ImplicitFunctionBase<ImplicitFunctionType>* function,
                       bool acquireOwnership = true,
                       DeviceAdapterList devices = DeviceAdapterList{})
  {
    this->Reset(new detail::ImplicitFunctionBaseExecWrapper<ImplicitFunctionType>(
                  *reinterpret_cast<ImplicitFunctionType*>(function)),
                true,
                devices);
    if (acquireOwnership)
    {
      delete function;
    }
  }

  template <typename VirtualDerivedType,
            typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST,
            typename = typename std::enable_if<
              std::is_base_of<vtkm::ImplicitFunction, VirtualDerivedType>::value>::type>
  VTKM_CONT void Reset(VirtualDerivedType* derived,
                       bool acquireOwnership = true,
                       DeviceAdapterList devices = DeviceAdapterList())
  {
    this->Superclass::Reset(derived, acquireOwnership, devices);
  }
};

template <typename ImplicitFunctionType,
          typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST>
VTKM_DEPRECATED(1.6,
                "ImplicitFunctions with virtual methods are no longer supported. "
                "Use vtkm::ImplicitFunctionX classes directly.")
VTKM_CONT ImplicitFunctionHandle
  make_ImplicitFunctionHandle(ImplicitFunctionType&& func,
                              DeviceAdapterList devices = DeviceAdapterList())
{
  using IFType = typename std::remove_reference<ImplicitFunctionType>::type;
  return ImplicitFunctionHandle(
    new IFType(std::forward<ImplicitFunctionType>(func)), true, devices);
}

template <typename ImplicitFunctionType, typename... Args>
VTKM_DEPRECATED(1.6,
                "ImplicitFunctions with virtual methods are no longer supported. "
                "Use vtkm::ImplicitFunctionX classes directly.")
VTKM_CONT ImplicitFunctionHandle make_ImplicitFunctionHandle(Args&&... args)
{
  return ImplicitFunctionHandle(new ImplicitFunctionType(std::forward<Args>(args)...),
                                true,
                                VTKM_DEFAULT_DEVICE_ADAPTER_LIST());
}

template <typename ImplicitFunctionType, typename DeviceAdapterList, typename... Args>
VTKM_CONT ImplicitFunctionHandle make_ImplicitFunctionHandle(Args&&... args)
{
  return ImplicitFunctionHandle(
    new ImplicitFunctionType(std::forward<Args>(args)...), true, DeviceAdapterList());
}

//============================================================================
/// A helpful wrapper that returns a functor that calls the (virtual) value method of a given
/// ImplicitFunction. Can be passed to things that expect a functor instead of an ImplictFunction
/// class (like an array transform).
///
class VTKM_DEPRECATED(1.6,
                      "ImplicitFunctions with virtual methods are no longer supported. "
                      "Use vtkm::ImplicitFunctionValueFunctor.")
  VTKM_ALWAYS_EXPORT ImplicitFunctionValueHandle : public vtkm::cont::ExecutionAndControlObjectBase
{
  vtkm::cont::ImplicitFunctionHandle Handle;

public:
  ImplicitFunctionValueHandle() = default;

  ImplicitFunctionValueHandle(const ImplicitFunctionHandle& handle)
    : Handle(handle)
  {
  }

  template <typename ImplicitFunctionType,
            typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST>
  explicit ImplicitFunctionValueHandle(ImplicitFunctionType* function,
                                       bool acquireOwnership = true,
                                       DeviceAdapterList devices = DeviceAdapterList())
    : Handle(function, acquireOwnership, devices)
  {
  }

  VTKM_CONT const vtkm::cont::ImplicitFunctionHandle& GetHandle() const { return this->Handle; }

  VTKM_CONT
  vtkm::ImplicitFunctionValue PrepareForExecution(vtkm::cont::DeviceAdapterId device,
                                                  vtkm::cont::Token& token) const
  {
    return vtkm::ImplicitFunctionValue(this->Handle.PrepareForExecution(device, token));
  }

  VTKM_CONT vtkm::ImplicitFunctionValue PrepareForControl() const
  {
    return vtkm::ImplicitFunctionValue(this->Handle.PrepareForControl());
  }
};

template <typename ImplicitFunctionType,
          typename DeviceAdapterList = VTKM_DEFAULT_DEVICE_ADAPTER_LIST>
VTKM_DEPRECATED(1.6,
                "ImplicitFunctions with virtual methods are no longer supported. "
                "Use vtkm::ImplicitFunctionValueFunctor.")
VTKM_CONT ImplicitFunctionValueHandle
  make_ImplicitFunctionValueHandle(ImplicitFunctionType&& func,
                                   DeviceAdapterList devices = DeviceAdapterList())
{
  using IFType = typename std::remove_reference<ImplicitFunctionType>::type;
  return ImplicitFunctionValueHandle(
    new IFType(std::forward<ImplicitFunctionType>(func)), true, devices);
}

template <typename ImplicitFunctionType, typename... Args>
VTKM_DEPRECATED(1.6,
                "ImplicitFunctions with virtual methods are no longer supported. "
                "Use vtkm::ImplicitFunctionValueFunctor.")
VTKM_CONT ImplicitFunctionValueHandle make_ImplicitFunctionValueHandle(Args&&... args)
{
  return ImplicitFunctionValueHandle(new ImplicitFunctionType(std::forward<Args>(args)...),
                                     true,
                                     VTKM_DEFAULT_DEVICE_ADAPTER_LIST());
}

template <typename ImplicitFunctionType, typename DeviceAdapterList, typename... Args>
VTKM_DEPRECATED(1.6,
                "ImplicitFunctions with virtual methods are no longer supported. "
                "Use vtkm::ImplicitFunctionValueFunctor.")
VTKM_CONT ImplicitFunctionValueHandle make_ImplicitFunctionValueHandle(Args&&... args)
{
  return ImplicitFunctionValueHandle(
    new ImplicitFunctionType(std::forward<Args>(args)...), true, DeviceAdapterList());
}
}
} // vtkm::cont

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
// Cuda seems to have a bug where it expects the template class VirtualObjectTransfer
// to be instantiated in a consistent order among all the translation units of an
// executable. Failing to do so results in random crashes and incorrect results.
// We workaroud this issue by explicitly instantiating VirtualObjectTransfer for
// all the implicit functions here.
#ifdef VTKM_CUDA
#include <vtkm/cont/internal/VirtualObjectTransferInstantiate.h>
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  vtkm::cont::detail::ImplicitFunctionBaseExecWrapper<vtkm::Box>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  vtkm::cont::detail::ImplicitFunctionBaseExecWrapper<vtkm::Cylinder>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  vtkm::cont::detail::ImplicitFunctionBaseExecWrapper<vtkm::Frustum>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  vtkm::cont::detail::ImplicitFunctionBaseExecWrapper<vtkm::Plane>);
VTKM_EXPLICITLY_INSTANTIATE_TRANSFER(
  vtkm::cont::detail::ImplicitFunctionBaseExecWrapper<vtkm::Sphere>);
#endif
#endif //VTKM_NO_DEPRECATED_VIRTUAL

VTKM_DEPRECATED_SUPPRESS_END

#endif // vtk_m_cont_ImplicitFunctionHandle_h
