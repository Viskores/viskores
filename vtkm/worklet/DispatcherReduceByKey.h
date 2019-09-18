//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_DispatcherReduceByKey_h
#define vtk_m_worklet_DispatcherReduceByKey_h

#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/worklet/WorkletReduceByKey.h>

#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm
{
namespace worklet
{

/// \brief Dispatcher for worklets that inherit from \c WorkletReduceByKey.
///
template <typename WorkletType>
class DispatcherReduceByKey
  : public vtkm::worklet::internal::DispatcherBase<DispatcherReduceByKey<WorkletType>,
                                                   WorkletType,
                                                   vtkm::worklet::WorkletReduceByKey>
{
  using Superclass = vtkm::worklet::internal::DispatcherBase<DispatcherReduceByKey<WorkletType>,
                                                             WorkletType,
                                                             vtkm::worklet::WorkletReduceByKey>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VTKM_CONT DispatcherReduceByKey(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename Invocation>
  void DoInvoke(Invocation& invocation) const
  {
    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;

    // If you get a compile error on this line, then you have tried to use
    // something other than vtkm::worklet::Keys as the input domain, which
    // is illegal.
    VTKM_STATIC_ASSERT_MSG(
      (vtkm::cont::arg::TypeCheck<vtkm::cont::arg::TypeCheckTagKeys, InputDomainType>::value),
      "Invalid input domain for WorkletReduceByKey.");

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();

    // Now that we have the input domain, we can extract the range of the
    // scheduling and call BasicInvoke.
    this->BasicInvoke(invocation, internal::scheduling_range(inputDomain));
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_DispatcherReduceByKey_h
