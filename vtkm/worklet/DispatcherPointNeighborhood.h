//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_DispatcherPointNeighborhood_h
#define vtk_m_worklet_DispatcherPointNeighborhood_h

#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/worklet/WorkletPointNeighborhood.h>

#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm
{
namespace worklet
{

/// \brief Dispatcher for worklets that inherit from \c WorkletPointNeighborhood.
///
template <typename WorkletType>
class DispatcherPointNeighborhood
  : public vtkm::worklet::internal::DispatcherBase<DispatcherPointNeighborhood<WorkletType>,
                                                   WorkletType,
                                                   vtkm::worklet::WorkletPointNeighborhoodBase>
{
  using Superclass =
    vtkm::worklet::internal::DispatcherBase<DispatcherPointNeighborhood<WorkletType>,
                                            WorkletType,
                                            vtkm::worklet::WorkletPointNeighborhoodBase>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VTKM_CONT DispatcherPointNeighborhood(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename Invocation>
  void DoInvoke(Invocation& invocation) const
  {
    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;

    // If you get a compile error on this line, then you have tried to use
    // something that is not a vtkm::cont::CellSet as the input domain to a
    // topology operation (that operates on a cell set connection domain).
    VTKM_IS_CELL_SET(InputDomainType);

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();
    auto inputRange = internal::scheduling_range(inputDomain, vtkm::TopologyElementTagPoint{});

    // This is pretty straightforward dispatch. Once we know the number
    // of invocations, the superclass can take care of the rest.
    this->BasicInvoke(invocation, inputRange);
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_DispatcherPointNeighborhood_h
