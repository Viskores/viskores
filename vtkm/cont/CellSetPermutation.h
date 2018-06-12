//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2015 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2015 UT-Battelle, LLC.
//  Copyright 2015 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
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
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandlePermutation.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/internal/ConnectivityExplicitInternals.h>
#include <vtkm/internal/ConnectivityStructuredInternals.h>
#include <vtkm/worklet/DispatcherMapTopology.h>
#include <vtkm/worklet/WorkletMapTopology.h>

#include <vtkm/exec/ConnectivityPermuted.h>

#ifndef VTKM_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG
#define VTKM_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

namespace vtkm
{
namespace cont
{

namespace internal
{

struct WriteConnectivity : public vtkm::worklet::WorkletMapPointToCell
{
  using ControlSignature = void(CellSetIn cellset,
                                FieldInCell<IdType> offset,
                                WholeArrayOut<> connectivity);
  using ExecutionSignature = void(PointCount, PointIndices, _2, _3);
  using InputDomain = _1;

  template <typename PointIndicesType, typename OutPortalType>
  VTKM_EXEC void operator()(vtkm::IdComponent pointcount,
                            const PointIndicesType& pointIndices,
                            vtkm::Id offset,
                            OutPortalType& connectivity) const
  {
    for (vtkm::IdComponent i = 0; i < pointcount; ++i)
    {
      connectivity.Set(offset++, pointIndices[i]);
    }
  }
};

// default for CellSetExplicit and CellSetSingleType
template <typename CellSetPermutationType>
class PointToCell
{
private:
  using NumIndicesArrayType = decltype(make_ArrayHandlePermutation(
    std::declval<CellSetPermutationType>().GetValidCellIds(),
    std::declval<CellSetPermutationType>().GetFullCellSet().GetNumIndicesArray(
      vtkm::TopologyElementTagPoint(),
      vtkm::TopologyElementTagCell())));

public:
  using ConnectivityArrays = vtkm::cont::internal::ConnectivityExplicitInternals<
    VTKM_DEFAULT_STORAGE_TAG, // dummy, shapes array is not used
    typename NumIndicesArrayType::StorageTag>;

  template <typename Device>
  static ConnectivityArrays Get(const CellSetPermutationType& cellset, Device)
  {
    ConnectivityArrays conn;
    conn.NumIndices =
      NumIndicesArrayType(cellset.GetValidCellIds(),
                          cellset.GetFullCellSet().GetNumIndicesArray(
                            vtkm::TopologyElementTagPoint(), vtkm::TopologyElementTagCell()));
    vtkm::Id connectivityLength = vtkm::cont::DeviceAdapterAlgorithm<Device>::ScanExclusive(
      vtkm::cont::make_ArrayHandleCast(conn.NumIndices, vtkm::Id()), conn.IndexOffsets);
    conn.Connectivity.Allocate(connectivityLength);
    vtkm::worklet::DispatcherMapTopology<WriteConnectivity, Device>().Invoke(
      cellset, conn.IndexOffsets, conn.Connectivity);

    return conn;
  }
};

// specialization for CellSetStructured
template <vtkm::IdComponent DIMENSION, typename PermutationArrayHandleType>
class PointToCell<CellSetPermutation<CellSetStructured<DIMENSION>, PermutationArrayHandleType>>
{
private:
  using CellSetPermutationType =
    CellSetPermutation<CellSetStructured<DIMENSION>, PermutationArrayHandleType>;

public:
  using ConnectivityArrays = vtkm::cont::internal::ConnectivityExplicitInternals<
    VTKM_DEFAULT_STORAGE_TAG, // dummy, shapes array is not used
    typename vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>::StorageTag,
    VTKM_DEFAULT_STORAGE_TAG,
    typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag>;

  template <typename Device>
  static ConnectivityArrays Get(const CellSetPermutationType& cellset, Device)
  {
    vtkm::Id numberOfCells = cellset.GetNumberOfCells();
    vtkm::IdComponent numPointsInCell =
      vtkm::internal::ConnectivityStructuredInternals<DIMENSION>::NUM_POINTS_IN_CELL;
    vtkm::Id connectivityLength = numberOfCells * numPointsInCell;

    ConnectivityArrays conn;
    conn.NumIndices = make_ArrayHandleConstant(numPointsInCell, numberOfCells);
    conn.IndexOffsets = ArrayHandleCounting<vtkm::Id>(0, numPointsInCell, numberOfCells);
    conn.Connectivity.Allocate(connectivityLength);
    vtkm::worklet::DispatcherMapTopology<WriteConnectivity, Device>().Invoke(
      cellset, conn.IndexOffsets, conn.Connectivity);

    return conn;
  }
};

template <typename CellSetPermutationType>
struct CellSetPermutationTraits;

template <typename OriginalCellSet_, typename PermutationArrayHandleType_>
struct CellSetPermutationTraits<CellSetPermutation<OriginalCellSet_, PermutationArrayHandleType_>>
{
  using OriginalCellSet = OriginalCellSet_;
  using PermutationArrayHandleType = PermutationArrayHandleType_;
};

template <typename OriginalCellSet_,
          typename OriginalPermutationArrayHandleType,
          typename PermutationArrayHandleType_>
struct CellSetPermutationTraits<
  CellSetPermutation<CellSetPermutation<OriginalCellSet_, OriginalPermutationArrayHandleType>,
                     PermutationArrayHandleType_>>
{
  using PreviousCellSet = CellSetPermutation<OriginalCellSet_, OriginalPermutationArrayHandleType>;
  using PermutationArrayHandleType = vtkm::cont::ArrayHandlePermutation<
    PermutationArrayHandleType_,
    typename CellSetPermutationTraits<PreviousCellSet>::PermutationArrayHandleType>;
  using OriginalCellSet = typename CellSetPermutationTraits<PreviousCellSet>::OriginalCellSet;
  using Superclass = CellSetPermutation<OriginalCellSet, PermutationArrayHandleType>;
};

} // internal

template <typename OriginalCellSetType_,
          typename PermutationArrayHandleType_ =
            vtkm::cont::ArrayHandle<vtkm::Id, VTKM_DEFAULT_CELLSET_PERMUTATION_STORAGE_TAG>>
class CellSetPermutation : public CellSet
{
public:
  using OriginalCellSetType = OriginalCellSetType_;
  using PermutationArrayHandleType = PermutationArrayHandleType_;

  VTKM_CONT
  CellSetPermutation(const PermutationArrayHandleType& validCellIds,
                     const OriginalCellSetType& cellset,
                     const std::string& name = std::string())
    : CellSet(name)
    , ValidCellIds(validCellIds)
    , FullCellSet(cellset)
  {
  }

  VTKM_CONT
  CellSetPermutation(const std::string& name = std::string())
    : CellSet(name)
    , ValidCellIds()
    , FullCellSet()
  {
  }

  VTKM_CONT
  const OriginalCellSetType& GetFullCellSet() const { return this->FullCellSet; }

  VTKM_CONT
  const PermutationArrayHandleType& GetValidCellIds() const { return this->ValidCellIds; }

  VTKM_CONT
  vtkm::Id GetNumberOfCells() const override { return this->ValidCellIds.GetNumberOfValues(); }

  VTKM_CONT
  vtkm::Id GetNumberOfPoints() const override { return this->FullCellSet.GetNumberOfPoints(); }

  VTKM_CONT
  vtkm::Id GetNumberOfFaces() const override { return -1; }

  VTKM_CONT
  vtkm::Id GetNumberOfEdges() const override { return -1; }

  VTKM_CONT
  void ReleaseResourcesExecution() override
  {
    this->ValidCellIds.ReleaseResourcesExecution();
    this->FullCellSet.ReleaseResourcesExecution();
    this->CellToPoint.ReleaseResourcesExecution();
  }


  VTKM_CONT
  vtkm::IdComponent GetNumberOfPointsInCell(vtkm::Id cellIndex) const
  {
    return this->FullCellSet.GetNumberOfPointsInCell(
      this->ValidCellIds.GetPortalConstControl().Get(cellIndex));
  }

  //This is the way you can fill the memory from another system without copying
  VTKM_CONT
  void Fill(const PermutationArrayHandleType& validCellIds, const OriginalCellSetType& cellset)
  {
    this->ValidCellIds = validCellIds;
    this->FullCellSet = cellset;
  }

  VTKM_CONT vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagCell) const
  {
    return this->ValidCellIds.GetNumberOfValues();
  }

  VTKM_CONT vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagPoint) const
  {
    return this->FullCellSet.GetNumberOfPoints();
  }

  template <typename Device, typename FromTopology, typename ToTopology>
  struct ExecutionTypes;

  template <typename Device>
  struct ExecutionTypes<Device, vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using ExecPortalType =
      typename PermutationArrayHandleType::template ExecutionTypes<Device>::PortalConst;
    using OrigExecObjectType = typename OriginalCellSetType::template ExecutionTypes<
      Device,
      vtkm::TopologyElementTagPoint,
      vtkm::TopologyElementTagCell>::ExecObjectType;

    using ExecObjectType =
      vtkm::exec::ConnectivityPermutedPointToCell<ExecPortalType, OrigExecObjectType>;
  };

  template <typename Device>
  struct ExecutionTypes<Device, vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);

    using ConnectiviyPortalType =
      typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<Device>::PortalConst;
    using NumIndicesPortalType = typename vtkm::cont::ArrayHandle<
      vtkm::IdComponent>::template ExecutionTypes<Device>::PortalConst;
    using IndexOffsetPortalType =
      typename vtkm::cont::ArrayHandle<vtkm::Id>::template ExecutionTypes<Device>::PortalConst;

    using ExecObjectType = vtkm::exec::ConnectivityPermutedCellToPoint<ConnectiviyPortalType,
                                                                       NumIndicesPortalType,
                                                                       IndexOffsetPortalType>;
  };

  template <typename Device>
  VTKM_CONT typename ExecutionTypes<Device,
                                    vtkm::TopologyElementTagPoint,
                                    vtkm::TopologyElementTagCell>::ExecObjectType
  PrepareForInput(Device device,
                  vtkm::TopologyElementTagPoint from,
                  vtkm::TopologyElementTagCell to) const
  {
    using ConnectivityType = typename ExecutionTypes<Device,
                                                     vtkm::TopologyElementTagPoint,
                                                     vtkm::TopologyElementTagCell>::ExecObjectType;
    return ConnectivityType(this->ValidCellIds.PrepareForInput(device),
                            this->FullCellSet.PrepareForInput(device, from, to));
  }

  template <typename Device>
  VTKM_CONT typename ExecutionTypes<Device,
                                    vtkm::TopologyElementTagCell,
                                    vtkm::TopologyElementTagPoint>::ExecObjectType
  PrepareForInput(Device device, vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const
  {
    if (!this->CellToPoint.ElementsValid)
    {
      auto pointToCell = internal::PointToCell<CellSetPermutation>::Get(*this, device);
      internal::ComputeCellToPointConnectivity(
        this->CellToPoint, pointToCell, this->GetNumberOfPoints(), device);
      this->CellToPoint.BuildIndexOffsets(device);
    }

    using ConnectivityType = typename ExecutionTypes<Device,
                                                     vtkm::TopologyElementTagCell,
                                                     vtkm::TopologyElementTagPoint>::ExecObjectType;
    return ConnectivityType(this->CellToPoint.Connectivity.PrepareForInput(device),
                            this->CellToPoint.NumIndices.PrepareForInput(device),
                            this->CellToPoint.IndexOffsets.PrepareForInput(device));
  }

  VTKM_CONT
  void PrintSummary(std::ostream& out) const override
  {
    out << "CellSetPermutation of: " << std::endl;
    this->FullCellSet.PrintSummary(out);
    out << "Permutation Array: " << std::endl;
    vtkm::cont::printSummary_ArrayHandle(this->ValidCellIds, out);
  }

private:
  using CellToPointConnectivity = vtkm::cont::internal::ConnectivityExplicitInternals<
    typename ArrayHandleConstant<vtkm::UInt8>::StorageTag>;

  PermutationArrayHandleType ValidCellIds;
  OriginalCellSetType FullCellSet;
  mutable CellToPointConnectivity CellToPoint;
};

template <typename CellSetType,
          typename PermutationArrayHandleType1,
          typename PermutationArrayHandleType2>
class CellSetPermutation<CellSetPermutation<CellSetType, PermutationArrayHandleType1>,
                         PermutationArrayHandleType2>
  : public internal::CellSetPermutationTraits<
      CellSetPermutation<CellSetPermutation<CellSetType, PermutationArrayHandleType1>,
                         PermutationArrayHandleType2>>::Superclass
{
private:
  using Superclass = typename internal::CellSetPermutationTraits<CellSetPermutation>::Superclass;

public:
  VTKM_CONT
  CellSetPermutation(const PermutationArrayHandleType2& validCellIds,
                     const CellSetPermutation<CellSetType, PermutationArrayHandleType1>& cellset,
                     const std::string& name = std::string())
    : Superclass(vtkm::cont::make_ArrayHandlePermutation(validCellIds, cellset.GetValidCellIds()),
                 cellset.GetFullCellSet(),
                 name)
  {
  }

  VTKM_CONT
  CellSetPermutation(const std::string& name = std::string())
    : Superclass(name)
  {
  }

  VTKM_CONT
  void Fill(const PermutationArrayHandleType2& validCellIds,
            const CellSetPermutation<CellSetType, PermutationArrayHandleType1>& cellset)
  {
    this->ValidCellIds = make_ArrayHandlePermutation(validCellIds, cellset.GetValidCellIds());
    this->FullCellSet = cellset.GetFullCellSet();
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
