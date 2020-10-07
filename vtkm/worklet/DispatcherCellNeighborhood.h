//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_DispatcherCellNeighborhood_h
#define vtk_m_worklet_DispatcherCellNeighborhood_h

#include <vtkm/cont/DeviceAdapter.h>

#include <vtkm/worklet/internal/DispatcherBase.h>

namespace vtkm
{
namespace worklet
{
class WorkletNeighborhood;
class WorkletCellNeighborhood;

/// \brief Dispatcher for worklets that inherit from \c WorkletCellNeighborhood.
///
template <typename WorkletType>
class DispatcherCellNeighborhood
  : public vtkm::worklet::internal::DispatcherBase<DispatcherCellNeighborhood<WorkletType>,
                                                   WorkletType,
                                                   vtkm::worklet::WorkletNeighborhood>
{
  using Superclass =
    vtkm::worklet::internal::DispatcherBase<DispatcherCellNeighborhood<WorkletType>,
                                            WorkletType,
                                            vtkm::worklet::WorkletNeighborhood>;
  using ScatterType = typename Superclass::ScatterType;

public:
  template <typename... T>
  VTKM_CONT DispatcherCellNeighborhood(T&&... args)
    : Superclass(std::forward<T>(args)...)
  {
  }

  template <typename Invocation>
  void DoInvoke(Invocation& invocation) const
  {
    using namespace vtkm::worklet::internal;

    // This is the type for the input domain
    using InputDomainType = typename Invocation::InputDomainType;

    // If you get a compile error on this line, then you have tried to use
    // something that is not a vtkm::cont::CellSet as the input domain to a
    // topology operation (that operates on a cell set connection domain).
    VTKM_IS_CELL_SET(InputDomainType);

    // We can pull the input domain parameter (the data specifying the input
    // domain) from the invocation object.
    const InputDomainType& inputDomain = invocation.GetInputDomain();
    auto inputRange = SchedulingRange(inputDomain, vtkm::TopologyElementTagCell{});

    // This is pretty straightforward dispatch. Once we know the number
    // of invocations, the superclass can take care of the rest.
    this->BasicInvoke(invocation, inputRange);
  }
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_DispatcherCellNeighborhood_h
