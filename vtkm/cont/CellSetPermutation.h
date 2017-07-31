//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 Sandia Corporation.
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_cont_CellSetPermutation_h
#define vtk_m_cont_CellSetPermutation_h

#include <vtkm/CellShape.h>
#include <vtkm/CellTraits.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSet.h>

#include <vtkm/exec/ConnectivityPermuted.h>

namespace vtkm
{
namespace cont
{

template <typename OriginalCellSet, typename ValidCellArrayHandleType>
class CellSetPermutation;

namespace internal
{

template <typename OriginalCellSet, typename PermutationArrayHandleType>
class CellSetGeneralPermutation : public CellSet
{
public:
  VTKM_CONT
  CellSetGeneralPermutation(const PermutationArrayHandleType& validCellIds,
                            const OriginalCellSet& cellset,
                            const std::string& name)
    : CellSet(name)
    , ValidCellIds(validCellIds)
    , FullCellSet(cellset)
  {
  }

  VTKM_CONT
  CellSetGeneralPermutation(const std::string& name)
    : CellSet(name)
    , ValidCellIds()
    , FullCellSet()
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfCells() const VTKM_OVERRIDE { return this->ValidCellIds.GetNumberOfValues(); }

  VTKM_CONT
  vtkm::Id GetNumberOfPoints() const VTKM_OVERRIDE { return this->FullCellSet.GetNumberOfPoints(); }

  vtkm::Id GetNumberOfFaces() const VTKM_OVERRIDE { return -1; }

  vtkm::Id GetNumberOfEdges() const VTKM_OVERRIDE { return -1; }

  //This is the way you can fill the memory from another system without copying
  VTKM_CONT
  void Fill(const PermutationArrayHandleType& validCellIds, const OriginalCellSet& cellset)
  {
    ValidCellIds = validCellIds;
    FullCellSet = cellset;
  }

  template <typename TopologyElement>
  VTKM_CONT vtkm::Id GetSchedulingRange(TopologyElement) const
  {
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(TopologyElement);
    return this->ValidCellIds.GetNumberOfValues();
  }

  template <typename Device, typename FromTopology, typename ToTopology>
  struct ExecutionTypes
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);

    typedef typename PermutationArrayHandleType::template ExecutionTypes<Device>::PortalConst
      ExecPortalType;

    typedef typename OriginalCellSet::template ExecutionTypes<Device, FromTopology, ToTopology>::
      ExecObjectType OrigExecObjectType;

    typedef vtkm::exec::ConnectivityPermuted<ExecPortalType, OrigExecObjectType> ExecObjectType;
  };

  template <typename Device, typename FromTopology, typename ToTopology>
  typename ExecutionTypes<Device, FromTopology, ToTopology>::ExecObjectType
    PrepareForInput(Device, FromTopology, ToTopology) const
  {
    // Developer's note: I looked into supporting cell to point connectivity in a permutation cell
    // set and found it to be complex. It is not straightforward to implement this on top of the
    // original cell set's cell to point because points could be attached to cells that do not
    // exist in the permuted topology. Ultimately, you will probably have to rebuild these
    // connections in a way very similar to how CellSetExplicit already does it. In fact, the
    // easiest implementation will probably be to just convert to a CellSetExplicit and use that.
    // In fact, it may be possible to change this whole implementation to just be a subclass of
    // CellSetExplicit with some fancy arrays for its point to cell arrays.
    throw vtkm::cont::ErrorBadType(
      "CellSetPermutation currently only supports point to cell connectivity. "
      "To support other connectivity, convert to an explicit grid with the CellDeepCopy "
      "worklet or the CleanGrid filter.");
  }

  template <typename Device>
  typename ExecutionTypes<Device,
                          vtkm::TopologyElementTagPoint,
                          vtkm::TopologyElementTagCell>::ExecObjectType
  PrepareForInput(Device d, vtkm::TopologyElementTagPoint f, vtkm::TopologyElementTagCell t) const
  {
    using FromTopology = vtkm::TopologyElementTagPoint;
    using ToTopology = vtkm::TopologyElementTagCell;
    using ConnectivityType =
      typename ExecutionTypes<Device, FromTopology, ToTopology>::ExecObjectType;
    return ConnectivityType(this->ValidCellIds.PrepareForInput(d),
                            this->FullCellSet.PrepareForInput(d, f, t));
  }

  void PrintSummary(std::ostream& out) const VTKM_OVERRIDE
  {
    out << "   CellSetGeneralPermutation of: " << std::endl;
    this->FullCellSet.PrintSummary(out);
  }

private:
  PermutationArrayHandleType ValidCellIds;
  OriginalCellSet FullCellSet;
};

} //namespace internal

#ifndef VTKM_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG
#define VTKM_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

template <typename OriginalCellSet,
          typename PermutationArrayHandleType =
            vtkm::cont::ArrayHandle<vtkm::Id, VTKM_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG>>
class CellSetPermutation
  : public vtkm::cont::internal::CellSetGeneralPermutation<OriginalCellSet,
                                                           PermutationArrayHandleType>
{
  VTKM_IS_CELL_SET(OriginalCellSet);
  VTKM_IS_ARRAY_HANDLE(PermutationArrayHandleType);

  typedef typename vtkm::cont::internal::CellSetGeneralPermutation<OriginalCellSet,
                                                                   PermutationArrayHandleType>
    ParentType;

public:
  VTKM_CONT
  CellSetPermutation(const PermutationArrayHandleType& validCellIds,
                     const OriginalCellSet& cellset,
                     const std::string& name = std::string())
    : ParentType(validCellIds, cellset, name)
  {
  }

  VTKM_CONT
  CellSetPermutation(const std::string& name = std::string())
    : ParentType(name)
  {
  }

  VTKM_CONT
  CellSetPermutation<OriginalCellSet, PermutationArrayHandleType>& operator=(
    const CellSetPermutation<OriginalCellSet, PermutationArrayHandleType>& src)
  {
    ParentType::operator=(src);
    return *this;
  }
};

template <typename OriginalCellSet, typename PermutationArrayHandleType>
vtkm::cont::CellSetPermutation<OriginalCellSet, PermutationArrayHandleType> make_CellSetPermutation(
  const PermutationArrayHandleType& cellIndexMap,
  const OriginalCellSet& cellSet,
  const std::string& name)
{
  VTKM_IS_CELL_SET(OriginalCellSet);
  VTKM_IS_ARRAY_HANDLE(PermutationArrayHandleType);

  return vtkm::cont::CellSetPermutation<OriginalCellSet, PermutationArrayHandleType>(
    cellIndexMap, cellSet, name);
}

template <typename OriginalCellSet, typename PermutationArrayHandleType>
vtkm::cont::CellSetPermutation<OriginalCellSet, PermutationArrayHandleType> make_CellSetPermutation(
  const PermutationArrayHandleType& cellIndexMap,
  const OriginalCellSet& cellSet)
{
  VTKM_IS_CELL_SET(OriginalCellSet);
  VTKM_IS_ARRAY_HANDLE(PermutationArrayHandleType);

  return vtkm::cont::make_CellSetPermutation(cellIndexMap, cellSet, cellSet.GetName());
}
}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetPermutation_h
