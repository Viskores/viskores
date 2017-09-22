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
template <typename WorkletType, typename Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class DispatcherMapTopology
  : public vtkm::worklet::internal::DispatcherBase<DispatcherMapTopology<WorkletType, Device>,
                                                   WorkletType,
                                                   vtkm::worklet::detail::WorkletMapTopologyBase>
{
  typedef vtkm::worklet::internal::DispatcherBase<DispatcherMapTopology<WorkletType, Device>,
                                                  WorkletType,
                                                  vtkm::worklet::detail::WorkletMapTopologyBase>
    Superclass;

public:
  VTKM_CONT
  DispatcherMapTopology(const WorkletType& worklet = WorkletType())
    : Superclass(worklet)
  {
  }

  template <typename Invocation>
  VTKM_CONT void DoInvoke(const Invocation& invocation) const
  {
    // This is the type for the input domain
    typedef typename Invocation::InputDomainType InputDomainType;

    // If you get a compile error on this line, then you have tried to use
    // something that is not a vtkm::cont::CellSet as the input domain to a
    // topology operation (that operates on a cell set connection domain).
    VTKM_IS_CELL_SET(InputDomainType);

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();

    // Now that we have the input domain, we can extract the range of the
    // scheduling and call BadicInvoke.
    this->BasicInvoke(
      invocation, inputDomain.GetSchedulingRange(typename WorkletType::ToTopologyType()), Device());
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_Dispatcher_MapTopology_h
