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

#ifndef vtk_m_cont_BoundingIntervalHierarchy_h
#define vtk_m_cont_BoundingIntervalHierarchy_h

#include <vtkm/Types.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleTransform.h>
#include <vtkm/cont/BoundingIntervalHierarchyNode.h>
#include <vtkm/cont/CellLocator.h>
#include <vtkm/worklet/spatialstructure/BoundingIntervalHierarchy.h>

namespace vtkm
{
namespace cont
{

class BoundingIntervalHierarchy : public vtkm::cont::CellLocator
{
private:
  using IdArrayHandle = vtkm::cont::ArrayHandle<vtkm::Id>;
  using CoordsArrayHandle = vtkm::cont::ArrayHandle<vtkm::FloatDefault>;
  using RangeArrayHandle = vtkm::cont::ArrayHandle<vtkm::Range>;
  using RangePermutationArrayHandle =
    vtkm::cont::ArrayHandlePermutation<IdArrayHandle, RangeArrayHandle>;
  using SplitPropertiesArrayHandle =
    vtkm::cont::ArrayHandle<vtkm::worklet::spatialstructure::SplitProperties>;
  using HandleType = vtkm::cont::VirtualObjectHandle<vtkm::exec::CellLocator>;

  class BuildFunctor;
  class PrepareForExecutionFunctor;

  template <typename DeviceAdapter>
  VTKM_CONT IdArrayHandle CalculateSegmentSizes(const IdArrayHandle&, vtkm::Id);

  template <typename DeviceAdapter>
  VTKM_CONT IdArrayHandle GenerateSegmentIds(const IdArrayHandle&, vtkm::Id);

  template <typename DeviceAdapter>
  VTKM_CONT void CalculateSplitCosts(RangePermutationArrayHandle&,
                                     RangeArrayHandle&,
                                     CoordsArrayHandle&,
                                     IdArrayHandle&,
                                     SplitPropertiesArrayHandle&,
                                     DeviceAdapter);

  template <typename DeviceAdapter>
  VTKM_CONT void CalculatePlaneSplitCost(vtkm::IdComponent,
                                         vtkm::IdComponent,
                                         RangePermutationArrayHandle&,
                                         RangeArrayHandle&,
                                         CoordsArrayHandle&,
                                         IdArrayHandle&,
                                         SplitPropertiesArrayHandle&,
                                         vtkm::IdComponent,
                                         DeviceAdapter);

  template <typename DeviceAdapter>
  VTKM_CONT IdArrayHandle CalculateSplitScatterIndices(const IdArrayHandle&,
                                                       const IdArrayHandle&,
                                                       const IdArrayHandle&,
                                                       DeviceAdapter);

public:
  VTKM_CONT
  BoundingIntervalHierarchy(vtkm::IdComponent numPlanes = 4, vtkm::IdComponent maxLeafSize = 5)
    : NumPlanes(numPlanes)
    , MaxLeafSize(maxLeafSize)
    , Nodes()
    , ProcessedCellIds()
  {
  }

  VTKM_CONT
  void SetNumberOfSplittingPlanes(vtkm::IdComponent numPlanes)
  {
    NumPlanes = numPlanes;
    SetDirty();
  }

  VTKM_CONT
  vtkm::IdComponent GetNumberOfSplittingPlanes() { return NumPlanes; }

  VTKM_CONT
  void SetMaxLeafSize(vtkm::IdComponent maxLeafSize)
  {
    MaxLeafSize = maxLeafSize;
    SetDirty();
  }

  VTKM_CONT
  vtkm::Id GetMaxLeafSize() { return MaxLeafSize; }

protected:
  VTKM_CONT
  void Build() override;

  VTKM_CONT
  virtual const HandleType PrepareForExecutionImpl(
    const vtkm::cont::DeviceAdapterId device) const override;

private:
  vtkm::IdComponent NumPlanes;
  vtkm::IdComponent MaxLeafSize;
  vtkm::cont::ArrayHandle<BoundingIntervalHierarchyNode> Nodes;
  IdArrayHandle ProcessedCellIds;
  mutable HandleType ExecHandle;
};

} // namespace cont
} // namespace vtkm

#endif // vtk_m_cont_BoundingIntervalHierarchy_h
