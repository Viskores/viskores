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
#include <vtkm/cont/CellSetExplicit.h>
#include <vtkm/cont/CellSetStructured.h>
#include <vtkm/exec/ConnectivityStructuredPermuted.h>

namespace vtkm {
namespace cont {

namespace internal{
template<typename ValidCellArrayHandleType, typename OriginalCellSet>
struct CellSetPermutationTraits
{
  typedef typename OriginalCellSet::ShapeArrayType ShapeArrayType;
  typedef typename OriginalCellSet::NumIndicesArrayType NumIndicesArrayType;
  typedef typename OriginalCellSet::ConnectivityArrayType ConnectivityArrayType;
  typedef typename OriginalCellSet::IndexOffsetArrayType IndexOffsetArrayType;


  typedef vtkm::cont::ArrayHandlePermutation<ValidCellArrayHandleType,
    ShapeArrayType> PermShapeArrayType;

  typedef vtkm::cont::ArrayHandlePermutation<ValidCellArrayHandleType,
    NumIndicesArrayType> PermNumIndicesArrayType;

  typedef vtkm::cont::ArrayHandlePermutation<ValidCellArrayHandleType,
    IndexOffsetArrayType> PermIndexOffsetArrayType;


  typedef vtkm::cont::CellSetExplicit<
    typename PermShapeArrayType::StorageTag,
    typename PermNumIndicesArrayType::StorageTag,
    typename ConnectivityArrayType::StorageTag,
    typename PermIndexOffsetArrayType::StorageTag> PermutedCellSetType;
};

template<typename ValidCellArrayHandleType, vtkm::IdComponent DIMENSION>
struct CellSetPermutationTraits<ValidCellArrayHandleType, CellSetStructured<DIMENSION> >
{
};


}

template< typename ValidCellArrayHandleType,
          typename OriginalCellSet >
class CellSetPermutation : public CellSet
{
  typedef vtkm::cont::CellSetPermutation<
      ValidCellArrayHandleType,OriginalCellSet> Thisclass;

public:
  typedef typename vtkm::cont::internal::CellSetPermutationTraits<
  ValidCellArrayHandleType,OriginalCellSet>::PermutedCellSetType PermutedCellSetType;


  typedef typename PermutedCellSetType::ShapeArrayType ShapeArrayType;
  typedef typename PermutedCellSetType::NumIndicesArrayType NumIndicesArrayType;
  typedef typename PermutedCellSetType::ConnectivityArrayType ConnectivityArrayType;
  typedef typename PermutedCellSetType::IndexOffsetArrayType IndexOffsetArrayType;

  VTKM_CONT_EXPORT
  CellSetPermutation(const ValidCellArrayHandleType& validCellIds,
                const OriginalCellSet& cellset,
                const std::string &name = std::string(),
                vtkm::IdComponent dimensionality = 3)
    : CellSet(name,dimensionality),
      PermutedCellSet(0, name, dimensionality)
  {
    this->Fill(validCellIds, cellset);
  }

  VTKM_CONT_EXPORT
  CellSetPermutation(const std::string &name = std::string(),
                vtkm::IdComponent dimensionality = 3)
    : CellSet(name,dimensionality),
      ValidCellIds(),
      PermutedCellSet(0, name, dimensionality)
  {
  }

  VTKM_CONT_EXPORT
  CellSetPermutation(const Thisclass &src)
    : CellSet(src),
      ValidCellIds(src.ValidCellIds),
      PermutedCellSet(src.PermutedCellSet)
  {  }

  VTKM_CONT_EXPORT
  Thisclass &operator=(const Thisclass &src)
  {
    this->CellSet::operator=(src);
    this->ValidCellIds = src.ValidCellIds;
    this->PermutedCellSet = src.PermutedCellSet;
    return *this;
  }

  virtual ~CellSetPermutation() {  }

  //This is the way you can fill the memory from another system without copying
  VTKM_CONT_EXPORT
  void Fill(const ValidCellArrayHandleType &validCellIds,
            const OriginalCellSet& cellset)
  {
    typedef vtkm::cont::internal::CellSetPermutationTraits<
      ValidCellArrayHandleType, OriginalCellSet> Traits;

    typedef vtkm::TopologyElementTagPoint ElemPointTag;
    typedef vtkm::TopologyElementTagCell ElemCellTag;

    PermutedCellSetType permutedCellSet(0,this->PermutedCellSet.GetName(),
                                        cellset.GetDimensionality());

    typename Traits::PermShapeArrayType shapeArray(validCellIds,
                                                   cellset.GetShapesArray(ElemPointTag(),ElemCellTag()));
    typename Traits::PermNumIndicesArrayType numArray(validCellIds,
                                                      cellset.GetNumIndicesArray(ElemPointTag(),ElemCellTag()));
    typename Traits::PermIndexOffsetArrayType offsArray(validCellIds,
                                                      cellset.GetIndexOffsetArray(ElemPointTag(),ElemCellTag()));

    permutedCellSet.Fill( shapeArray,
                          numArray,
                          cellset.GetConnectivityArray(ElemPointTag(),ElemCellTag()),
                          offsArray);

    this->ValidCellIds = validCellIds;
    this->PermutedCellSet = permutedCellSet;
  }

  template<typename FromTopology, typename ToTopology>
  VTKM_CONT_EXPORT
  const ShapeArrayType&
  GetShapesArray(FromTopology,ToTopology) const
  {
    return this->PermutedCellSet.GetShapesArray(FromTopology(), ToTopology());
  }

  template<typename FromTopology, typename ToTopology>
  VTKM_CONT_EXPORT
  const NumIndicesArrayType&
  GetNumIndicesArray(FromTopology,ToTopology) const
  {
    return this->PermutedCellSet.GetNumIndicesArray(FromTopology(),ToTopology());
  }

  template<typename FromTopology, typename ToTopology>
  VTKM_CONT_EXPORT
  const ConnectivityArrayType&
  GetConnectivityArray(FromTopology,ToTopology) const
  {
    return this->PermutedCellSet.GetConnectivityArray(FromTopology(),ToTopology());
  }

  template<typename FromTopology, typename ToTopology>
  VTKM_CONT_EXPORT
  const IndexOffsetArrayType&
  GetIndexOffsetArray(FromTopology,ToTopology) const
  {
    return this->PermutedCellSet.GetIndexOffsetArray(FromTopology(),ToTopology());
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const
  {
    return this->PermutedCellSet.GetNumberOfCells();
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagCell) const
  {
    return this->PermutedCellSet.GetNumberOfCells();
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagPoint) const
  {
    return this->PermutedCellSet.GetNumberOfPoints();
  }

  template <typename DeviceAdapter, typename FromTopology, typename ToTopology>
  struct ExecutionTypes
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapter);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);

    typedef typename PermutedCellSetType::template ExecutionTypes<
                                                         DeviceAdapter,
                                                         FromTopology,
                                                         ToTopology>::ExecObjectType ExecObjectType;
  };


  template<typename Device, typename FromTopology, typename ToTopology>
  typename ExecutionTypes<Device,FromTopology,ToTopology>::ExecObjectType
  PrepareForInput(Device d, FromTopology f, ToTopology t) const
  {
    return this->PermutedCellSet.PrepareForInput(d,f,t);
  }

  virtual void PrintSummary(std::ostream &out) const
  {
    out << "   CellSetPermutation<ExplicitCellType>: " <<std::endl;
    PermutedCellSet.PrintSummary(out);
  }

private:
  ValidCellArrayHandleType ValidCellIds;
  PermutedCellSetType PermutedCellSet;
};

template< typename ValidCellArrayHandleType,vtkm::IdComponent Dimension>
class CellSetPermutation<ValidCellArrayHandleType, CellSetStructured<Dimension> > : public CellSet
{
  typedef vtkm::internal::ConnectivityStructuredInternals<Dimension>
      InternalsType;
public:
  VTKM_CONT_EXPORT
  CellSetPermutation(const ValidCellArrayHandleType& validCellIds,
                const CellSetStructured<Dimension>& cellset,
                const std::string &name = std::string(),
                vtkm::IdComponent dimensionality = Dimension)
    : CellSet(name,dimensionality),
      ValidCellIds(),
      FullCellSet()
  {
    this->Fill(validCellIds, cellset);
  }

  VTKM_CONT_EXPORT
  CellSetPermutation(const std::string &name = std::string(),
                vtkm::IdComponent dimensionality = Dimension)
    : CellSet(name,dimensionality),
      ValidCellIds(),
      FullCellSet()
  {
  }

  VTKM_CONT_EXPORT
  vtkm::Id GetNumberOfCells() const
  {
    return this->ValidCellIds.GetNumberOfValues();
  }

  //This is the way you can fill the memory from another system without copying
  VTKM_CONT_EXPORT
  void Fill(const ValidCellArrayHandleType &validCellIds,
            const CellSetStructured<Dimension>& cellset)
  {
    ValidCellIds = validCellIds;
    FullCellSet = cellset;
  }

  template<typename TopologyElement>
  VTKM_CONT_EXPORT
  vtkm::Id GetSchedulingRange(TopologyElement) const {
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(TopologyElement);
    return this->ValidCellIds.GetNumberOfValues();
  }

  template<typename Device, typename FromTopology, typename ToTopology>
  struct ExecutionTypes {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);

    typedef typename ValidCellArrayHandleType::template ExecutionTypes<Device>::PortalConst ExecPortalType;

    typedef vtkm::exec::ConnectivityStructuredPermuted< ExecPortalType,
                                                        FromTopology,ToTopology,Dimension> ExecObjectType;
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
    out << "   CellSetPermutation<StructuredCellType<"<<Dimension<<"> >: " <<std::endl;
  }

private:
  std::string Name;
  vtkm::Id Dimensionality;

  ValidCellArrayHandleType ValidCellIds;
  CellSetStructured<Dimension> FullCellSet;
};

}
} // namespace vtkm::cont

#endif //vtk_m_cont_CellSetPermutation_h
