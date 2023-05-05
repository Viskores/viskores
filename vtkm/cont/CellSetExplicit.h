//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellSetExplicit_h
#define vtk_m_cont_CellSetExplicit_h

#include <vtkm/CellShape.h>
#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/ArrayGetValues.h>
#include <vtkm/cont/ArrayHandleCast.h>
#include <vtkm/cont/ArrayHandleConstant.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleOffsetsToNumComponents.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/UnknownArrayHandle.h>
#include <vtkm/cont/internal/ConnectivityExplicitInternals.h>
#include <vtkm/exec/ConnectivityExplicit.h>

#include <vtkm/cont/vtkm_cont_export.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

template <typename CellSetType, typename VisitTopology, typename IncidentTopology>
struct CellSetExplicitConnectivityChooser
{
  using ConnectivityType = vtkm::cont::internal::ConnectivityExplicitInternals<>;
};

// The connectivity generally used for the visit-points-with-cells connectivity.
// This type of connectivity does not have variable shape types, and since it is
// never really provided externally we can use the defaults for the other arrays.
using DefaultVisitPointsWithCellsConnectivityExplicit =
  vtkm::cont::internal::ConnectivityExplicitInternals<
    typename ArrayHandleConstant<vtkm::UInt8>::StorageTag>;

VTKM_CONT_EXPORT void BuildReverseConnectivity(
  const vtkm::cont::UnknownArrayHandle& connections,
  const vtkm::cont::UnknownArrayHandle& offsets,
  vtkm::Id numberOfPoints,
  vtkm::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit& visitPointsWithCells,
  vtkm::cont::DeviceAdapterId device);

} // namespace detail

#ifndef VTKM_DEFAULT_SHAPES_STORAGE_TAG
#define VTKM_DEFAULT_SHAPES_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

#ifndef VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG
#define VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

#ifndef VTKM_DEFAULT_OFFSETS_STORAGE_TAG
#define VTKM_DEFAULT_OFFSETS_STORAGE_TAG VTKM_DEFAULT_STORAGE_TAG
#endif

/// @brief Defines an irregular collection of cells.
///
/// The cells can be of different types and connected in arbitrary ways.
/// This is done by explicitly providing for each cell a sequence of points that defines the cell.
template <typename ShapesStorageTag = VTKM_DEFAULT_SHAPES_STORAGE_TAG,
          typename ConnectivityStorageTag = VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
          typename OffsetsStorageTag = VTKM_DEFAULT_OFFSETS_STORAGE_TAG>
class VTKM_ALWAYS_EXPORT CellSetExplicit : public CellSet
{
  using Thisclass = CellSetExplicit<ShapesStorageTag, ConnectivityStorageTag, OffsetsStorageTag>;

  template <typename VisitTopology, typename IncidentTopology>
  struct ConnectivityChooser
  {
  private:
    using Chooser = typename detail::
      CellSetExplicitConnectivityChooser<Thisclass, VisitTopology, IncidentTopology>;

  public:
    using ConnectivityType = typename Chooser::ConnectivityType;
    using ShapesArrayType = typename ConnectivityType::ShapesArrayType;
    using ConnectivityArrayType = typename ConnectivityType::ConnectivityArrayType;
    using OffsetsArrayType = typename ConnectivityType::OffsetsArrayType;

    using NumIndicesArrayType = vtkm::cont::ArrayHandleOffsetsToNumComponents<OffsetsArrayType>;

    using ExecConnectivityType =
      vtkm::exec::ConnectivityExplicit<typename ShapesArrayType::ReadPortalType,
                                       typename ConnectivityArrayType::ReadPortalType,
                                       typename OffsetsArrayType::ReadPortalType>;
  };

  using ConnTypes =
    ConnectivityChooser<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>;
  using RConnTypes =
    ConnectivityChooser<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>;

  using CellPointIdsType = typename ConnTypes::ConnectivityType;
  using PointCellIdsType = typename RConnTypes::ConnectivityType;

public:
  using SchedulingRangeType = vtkm::Id;

  using ShapesArrayType = typename CellPointIdsType::ShapesArrayType;
  using ConnectivityArrayType = typename CellPointIdsType::ConnectivityArrayType;
  using OffsetsArrayType = typename CellPointIdsType::OffsetsArrayType;
  using NumIndicesArrayType = typename ConnTypes::NumIndicesArrayType;

  VTKM_CONT CellSetExplicit();
  VTKM_CONT CellSetExplicit(const Thisclass& src);
  VTKM_CONT CellSetExplicit(Thisclass&& src) noexcept;

  VTKM_CONT Thisclass& operator=(const Thisclass& src);
  VTKM_CONT Thisclass& operator=(Thisclass&& src) noexcept;

  VTKM_CONT ~CellSetExplicit() override;

  VTKM_CONT vtkm::Id GetNumberOfCells() const override;
  VTKM_CONT vtkm::Id GetNumberOfPoints() const override;
  VTKM_CONT vtkm::Id GetNumberOfFaces() const override;
  VTKM_CONT vtkm::Id GetNumberOfEdges() const override;
  VTKM_CONT void PrintSummary(std::ostream& out) const override;

  VTKM_CONT void ReleaseResourcesExecution() override;

  VTKM_CONT std::shared_ptr<CellSet> NewInstance() const override;
  VTKM_CONT void DeepCopy(const CellSet* src) override;

  VTKM_CONT vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagCell) const;
  VTKM_CONT vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagPoint) const;

  VTKM_CONT vtkm::IdComponent GetNumberOfPointsInCell(vtkm::Id cellid) const override;
  VTKM_CONT void GetCellPointIds(vtkm::Id id, vtkm::Id* ptids) const override;

  VTKM_CONT typename vtkm::cont::ArrayHandle<vtkm::UInt8, ShapesStorageTag>::ReadPortalType
  ShapesReadPortal() const;

  VTKM_CONT vtkm::UInt8 GetCellShape(vtkm::Id cellid) const override;

  template <vtkm::IdComponent NumIndices>
  VTKM_CONT void GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id, NumIndices>& ids) const;

  VTKM_CONT void GetIndices(vtkm::Id index, vtkm::cont::ArrayHandle<vtkm::Id>& ids) const;

  /// @brief Start adding cells one at a time.
  ///
  /// After this method is called, `AddCell` is called repeatedly to add each cell.
  /// Once all cells are added, call `CompleteAddingCells`.
  VTKM_CONT void PrepareToAddCells(vtkm::Id numCells, vtkm::Id connectivityMaxLen);

  /// @brief Add a cell.
  ///
  /// This can only be called after `AddCell`.
  template <typename IdVecType>
  VTKM_CONT void AddCell(vtkm::UInt8 cellType, vtkm::IdComponent numVertices, const IdVecType& ids);

  /// @brief Finish adding cells one at a time.
  VTKM_CONT void CompleteAddingCells(vtkm::Id numPoints);

  /// @brief Set all the cells of the mesh.
  ///
  /// This method can be used to fill the memory from another system without
  /// copying data.
  VTKM_CONT
  void Fill(vtkm::Id numPoints,
            const vtkm::cont::ArrayHandle<vtkm::UInt8, ShapesStorageTag>& cellTypes,
            const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag>& connectivity,
            const vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorageTag>& offsets);

  template <typename VisitTopology, typename IncidentTopology>
  using ExecConnectivityType =
    typename ConnectivityChooser<VisitTopology, IncidentTopology>::ExecConnectivityType;

  template <typename VisitTopology, typename IncidentTopology>
  VTKM_CONT ExecConnectivityType<VisitTopology, IncidentTopology> PrepareForInput(
    vtkm::cont::DeviceAdapterId,
    VisitTopology,
    IncidentTopology,
    vtkm::cont::Token&) const;

  template <typename VisitTopology, typename IncidentTopology>
  VTKM_CONT const typename ConnectivityChooser<VisitTopology, IncidentTopology>::ShapesArrayType&
    GetShapesArray(VisitTopology, IncidentTopology) const;

  template <typename VisitTopology, typename IncidentTopology>
  VTKM_CONT const typename ConnectivityChooser<VisitTopology,
                                               IncidentTopology>::ConnectivityArrayType&
    GetConnectivityArray(VisitTopology, IncidentTopology) const;

  template <typename VisitTopology, typename IncidentTopology>
  VTKM_CONT const typename ConnectivityChooser<VisitTopology, IncidentTopology>::OffsetsArrayType&
    GetOffsetsArray(VisitTopology, IncidentTopology) const;

  template <typename VisitTopology, typename IncidentTopology>
  VTKM_CONT typename ConnectivityChooser<VisitTopology, IncidentTopology>::NumIndicesArrayType
    GetNumIndicesArray(VisitTopology, IncidentTopology) const;

  template <typename VisitTopology, typename IncidentTopology>
  VTKM_CONT bool HasConnectivity(VisitTopology visit, IncidentTopology incident) const
  {
    return this->HasConnectivityImpl(visit, incident);
  }

  // Can be used to reset a connectivity table, mostly useful for benchmarking.
  template <typename VisitTopology, typename IncidentTopology>
  VTKM_CONT void ResetConnectivity(VisitTopology visit, IncidentTopology incident)
  {
    this->ResetConnectivityImpl(visit, incident);
  }

protected:
  VTKM_CONT void BuildConnectivity(vtkm::cont::DeviceAdapterId,
                                   vtkm::TopologyElementTagCell,
                                   vtkm::TopologyElementTagPoint) const
  {
    VTKM_ASSERT(this->Data->CellPointIds.ElementsValid);
    // no-op
  }

  VTKM_CONT void BuildConnectivity(vtkm::cont::DeviceAdapterId device,
                                   vtkm::TopologyElementTagPoint,
                                   vtkm::TopologyElementTagCell) const
  {
    detail::BuildReverseConnectivity(this->Data->CellPointIds.Connectivity,
                                     this->Data->CellPointIds.Offsets,
                                     this->Data->NumberOfPoints,
                                     this->Data->PointCellIds,
                                     device);
  }

  VTKM_CONT bool HasConnectivityImpl(vtkm::TopologyElementTagCell,
                                     vtkm::TopologyElementTagPoint) const
  {
    return this->Data->CellPointIds.ElementsValid;
  }

  VTKM_CONT bool HasConnectivityImpl(vtkm::TopologyElementTagPoint,
                                     vtkm::TopologyElementTagCell) const
  {
    return this->Data->PointCellIds.ElementsValid;
  }

  VTKM_CONT void ResetConnectivityImpl(vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint)
  {
    // Reset entire cell set
    this->Data->CellPointIds = CellPointIdsType{};
    this->Data->PointCellIds = PointCellIdsType{};
    this->Data->ConnectivityAdded = -1;
    this->Data->NumberOfCellsAdded = -1;
    this->Data->NumberOfPoints = 0;
  }

  VTKM_CONT void ResetConnectivityImpl(vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell)
  {
    this->Data->PointCellIds = PointCellIdsType{};
  }

  // Store internals in a shared pointer so shallow copies stay consistent.
  // See #2268.
  struct Internals
  {
    CellPointIdsType CellPointIds;
    PointCellIdsType PointCellIds;

    // These are used in the AddCell and related methods to incrementally add
    // cells. They need to be protected as subclasses of CellSetExplicit
    // need to set these values when implementing Fill()
    vtkm::Id ConnectivityAdded;
    vtkm::Id NumberOfCellsAdded;
    vtkm::Id NumberOfPoints;

    VTKM_CONT
    Internals()
      : ConnectivityAdded(-1)
      , NumberOfCellsAdded(-1)
      , NumberOfPoints(0)
    {
    }
  };

  std::shared_ptr<Internals> Data;

private:
  VTKM_CONT
  const CellPointIdsType& GetConnectivity(vtkm::TopologyElementTagCell,
                                          vtkm::TopologyElementTagPoint) const
  {
    return this->Data->CellPointIds;
  }

  VTKM_CONT
  const CellPointIdsType& GetConnectivity(vtkm::TopologyElementTagCell,
                                          vtkm::TopologyElementTagPoint)
  {
    return this->Data->CellPointIds;
  }

  VTKM_CONT
  const PointCellIdsType& GetConnectivity(vtkm::TopologyElementTagPoint,
                                          vtkm::TopologyElementTagCell) const
  {
    return this->Data->PointCellIds;
  }

  VTKM_CONT
  const PointCellIdsType& GetConnectivity(vtkm::TopologyElementTagPoint,
                                          vtkm::TopologyElementTagCell)
  {
    return this->Data->PointCellIds;
  }
};

namespace detail
{

template <typename Storage1, typename Storage2, typename Storage3>
struct CellSetExplicitConnectivityChooser<vtkm::cont::CellSetExplicit<Storage1, Storage2, Storage3>,
                                          vtkm::TopologyElementTagCell,
                                          vtkm::TopologyElementTagPoint>
{
  using ConnectivityType =
    vtkm::cont::internal::ConnectivityExplicitInternals<Storage1, Storage2, Storage3>;
};

template <typename CellSetType>
struct CellSetExplicitConnectivityChooser<CellSetType,
                                          vtkm::TopologyElementTagPoint,
                                          vtkm::TopologyElementTagCell>
{
  //only specify the shape type as it will be constant as everything
  //is a vertex. otherwise use the defaults.
  using ConnectivityType = vtkm::cont::detail::DefaultVisitPointsWithCellsConnectivityExplicit;
};

} // namespace detail

/// \cond
/// Make doxygen ignore this section
#ifndef vtk_m_cont_CellSetExplicit_cxx
extern template class VTKM_CONT_TEMPLATE_EXPORT CellSetExplicit<>; // default
extern template class VTKM_CONT_TEMPLATE_EXPORT CellSetExplicit<
  typename vtkm::cont::ArrayHandleConstant<vtkm::UInt8>::StorageTag,
  VTKM_DEFAULT_CONNECTIVITY_STORAGE_TAG,
  typename vtkm::cont::ArrayHandleCounting<vtkm::Id>::StorageTag>; // CellSetSingleType base
#endif
/// \endcond
}
} // namespace vtkm::cont

//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <typename SST, typename CST, typename OST>
struct SerializableTypeString<vtkm::cont::CellSetExplicit<SST, CST, OST>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "CS_Explicit<" +
      SerializableTypeString<vtkm::cont::ArrayHandle<vtkm::UInt8, SST>>::Get() + "_ST," +
      SerializableTypeString<vtkm::cont::ArrayHandle<vtkm::Id, CST>>::Get() + "_ST," +
      SerializableTypeString<vtkm::cont::ArrayHandle<vtkm::Id, OST>>::Get() + "_ST>";

    return name;
  }
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename SST, typename CST, typename OST>
struct Serialization<vtkm::cont::CellSetExplicit<SST, CST, OST>>
{
private:
  using Type = vtkm::cont::CellSetExplicit<SST, CST, OST>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    vtkmdiy::save(bb, cs.GetNumberOfPoints());
    vtkmdiy::save(
      bb, cs.GetShapesArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{}));
    vtkmdiy::save(
      bb, cs.GetConnectivityArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{}));
    vtkmdiy::save(
      bb, cs.GetOffsetsArray(vtkm::TopologyElementTagCell{}, vtkm::TopologyElementTagPoint{}));
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    vtkm::Id numberOfPoints = 0;
    vtkmdiy::load(bb, numberOfPoints);
    vtkm::cont::ArrayHandle<vtkm::UInt8, SST> shapes;
    vtkmdiy::load(bb, shapes);
    vtkm::cont::ArrayHandle<vtkm::Id, CST> connectivity;
    vtkmdiy::load(bb, connectivity);
    vtkm::cont::ArrayHandle<vtkm::Id, OST> offsets;
    vtkmdiy::load(bb, offsets);

    cs = Type{};
    cs.Fill(numberOfPoints, shapes, connectivity, offsets);
  }
};

} // diy
/// @endcond SERIALIZATION

#ifndef vtk_m_cont_CellSetExplicit_hxx
#include <vtkm/cont/CellSetExplicit.hxx>
#endif //vtk_m_cont_CellSetExplicit_hxx

#endif //vtk_m_cont_CellSetExplicit_h
