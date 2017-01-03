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

namespace vtkm {
namespace cont {

template< typename OriginalCellSet, typename ValidCellArrayHandleType> class CellSetPermutation;

namespace internal {

template< typename OriginalCellSet, typename PermutationArrayHandleType >
class CellSetGeneralPermutation : public CellSet
{
public:
  VTKM_CONT
  CellSetGeneralPermutation(const PermutationArrayHandleType& validCellIds,
                            const OriginalCellSet& cellset,
                            const std::string &name)
    : CellSet(name),
      ValidCellIds(validCellIds),
      FullCellSet(cellset)
  {
  }

  VTKM_CONT
  CellSetGeneralPermutation(const std::string &name)
    : CellSet(name),
      ValidCellIds(),
      FullCellSet()
  {
  }

  VTKM_CONT
  vtkm::Id GetNumberOfCells() const
  {
    return this->ValidCellIds.GetNumberOfValues();
  }

  VTKM_CONT
  vtkm::Id GetNumberOfPoints() const
  {
    return this->FullCellSet.GetNumberOfPoints();
  }

  //This is the way you can fill the memory from another system without copying
  VTKM_CONT
  void Fill(const PermutationArrayHandleType &validCellIds,
            const OriginalCellSet& cellset)
  {
    ValidCellIds = validCellIds;
    FullCellSet = cellset;
  }

  template<typename TopologyElement>
  VTKM_CONT
  vtkm::Id GetSchedulingRange(TopologyElement) const {
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(TopologyElement);
    return this->ValidCellIds.GetNumberOfValues();
  }

  template<typename Device, typename FromTopology, typename ToTopology>
  struct ExecutionTypes {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);

    typedef typename PermutationArrayHandleType::template ExecutionTypes<Device>::PortalConst ExecPortalType;

    typedef typename OriginalCellSet::template ExecutionTypes<
                                                         Device,
                                                         FromTopology,
                                                         ToTopology>::ExecObjectType OrigExecObjectType;

    typedef vtkm::exec::ConnectivityPermuted< ExecPortalType, OrigExecObjectType> ExecObjectType;
  };

  template<typename Device, typename FromTopology, typename ToTopology>
  typename ExecutionTypes<Device,FromTopology,ToTopology>::ExecObjectType
  PrepareForInput(Device d, FromTopology f, ToTopology t) const
  {
    typedef typename
        ExecutionTypes<Device,FromTopology,ToTopology>::ExecObjectType
            ConnectivityType;
    return ConnectivityType(this->ValidCellIds.PrepareForInput(d),
                            this->FullCellSet.PrepareForInput(d,f,t) );
  }

  virtual void PrintSummary(std::ostream &out) const
  {
    out << "   CellSetGeneralPermutation of: "<< std::endl;
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

template< typename OriginalCellSet,
          typename PermutationArrayHandleType = vtkm::cont::ArrayHandle< vtkm::Id, VTKM_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG > >
class CellSetPermutation : public vtkm::cont::internal::CellSetGeneralPermutation<OriginalCellSet, PermutationArrayHandleType>
{
  VTKM_IS_CELL_SET(OriginalCellSet);
  VTKM_IS_ARRAY_HANDLE(PermutationArrayHandleType);

  typedef typename vtkm::cont::internal::CellSetGeneralPermutation<
      OriginalCellSet, PermutationArrayHandleType> ParentType;
public:

  VTKM_CONT
  CellSetPermutation(const PermutationArrayHandleType& validCellIds,
                     const OriginalCellSet& cellset,
                     const std::string &name = std::string())
    : ParentType(validCellIds, cellset, name)
  {
  }

  VTKM_CONT
  CellSetPermutation(const std::string &name = std::string())
    :  ParentType(name)
  {
  }

  VTKM_CONT
  CellSetPermutation<OriginalCellSet,PermutationArrayHandleType>&
  operator=(const CellSetPermutation<OriginalCellSet,PermutationArrayHandleType> &src)
  {
    ParentType::operator=(src);
    return *this;
  }

};

template<typename OriginalCellSet, typename PermutationArrayHandleType>
vtkm::cont::CellSetPermutation<OriginalCellSet,PermutationArrayHandleType>
make_CellSetPermutation(const PermutationArrayHandleType &cellIndexMap,
                        const OriginalCellSet &cellSet,
                        const std::string &name)
{
  VTKM_IS_CELL_SET(OriginalCellSet);
  VTKM_IS_ARRAY_HANDLE(PermutationArrayHandleType);

  return vtkm::cont::CellSetPermutation<
      OriginalCellSet,PermutationArrayHandleType>(
        cellIndexMap, cellSet, name);
}

template<typename OriginalCellSet, typename PermutationArrayHandleType>
vtkm::cont::CellSetPermutation<OriginalCellSet,PermutationArrayHandleType>
make_CellSetPermutation(const PermutationArrayHandleType &cellIndexMap,
                        const OriginalCellSet &cellSet)
{
  VTKM_IS_CELL_SET(OriginalCellSet);
  VTKM_IS_ARRAY_HANDLE(PermutationArrayHandleType);

  return vtkm::cont::make_CellSetPermutation(
        cellIndexMap, cellSet, cellSet.GetName());
}

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetPermutation_h
