//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_Dispatcher_MapTopology_h
#define vtk_m_worklet_Dispatcher_MapTopology_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm
{
namespace worklet
{

/// \brief Dispatcher for worklets that inherit from \c WorkletMapTopology.
///
template <typename WorkletType>
class DispatcherMapTopology
  : public vtkm::worklet::internal::DispatcherBase<DispatcherMapTopology<WorkletType>,
                                                   WorkletType,
                                                   vtkm::worklet::detail::WorkletMapTopologyBase>
{
  using Superclass =
    vtkm::worklet::internal::DispatcherBase<DispatcherMapTopology<WorkletType>,
                                            WorkletType,
                                            vtkm::worklet::detail::WorkletMapTopologyBase>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VTKM_CONT DispatcherMapTopology(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename Invocation>
  VTKM_CONT void DoInvoke(Invocation& invocation) const
  {
    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;
    using SchedulingRangeType = typename WorkletType::VisitTopologyType;

    // If you get a compile error on this line, then you have tried to use
    // something that is not a vtkm::cont::CellSet as the input domain to a
    // topology operation (that operates on a cell set connection domain).
    VTKM_IS_CELL_SET(InputDomainType);

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const auto& inputDomain = invocation.GetInputDomain();

    // Now that we have the input domain, we can extract the range of the
    // scheduling and call BadicInvoke.
    this->BasicInvoke(invocation, internal::scheduling_range(inputDomain, SchedulingRangeType{}));
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_Dispatcher_MapTopology_h
