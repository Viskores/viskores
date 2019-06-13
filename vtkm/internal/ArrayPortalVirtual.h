//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_internal_ArrayPortalVirtual_h
#define vtk_m_internal_ArrayPortalVirtual_h


#include <vtkm/VecTraits.h>
#include <vtkm/VirtualObjectBase.h>

#include <vtkm/internal/ArrayPortalHelpers.h>
#include <vtkm/internal/ExportMacros.h>

namespace vtkm
{
namespace internal
{

class VTKM_ALWAYS_EXPORT PortalVirtualBase
{
public:
  VTKM_EXEC_CONT PortalVirtualBase() noexcept {}

  VTKM_EXEC_CONT virtual ~PortalVirtualBase() noexcept {
    //we implement this as we need a destructor with cuda markup.
    //Using =default causes cuda free errors inside VirtualObjectTransferCuda
  };
};

} // namespace internal

template <typename T>
class VTKM_ALWAYS_EXPORT ArrayPortalVirtual : public internal::PortalVirtualBase
{
public:
  using ValueType = T;

  //use parents constructor
  using PortalVirtualBase::PortalVirtualBase;

  VTKM_EXEC_CONT virtual ~ArrayPortalVirtual<T>(){};

  VTKM_EXEC_CONT virtual T Get(vtkm::Id index) const noexcept = 0;

  VTKM_EXEC_CONT virtual void Set(vtkm::Id, const T&) const noexcept {}
};


template <typename PortalT>
class VTKM_ALWAYS_EXPORT ArrayPortalWrapper final
  : public vtkm::ArrayPortalVirtual<typename PortalT::ValueType>
{
  using T = typename PortalT::ValueType;

public:
  ArrayPortalWrapper(const PortalT& p) noexcept : ArrayPortalVirtual<T>(), Portal(p) {}

  VTKM_EXEC
  T Get(vtkm::Id index) const noexcept
  {
    using call_supported_t = typename internal::PortalSupportsGets<PortalT>::type;
    return this->Get(call_supported_t(), index);
  }

  VTKM_EXEC
  void Set(vtkm::Id index, const T& value) const noexcept
  {
    using call_supported_t = typename internal::PortalSupportsSets<PortalT>::type;
    this->Set(call_supported_t(), index, value);
  }

private:
  // clang-format off
  VTKM_EXEC inline T Get(std::true_type, vtkm::Id index) const noexcept { return this->Portal.Get(index); }
  VTKM_EXEC inline T Get(std::false_type, vtkm::Id) const noexcept { return T{}; }
  VTKM_EXEC inline void Set(std::true_type, vtkm::Id index, const T& value) const noexcept { this->Portal.Set(index, value); }
  VTKM_EXEC inline void Set(std::false_type, vtkm::Id, const T&) const noexcept {}
  // clang-format on


  PortalT Portal;
};


template <typename T>
class VTKM_ALWAYS_EXPORT ArrayPortalRef
{
public:
  using ValueType = T;

  ArrayPortalRef() noexcept : Portal(nullptr), NumberOfValues(0) {}

  ArrayPortalRef(const ArrayPortalVirtual<T>* portal, vtkm::Id numValues) noexcept
    : Portal(portal),
      NumberOfValues(numValues)
  {
  }

  //Currently this needs to be valid on both the host and device for cuda, so we can't
  //call the underlying portal as that uses device virtuals and the method will fail.
  //We need to seriously look at the interaction of portals and iterators for device
  //adapters and determine a better approach as iterators<Portal> are really fat
  VTKM_EXEC_CONT inline vtkm::Id GetNumberOfValues() const noexcept { return this->NumberOfValues; }

  //This isn't valid on the host for cuda
  VTKM_EXEC_CONT inline T Get(vtkm::Id index) const noexcept { return this->Portal->Get(index); }

  //This isn't valid on the host for
  VTKM_EXEC_CONT inline void Set(vtkm::Id index, const T& t) const noexcept
  {
    this->Portal->Set(index, t);
  }

  const ArrayPortalVirtual<T>* Portal;
  vtkm::Id NumberOfValues;
};

template <typename T>
inline ArrayPortalRef<T> make_ArrayPortalRef(const ArrayPortalVirtual<T>* portal,
                                             vtkm::Id numValues) noexcept
{
  return ArrayPortalRef<T>(portal, numValues);
}


} // namespace vtkm

#endif
