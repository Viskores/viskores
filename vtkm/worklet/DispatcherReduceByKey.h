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
template <typename WorkletType, typename Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class DispatcherReduceByKey
  : public vtkm::worklet::internal::DispatcherBase<DispatcherReduceByKey<WorkletType, Device>,
                                                   WorkletType,
                                                   vtkm::worklet::WorkletReduceByKey>
{
  using Superclass =
    vtkm::worklet::internal::DispatcherBase<DispatcherReduceByKey<WorkletType, Device>,
                                            WorkletType,
                                            vtkm::worklet::WorkletReduceByKey>;
  using ScatterType = typename Superclass::ScatterType;

public:
  // If you get a compile error here about there being no appropriate constructor for ScatterType,
  // then that probably means that the worklet you are trying to execute has defined a custom
  // ScatterType and that you need to create one (because there is no default way to construct
  // the scatter). By convention, worklets that define a custom scatter type usually provide a
  // static method named MakeScatter that constructs a scatter object.
  VTKM_CONT
  DispatcherReduceByKey(const WorkletType& worklet = WorkletType(),
                        const ScatterType& scatter = ScatterType())
    : Superclass(worklet, scatter)
  {
  }

  VTKM_CONT
  DispatcherReduceByKey(const ScatterType& scatter)
    : Superclass(WorkletType(), scatter)
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
    // scheduling and call BadicInvoke.
    this->BasicInvoke(invocation, inputDomain.GetInputRange(), Device());
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_DispatcherReduceByKey_h
