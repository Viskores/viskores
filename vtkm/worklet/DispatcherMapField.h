//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_Dispatcher_MapField_h
#define vtk_m_worklet_Dispatcher_MapField_h

#include <vtkm/worklet/WorkletMapField.h>
#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm
{
namespace worklet
{

/// \brief Dispatcher for worklets that inherit from \c WorkletMapField.
///
template <typename WorkletType>
class DispatcherMapField
  : public vtkm::worklet::internal::DispatcherBase<DispatcherMapField<WorkletType>,
                                                   WorkletType,
                                                   vtkm::worklet::WorkletMapField>
{
  using Superclass = vtkm::worklet::internal::DispatcherBase<DispatcherMapField<WorkletType>,
                                                             WorkletType,
                                                             vtkm::worklet::WorkletMapField>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VTKM_CONT DispatcherMapField(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename Invocation>
  VTKM_CONT void DoInvoke(Invocation& invocation) const
  {
    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();

    // For a DispatcherMapField, the inputDomain must be an ArrayHandle (or
    // an VariantArrayHandle that gets cast to one). The size of the domain
    // (number of threads/worklet instances) is equal to the size of the
    // array.
    auto numInstances = internal::scheduling_range(inputDomain);

    // A MapField is a pretty straightforward dispatch. Once we know the number
    // of invocations, the superclass can take care of the rest.
    this->BasicInvoke(invocation, numInstances);
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_Dispatcher_MapField_h
