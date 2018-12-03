//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2018 UT-Battelle, LLC.
//  Copyright 2018 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

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
  using ControlSignature = void(FieldIn<IdType> index, WholeArrayInOut<IdType> comp);
  using ExecutionSignature = void(_1, _2);
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
  using ControlSignature = void(FieldIn<IdType> index, WholeArrayIn<IdType> comp, FieldOut<>);
  using ExecutionSignature = _3(_1, _2);
  using InputDomain = _1;

  template <typename InOutPortalType>
  VTKM_EXEC bool operator()(vtkm::Id index, InOutPortalType& comp) const
  {
    return comp.Get(index) == comp.Get(comp.Get(index));
  }
};

} // connectivity
} // worklet
} // vtkm
#endif // vtk_m_worklet_connectivity_union_find_h
