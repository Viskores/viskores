//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_internal_CellLocatorBase_h
#define vtk_m_cont_internal_CellLocatorBase_h

#include <vtkm/cont/vtkm_cont_export.h>

#include <vtkm/Types.h>
#include <vtkm/cont/CoordinateSystem.h>
#include <vtkm/cont/DynamicCellSet.h>
#include <vtkm/cont/ExecutionObjectBase.h>

#ifndef VTKM_NO_DEPRECATED_VIRTUAL
// To support deprecated implementation
#include <vtkm/cont/CellLocator.h>
#include <vtkm/cont/VirtualObjectHandle.h>
#include <vtkm/exec/CellLocator.h>
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
class VTKM_ALWAYS_EXPORT CellLocatorBaseExecWrapper : public vtkm::exec::CellLocator
{
  LocatorType Locator;

public:
  VTKM_CONT CellLocatorBaseExecWrapper(const LocatorType& locator)
    : Locator(locator)
  {
  }

  VTKM_EXEC_CONT virtual ~CellLocatorBaseExecWrapper() noexcept override
  {
    // This must not be defaulted, since defaulted virtual destructors are
    // troublesome with CUDA __host__ __device__ markup.
  }

  VTKM_EXEC vtkm::ErrorCode FindCell(const vtkm::Vec3f& point,
                                     vtkm::Id& cellId,
                                     vtkm::Vec3f& parametric) const override
  {
    return this->Locator.FindCell(point, cellId, parametric);
  }
};

template <typename LocatorType>
struct CellLocatorBaseWrapperPrepareForExecutionFunctor
{
  template <typename Device>
  VTKM_CONT bool operator()(Device device,
                            vtkm::cont::VirtualObjectHandle<vtkm::exec::CellLocator>& execHandle,
                            const LocatorType& locator,
                            vtkm::cont::Token& token)
  {
    auto execObject = locator.PrepareForExecution(device, token);
    using WrapType = CellLocatorBaseExecWrapper<decltype(execObject)>;
    execHandle.Reset(new WrapType(execObject));
    return true;
  }
};

template <typename Derived>
class VTKM_ALWAYS_EXPORT CellLocatorBaseWrapper : public vtkm::cont::CellLocator
{
  Derived Locator;
  mutable vtkm::cont::VirtualObjectHandle<vtkm::exec::CellLocator> ExecutionObjectHandle;

public:
  CellLocatorBaseWrapper() = default;

  CellLocatorBaseWrapper(const Derived& locator)
    : Locator(locator)
  {
    this->SetCellSet(locator.GetCellSet());
    this->SetCoordinates(locator.GetCoordinates());
  }

  VTKM_CONT const vtkm::exec::CellLocator* PrepareForExecution(
    vtkm::cont::DeviceAdapterId device,
    vtkm::cont::Token& token) const override
  {
    bool success =
      vtkm::cont::TryExecuteOnDevice(device,
                                     CellLocatorBaseWrapperPrepareForExecutionFunctor<Derived>{},
                                     this->ExecutionObjectHandle,
                                     this->Locator,
                                     token);
    if (!success)
    {
      throwFailedRuntimeDeviceTransfer("CellLocatorUniformGrid", device);
    }
    return this->ExecutionObjectHandle.PrepareForExecution(device, token);
  }

private:
  void Build() override
  {
    this->Locator.SetCellSet(this->GetCellSet());
    this->Locator.SetCoordinates(this->GetCoordinates());
    this->Locator.Update();
  }
};

VTKM_DEPRECATED_SUPPRESS_END
#endif //!VTKM_NO_DEPRECATED_VIRTUAL


} // namespace detail

/// \brief Base class for all `CellLocator` classes.
///
/// `CellLocatorBase` uses the curiously recurring template pattern (CRTP). Subclasses
/// must provide their own type for the template parameter. Subclasses must implement
/// `Update` and `PrepareForExecution` methods.
///
template <typename Derived>
class VTKM_ALWAYS_EXPORT CellLocatorBase : public vtkm::cont::ExecutionObjectBase
{
  vtkm::cont::DynamicCellSet CellSet;
  vtkm::cont::CoordinateSystem Coords;
  mutable bool Modified = true;

public:
#ifndef VTKM_NO_DEPRECATED_VIRTUAL
  VTKM_DEPRECATED_SUPPRESS_BEGIN
  // Support deprecated classes
  operator detail::CellLocatorBaseWrapper<Derived>() const
  {
    return detail::CellLocatorBaseWrapper<Derived>(reinterpret_cast<const Derived&>(*this));
  }
  VTKM_DEPRECATED_SUPPRESS_END
#endif //!VTKM_NO_DEPRECATED_VIRTUAL

  const vtkm::cont::DynamicCellSet& GetCellSet() const { return this->CellSet; }

  void SetCellSet(const vtkm::cont::DynamicCellSet& cellSet)
  {
    this->CellSet = cellSet;
    this->SetModified();
  }

  const vtkm::cont::CoordinateSystem& GetCoordinates() const { return this->Coords; }

  void SetCoordinates(const vtkm::cont::CoordinateSystem& coords)
  {
    this->Coords = coords;
    this->SetModified();
  }

  void Update() const
  {
    if (this->Modified)
    {
      static_cast<Derived*>(const_cast<CellLocatorBase*>(this))->Build();
      this->Modified = false;
    }
  }

  template <typename Device>
  VTKM_CONT VTKM_DEPRECATED(1.6, "PrepareForExecution now requires a vtkm::cont::Token object.")
    vtkm::cont::internal::ExecutionObjectType<Derived, Device> PrepareForExecution(
      Device device) const
  {
    vtkm::cont::Token token;
    return this->PrepareForExecution(device, token);
  }

protected:
  void SetModified() { this->Modified = true; }
  bool GetModified() const { return this->Modified; }
};

}
}
} // vtkm::cont::internal

#endif //vtk_m_cont_internal_CellLocatorBase_h
