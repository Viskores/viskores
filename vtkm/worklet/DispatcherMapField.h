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
#ifndef vtk_m_worklet_Dispatcher_MapField_h
#define vtk_m_worklet_Dispatcher_MapField_h

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm
{
namespace worklet
{

/// \brief Dispatcher for worklets that inherit from \c WorkletMapField.
///
template <typename WorkletType, typename Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class DispatcherMapField
  : public vtkm::worklet::internal::DispatcherBase<DispatcherMapField<WorkletType, Device>,
                                                   WorkletType,
                                                   vtkm::worklet::WorkletMapField>
{
  typedef vtkm::worklet::internal::DispatcherBase<DispatcherMapField<WorkletType, Device>,
                                                  WorkletType,
                                                  vtkm::worklet::WorkletMapField>
    Superclass;

public:
  VTKM_CONT
  DispatcherMapField(const WorkletType& worklet = WorkletType())
    : Superclass(worklet)
  {
  }

  template <typename Invocation>
  VTKM_CONT void DoInvoke(const Invocation& invocation) const
  {
    // This is the type for the input domain
    typedef typename Invocation::InputDomainType InputDomainType;

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();

    // For a DispatcherMapField, the inputDomain must be an ArrayHandle (or
    // a DynamicArrayHandle that gets cast to one). The size of the domain
    // (number of threads/worklet instances) is equal to the size of the
    // array.
    vtkm::Id numInstances = inputDomain.GetNumberOfValues();

    // A MapField is a pretty straightforward dispatch. Once we know the number
    // of invocations, the superclass can take care of the rest.
    this->BasicInvoke(invocation, numInstances, Device());
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_Dispatcher_MapField_h
