//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
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

namespace vtkm {
namespace worklet {

/// \brief Dispatcher for worklets that inherit from \c WorkletMapField.
///
template<typename WorkletType,
         typename Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class DispatcherMapField :
    public vtkm::worklet::internal::DispatcherBase<
      DispatcherMapField<WorkletType,Device>,
      WorkletType,
      vtkm::worklet::WorkletMapField,
      Device>
{
  typedef vtkm::worklet::internal::DispatcherBase<
    DispatcherMapField<WorkletType,Device>,
    WorkletType,
    vtkm::worklet::WorkletMapField,
    Device> Superclass;

public:
  VTKM_CONT_EXPORT
  DispatcherMapField(const WorkletType &worklet = WorkletType())
    : Superclass(worklet) { this->Use3DSchedule=false;  }

  template<typename Invocation>
  VTKM_CONT_EXPORT
  void DoInvoke(const Invocation &invocation) const
  {
    // The parameter for the input domain is stored in the Invocation. (It is
    // also in the worklet, but it is safer to get it from the Invocation
    // in case some other dispatch operation had to modify it.)
    static const vtkm::IdComponent InputDomainIndex =
        Invocation::InputDomainIndex;

    // ParameterInterface (from Invocation) is a FunctionInterface type
    // containing types for all objects passed to the Invoke method (with
    // some dynamic casting performed so objects like DynamicArrayHandle get
    // cast to ArrayHandle).
    typedef typename Invocation::ParameterInterface ParameterInterface;

    // This is the type for the input domain (derived from the last two things
    // we got from the Invocation).
    typedef typename ParameterInterface::
        template ParameterType<InputDomainIndex>::type InputDomainType;

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    InputDomainType inputDomain =
        invocation.Parameters.template GetParameter<InputDomainIndex>();

    // For a DispatcherMapField, the inputDomain must be an ArrayHandle (or
    // a DynamicArrayHandle that gets cast to one). The size of the domain
    // (number of threads/worklet instances) is equal to the size of the
    // array.
    vtkm::Id numInstances = inputDomain.GetNumberOfValues();

    // A MapField is a pretty straightforward dispatch. Once we know the number
    // of invocations, the superclass can take care of the rest.
    this->BasicInvoke(invocation, numInstances);
  }

};

}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_Dispatcher_MapField_h
