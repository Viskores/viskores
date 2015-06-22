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
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_Dispatcher_MapTopology_h
#define vtk_m_worklet_Dispatcher_MapTopology_h

#include <vtkm/RegularConnectivity.h>

#include <vtkm/cont/DeviceAdapter.h>
#include <vtkm/cont/ExplicitConnectivity.h>
#include <vtkm/worklet/WorkletMapTopology.h>
#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm {
namespace worklet {

/// \brief Dispatcher for worklets that inherit from \c WorkletMapTopology.
///
template<typename WorkletType,
         typename Device = VTKM_DEFAULT_DEVICE_ADAPTER_TAG>
class DispatcherMapTopology :
    public vtkm::worklet::internal::DispatcherBase<
      DispatcherMapTopology<WorkletType,Device>,
      WorkletType,
      vtkm::worklet::WorkletMapTopology,
      Device>
{
  typedef vtkm::worklet::internal::DispatcherBase<
    DispatcherMapTopology<WorkletType,Device>,
    WorkletType,
    vtkm::worklet::WorkletMapTopology,
    Device> Superclass;

public:
  VTKM_CONT_EXPORT
  DispatcherMapTopology(const WorkletType &worklet = WorkletType())
    : Superclass(worklet) {  }

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

    //we need to now template based on the input domain type. If the input
    //domain type is a regular or explicit grid we call GetSchedulingDimensions.
    //but in theory your input domain could be a permutation array
    this->InvokeBasedOnDomainType(invocation,inputDomain);
  }

  template<typename Invocation, typename InputDomainType>
  VTKM_CONT_EXPORT
  void InvokeBasedOnDomainType(const Invocation &invocation,
                               const InputDomainType& domain) const
  {
    //presume that the input domain isn't a grid, so call GetNumberOfValues()
    //this code path is currently not exercised as the InputDomain currently
    //is required to be Explicit or Regular Connectivity. In the future if
    //we ever allow the InputDomain and the TopologyDomain to differ, this
    //invocation will be used
    this->BasicInvoke(invocation, domain.GetNumberOfValues());
  }

  template<typename Invocation,
           typename T,
           typename U,
           typename V>
  VTKM_CONT_EXPORT
  void InvokeBasedOnDomainType(const Invocation &invocation,
                               const vtkm::cont::ExplicitConnectivity<T,U,V>& domain) const
  {

    // For a DispatcherMapTopology, when the inputDomain is some for of
    // explicit connectivity we call GetSchedulingDimensions which will return
    // a linear value representing the number of cells to schedule
    this->BasicInvoke(invocation, domain.GetSchedulingDimensions());
  }

  template<typename Invocation,
           vtkm::cont::TopologyType From,
           vtkm::cont::TopologyType To,
           vtkm::IdComponent Domain>
  VTKM_CONT_EXPORT
  void InvokeBasedOnDomainType(const Invocation &invocation,
                               const vtkm::RegularConnectivity<From,To,Domain>& domain) const
  {

    // For a DispatcherMapTopology, the inputDomain is some for of connectivity
    // so the GetSchedulingDimensions can return a vtkm::Id for linear scheduling,
    // or a vtkm::Id2 or vtkm::Id3 for 3d block scheduling
    this->BasicInvoke(invocation, domain.GetSchedulingDimensions());
  }
};

}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_Dispatcher_MapTopology_h
