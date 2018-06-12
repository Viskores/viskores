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
#ifndef vtk_m_cont_CellSetExplicit_h
#define vtk_m_cont_CellSetExplicit_h

#include <vtkm/CellShape.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/internal/ConnectivityExplicitInternals.h>
#include <vtkm/exec/ConnectivityExplicit.h>

#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

template <typename CellSetType, typename FromTopology, typename ToTopology>
struct CellSetExplicitConnectivityChooser
{
  using ConnectivityType = vtkm::cont::internal::ConnectivityExplicitInternals<>;
};

} // namespace detail

#ifndef VTKM_DEFAULT_SHAPE_STORAGE_TAG
#define VTKM_DEFAULT_SHAPE_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

#ifndef VTKM_DEFAULT_NUM_INDICES_STORAGE_TAG
#define VTKM_DEFAULT_NUM_INDICES_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

#ifndef VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG
#define VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

#ifndef VTKM_DEFAULT_OFFSETS_STORAGE_TAG
#define VTKM_DEFAULT_OFFSETS_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

template <typename ShapeStorageTag = VTKM_DEFAULT_SHAPE_STORAGE_TAG,
          typename NumIndicesStorageTag = VTKM_DEFAULT_NUM_INDICES_STORAGE_TAG,
          typename ConnectivityStorageTag = VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
          typename OffsetsStorageTag = VTKM_DEFAULT_OFFSETS_STORAGE_TAG>
class VTKM_ALWAYS_EXPORT CellSetExplicit : public CellSet
{
  using Thisclass = CellSetExplicit<ShapeStorageTag,
                                    NumIndicesStorageTag,
                                    ConnectivityStorageTag,
                                    OffsetsStorageTag>;

  template <typename FromTopology, typename ToTopology>
  struct ConnectivityChooser
  {
    using ConnectivityType =
      typename detail::CellSetExplicitConnectivityChooser<Thisclass,
                                                          FromTopology,
                                                          ToTopology>::ConnectivityType;

    using ShapeArrayType = typename ConnectivityType::ShapeArrayType;
    using NumIndicesArrayType = typename ConnectivityType::NumIndicesArrayType;
    using ConnectivityArrayType = typename ConnectivityType::ConnectivityArrayType;
    using IndexOffsetArrayType = typename ConnectivityType::IndexOffsetArrayType;
  };

public:
  using SchedulingRangeType = vtkm::Id;

  //point to cell is used when iterating cells and asking for point properties
  using PointToCellConnectivityType =
    ConnectivityChooser<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>;

  using ShapeArrayType = typename PointToCellConnectivityType::ShapeArrayType;
  using NumIndicesArrayType = typename PointToCellConnectivityType::NumIndicesArrayType;
  using ConnectivityArrayType = typename PointToCellConnectivityType::ConnectivityArrayType;
  using IndexOffsetArrayType = typename PointToCellConnectivityType::IndexOffsetArrayType;

  VTKM_CONT CellSetExplicit(const std::string& name = std::string());
  VTKM_CONT CellSetExplicit(const Thisclass& src);

  VTKM_CONT Thisclass& operator=(const Thisclass& src);

  virtual ~CellSetExplicit();

  vtkm::Id GetNumberOfCells() const override;
  vtkm::Id GetNumberOfPoints() const override;
  vtkm::Id GetNumberOfFaces() const override;
  vtkm::Id GetNumberOfEdges() const override;
  void PrintSummary(std::ostream& out) const override;
  void ReleaseResourcesExecution() override;

  VTKM_CONT vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagCell) const;
  VTKM_CONT vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagPoint) const;

  VTKM_CONT vtkm::IdComponent GetNumberOfPointsInCell(vtkm::Id cellIndex) const;

  VTKM_CONT vtkm::UInt8 GetCellShape(vtkm::Id cellIndex) const;

  template <vtkm::IdComponent ItemTupleLength>
  VTKM_CONT void GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id, ItemTupleLength>& ids) const;

  VTKM_CONT void GetIndices(vtkm::Id index, vtkm::cont::ArrayHandle<vtkm::Id>& ids) const;

  /// First method to add cells -- one at a time.
  VTKM_CONT void PrepareToAddCells(vtkm::Id numCells, vtkm::Id connectivityMaxLen);

  template <typename IdVecType>
  VTKM_CONT void AddCell(vtkm::UInt8 cellType, vtkm::IdComponent numVertices, const IdVecType& ids);

  VTKM_CONT void CompleteAddingCells(vtkm::Id numPoints);

  /// Second method to add cells -- all at once.
  /// Assigns the array handles to the explicit connectivity. This is
  /// the way you can fill the memory from another system without copying
  VTKM_CONT
  void Fill(vtkm::Id numPoints,
            const vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeStorageTag>& cellTypes,
            const vtkm::cont::ArrayHandle<vtkm::IdComponent, NumIndicesStorageTag>& numIndices,
            const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag>& connectivity,
            const vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorageTag>& offsets =
              vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorageTag>());

  template <typename DeviceAdapter, typename FromTopology, typename ToTopology>
  struct ExecutionTypes
  {
    VTKM_IS_DEVICE_ADAPTER_TAG(DeviceAdapter);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(FromTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(ToTopology);

    using ConnectivityTypes = ConnectivityChooser<FromTopology, ToTopology>;

    using ShapePortalType = typename ConnectivityTypes::ShapeArrayType::template ExecutionTypes<
      DeviceAdapter>::PortalConst;
    using IndicePortalType =
      typename ConnectivityTypes::NumIndicesArrayType::template ExecutionTypes<
        DeviceAdapter>::PortalConst;
    using ConnectivityPortalType =
      typename ConnectivityTypes::ConnectivityArrayType::template ExecutionTypes<
        DeviceAdapter>::PortalConst;
    using IndexOffsetPortalType =
      typename ConnectivityTypes::IndexOffsetArrayType::template ExecutionTypes<
        DeviceAdapter>::PortalConst;

    using ExecObjectType = vtkm::exec::ConnectivityExplicit<ShapePortalType,
                                                            IndicePortalType,
                                                            ConnectivityPortalType,
                                                            IndexOffsetPortalType>;
  };

  template <typename Device, typename FromTopology, typename ToTopology>
  typename ExecutionTypes<Device, FromTopology, ToTopology>::ExecObjectType
    PrepareForInput(Device, FromTopology, ToTopology) const;

  template <typename FromTopology, typename ToTopology>
  VTKM_CONT const typename ConnectivityChooser<FromTopology, ToTopology>::ShapeArrayType&
    GetShapesArray(FromTopology, ToTopology) const;

  template <typename FromTopology, typename ToTopology>
  VTKM_CONT const typename ConnectivityChooser<FromTopology, ToTopology>::NumIndicesArrayType&
    GetNumIndicesArray(FromTopology, ToTopology) const;

  template <typename FromTopology, typename ToTopology>
  VTKM_CONT const typename ConnectivityChooser<FromTopology, ToTopology>::ConnectivityArrayType&
    GetConnectivityArray(FromTopology, ToTopology) const;

  template <typename FromTopology, typename ToTopology>
  VTKM_CONT const typename ConnectivityChooser<FromTopology, ToTopology>::IndexOffsetArrayType&
    GetIndexOffsetArray(FromTopology, ToTopology) const;

protected:
  VTKM_CONT void BuildConnectivity(
    vtkm::TopologyElementTagPoint,
    vtkm::TopologyElementTagCell,
    vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker()) const;

  VTKM_CONT void BuildConnectivity(
    vtkm::TopologyElementTagCell,
    vtkm::TopologyElementTagPoint,
    vtkm::cont::RuntimeDeviceTracker tracker = vtkm::cont::GetGlobalRuntimeDeviceTracker()) const;

  typename ConnectivityChooser<vtkm::TopologyElementTagPoint,
                               vtkm::TopologyElementTagCell>::ConnectivityType PointToCell;

  typename ConnectivityChooser<vtkm::TopologyElementTagCell,
                               vtkm::TopologyElementTagPoint>::ConnectivityType CellToPoint;

  // These are used in the AddCell and related methods to incrementally add
  // cells. They need to be protected as subclasses of CellSetExplicit
  // need to set these values when implementing Fill()
  vtkm::Id ConnectivityAdded;
  vtkm::Id NumberOfCellsAdded;
  vtkm::Id NumberOfPoints;

private:
  auto GetConnectivity(vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell) const -> const
    decltype(Thisclass::PointToCell)&
  {
    return this->PointToCell;
  }

  auto GetConnectivity(vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell)
    -> decltype(Thisclass::PointToCell)&
  {
    return this->PointToCell;
  }

  auto GetConnectivity(vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint) const -> const
    decltype(Thisclass::CellToPoint)&
  {
    return this->CellToPoint;
  }

  auto GetConnectivity(vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint)
    -> decltype(Thisclass::CellToPoint)&
  {
    return this->CellToPoint;
  }
};

namespace detail
{

template <typename Storage1, typename Storage2, typename Storage3, typename Storage4>
struct CellSetExplicitConnectivityChooser<
  vtkm::cont::CellSetExplicit<Storage1, Storage2, Storage3, Storage4>,
  vtkm::TopologyElementTagPoint,
  vtkm::TopologyElementTagCell>
{
  using ConnectivityType =
    vtkm::cont::internal::ConnectivityExplicitInternals<Storage1, Storage2, Storage3, Storage4>;
};

template <typename CellSetType>
struct CellSetExplicitConnectivityChooser<CellSetType,
                                          vtkm::TopologyElementTagCell,
                                          vtkm::TopologyElementTagPoint>
{
  //only specify the shape type as it will be constant as everything
  //is a vertex. otherwise use the defaults.
  using ConnectivityType = vtkm::cont::internal::ConnectivityExplicitInternals<
    typename ArrayHandleConstant<vtkm::UInt8>::StorageTag>;
};

} // namespace detail

/// \cond
/// Make doxygen ignore this section
#ifndef vtk_m_cont_CellSetExplicit_cxx
extern template class VTKM_CONT_TEMPLATE_EXPORT CellSetExplicit<>; // default
extern template class VTKM_CONT_TEMPLATE_EXPORT CellSetExplicit<
  typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag,
  typename vtkm::cont::ArrayHandleConstant<vtkm::IdComponent>::StorageTag,
  VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
  typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag>; // CellSetSingleType base
#endif
/// \endcond
}
} // namespace vtkm::cont

#include <vtkm/cont/CellSetExplicit.hxx>

#endif //vtk_m_cont_CellSetExplicit_h
