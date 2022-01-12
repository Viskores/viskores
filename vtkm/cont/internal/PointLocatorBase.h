//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_PointLocatorBase_h
#define vtk_m_cont_internal_PointLocatorBase_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/ExecutionObjectBase.h>

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
#include <vtkm/cont/PointLocator.h>
#include <vtkm/cont/VirtualObjectHandle.h>
#include <vtkm/exec/PointLocator.h>
#endif //!VTKM_NO_DEPRECATED_VIRTUAL

namespace vtkm
{
namespace cont
{
namespace internal
{

namespace detail
{

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
VTKM_DEPRECATED_SUPPRESS_BEGIN

// Wrong namespace, but it's only for deprecated code.
template <typename LocatorType>
class VTKM_ALWAYS_EXPORT PointLocatorBaseExecWrapper : public vtkm::exec::PointLocator
{
  LocatorType Locator;

public:
  VTKM_CONT PointLocatorBaseExecWrapper(const LocatorType& locator)
    : Locator(locator)
  {
  }

  VTKM_EXEC_CONT virtual ~PointLocatorBaseExecWrapper() noexcept override
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC void FindNearestNeighbor(const vtkm::Vec3f& queryPoint,
                                     vtkm::Id& pointId,
                                     vtkm::FloatDefault& distanceSquared) const override
  {
    return this->Locator.FindNearestNeighbor(queryPoint, pointId, distanceSquared);
  }
};

template <typename LocatorType>
struct PointLocatorBaseWrapperPrepareForExecutionFunctor
{
  template <typename Device>
  VTKM_CONT bool operator()(Device device,
                            vtkm::cont::VirtualObjectHandle<vtkm::exec::PointLocator>& execHandle,
                            const LocatorType& locator,
                            vtkm::cont::Token& token)
  {
    auto execObject = locator.PrepareForExecution(device, token);
    using WrapType = PointLocatorBaseExecWrapper<decltype(execObject)>;
    execHandle.Reset(new WrapType(execObject));
    return true;
  }
};

template <typename Derived>
class VTKM_ALWAYS_EXPORT PointLocatorBaseWrapper : public vtkm::cont::PointLocator
{
  Derived Locator;
  mutable vtkm::cont::VirtualObjectHandle<vtkm::exec::PointLocator> ExecutionObjectHandle;

public:
  PointLocatorBaseWrapper() = default;

  PointLocatorBaseWrapper(const Derived& locator)
    : Locator(locator)
  {
    this->SetCoordinates(locator.GetCoordinates());
  }

  VTKM_CONT const vtkm::exec::PointLocator* PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const override
  {
    const bool success =
      vtkm::cont::TryExecuteOnDevice(device,
                                     PointLocatorBaseWrapperPrepareForExecutionFunctor<Derived>{},
                                     this->ExecutionObjectHandle,
                                     this->Locator,
                                     token);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("PointLocatorWrapper", device);
    }
    return this->ExecutionObjectHandle.PrepareForExecution(device, token);
  }

private:
  void Build() override
  {
    this->Locator.SetCoordinates(this->GetCoordinates());
    this->Locator.Update();
  }
};

VTKM_DEPRECATED_SUPPRESS_END
#endif //!VTKM_NO_DEPRECATED_VIRTUAL


} // namespace detail

/// \brief Base class for all `PointLocator` classes.
///
/// `PointLocatorBase` uses the curiously recurring template pattern (CRTP). Subclasses
/// must provide their own type for the template parameter. Subclasses must implement
/// `Build` and `PrepareForExecution` methods.
///
template <typename Derived>
class VTKM_ALWAYS_EXPORT PointLocatorBase : public vtkm::cont::ExecutionObjectBase
{
public:
#ifndef VTKM_NO_DEPRECATED_VIRTUAL
  VTKM_DEPRECATED_SUPPRESS_BEGIN
  // Support deprecated classes
  operator detail::PointLocatorBaseWrapper<Derived>() const
  {
    return detail::PointLocatorBaseWrapper<Derived>(reinterpret_cast<const Derived&>(*this));
  }
  VTKM_DEPRECATED_SUPPRESS_END
#endif //!VTKM_NO_DEPRECATED_VIRTUAL

  vtkm::cont::CoordinateSystem GetCoordinates() const { return this->Coords; }
  void SetCoordinates(const vtkm::cont::CoordinateSystem& coords)
  {
    this->Coords = coords;
    this->SetModified();
  }

  void Update()
  {
    if (this->Modified)
    {
      static_cast<Derived*>(const_cast<PointLocatorBase*>(this))->Build();
      this->Modified = false;
    }
  }

  template <typename Device>
  VTKM_CONT VTKM_DEPRECATED(1.6, "PrepareForExecution now requires a vtkm::cont::Token object")
    const vtkm::cont::internal::ExecutionObjectType<Derived, Device> PrepareForExecution(
      Device device) const
  {
    vtkm::cont::Token token;
    return this->PrepareForExecution(device, token);
  }

protected:
  void SetModified() { this->Modified = true; }
  bool GetModified() const { return this->Modified; }

private:
  vtkm::cont::CoordinateSystem Coords;
  mutable bool Modified = true;
};

} // vtkm::cont::internal
} // vtkm::cont
} // vtkm

#endif // vtk_m_cont_internal_PointLocatorBase_h
