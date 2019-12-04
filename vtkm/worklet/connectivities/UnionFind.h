//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef vtk_m_worklet_connectivity_union_find_h
#define vtk_m_worklet_connectivity_union_find_h

#include <vtkm/worklet/WorkletMapField.h>

namespace vtkm
{
namespace worklet
{
namespace connectivity
{

class PointerJumping : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayInOut comp);
  using ExecutionSignature = void(WorkIndex, _1);
  using InputDomain = _1;

  template <typename Comp>
  VTKM_EXEC vtkm::Id findRoot(Comp& comp, vtkm::Id index) const
  {
    while (comp.Get(index) != index)
      index = comp.Get(index);
    return index;
  }

  template <typename InOutPortalType>
  VTKM_EXEC void operator()(vtkm::Id index, InOutPortalType& comp) const
  {
    // TODO: is there a data race between findRoot and comp.Set?
    auto root = findRoot(comp, index);
    comp.Set(index, root);
  }
};

class IsStar : public vtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(WholeArrayIn comp, AtomicArrayInOut);
  using ExecutionSignature = void(WorkIndex, _1, _2);
  using InputDomain = _1;

  template <typename InOutPortalType, typename AtomicInOut>
  VTKM_EXEC void operator()(vtkm::Id index, InOutPortalType& comp, AtomicInOut& hasStar) const
  {
    //hasStar emulates a LogicalAnd across all the values
    //where we start with a value of 'true'|1.
    const bool isAStar = (comp.Get(index) == comp.Get(comp.Get(index)));
    if (!isAStar && hasStar.Get(0) == 1)
    {
      hasStar.Set(0, 0);
    }
  }
};

} // connectivity
} // worklet
} // vtkm
#endif // vtk_m_worklet_connectivity_union_find_h
