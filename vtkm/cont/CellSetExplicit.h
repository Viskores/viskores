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

  using PointToCellInternalsType =
    typename ConnectivityChooser<vtkm::TopologyElementTagPoint,
                                 vtkm::TopologyElementTagCell>::ConnectivityType;

  using CellToPointInternalsType =
    typename ConnectivityChooser<vtkm::TopologyElementTagCell,
                                 vtkm::TopologyElementTagPoint>::ConnectivityType;

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
  VTKM_CONT CellSetExplicit(Thisclass&& src) noexcept;

  VTKM_CONT Thisclass& operator=(const Thisclass& src);
  VTKM_CONT Thisclass& operator=(Thisclass&& src) noexcept;

  virtual ~CellSetExplicit();

  vtkm::Id GetNumberOfCells() const override;
  vtkm::Id GetNumberOfPoints() const override;
  vtkm::Id GetNumberOfFaces() const override;
  vtkm::Id GetNumberOfEdges() const override;
  void PrintSummary(std::ostream& out) const override;
  void ReleaseResourcesExecution() override;

  std::shared_ptr<CellSet> NewInstance() const override;
  void DeepCopy(const CellSet* src) override;

  VTKM_CONT vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagCell) const;
  VTKM_CONT vtkm::Id GetSchedulingRange(vtkm::TopologyElementTagPoint) const;

  VTKM_CONT vtkm::IdComponent GetNumberOfPointsInCell(vtkm::Id cellIndex) const override;
  void GetCellPointIds(vtkm::Id id, vtkm::Id* ptids) const override;

  VTKM_CONT vtkm::UInt8 GetCellShape(vtkm::Id cellIndex) const override;

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

  // Can be used to check if e.g. CellToPoint table is built.
  template <typename FromTopology, typename ToTopology>
  VTKM_CONT bool HasConnectivity(FromTopology from, ToTopology to) const
  {
    return this->HasConnectivityImpl(from, to);
  }

  // Can be used to reset a connectivity table, mostly useful for benchmarking.
  template <typename FromTopology, typename ToTopology>
  VTKM_CONT void ResetConnectivity(FromTopology from, ToTopology to)
  {
    this->ResetConnectivityImpl(from, to);
  }

protected:
  VTKM_CONT void BuildConnectivity(vtkm::cont::DeviceAdapterId,
                                   vtkm::TopologyElementTagPoint,
                                   vtkm::TopologyElementTagCell) const;

  VTKM_CONT void BuildConnectivity(vtkm::cont::DeviceAdapterId,
                                   vtkm::TopologyElementTagCell,
                                   vtkm::TopologyElementTagPoint) const;

  VTKM_CONT bool HasConnectivityImpl(vtkm::TopologyElementTagPoint,
                                     vtkm::TopologyElementTagCell) const
  {
    return this->Data->PointToCell.ElementsValid;
  }

  VTKM_CONT bool HasConnectivityImpl(vtkm::TopologyElementTagCell,
                                     vtkm::TopologyElementTagPoint) const
  {
    return this->Data->CellToPoint.ElementsValid;
  }

  VTKM_CONT void ResetConnectivityImpl(vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell)
  {
    // Reset entire cell set
    this->Data->PointToCell = PointToCellInternalsType{};
    this->Data->CellToPoint = CellToPointInternalsType{};
    this->Data->ConnectivityAdded = -1;
    this->Data->NumberOfCellsAdded = -1;
    this->Data->NumberOfPoints = 0;
  }

  VTKM_CONT void ResetConnectivityImpl(vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint)
  {
    this->Data->CellToPoint = CellToPointInternalsType{};
  }

  // Store internals in a shared pointer so shallow copies stay consistent.
  // See #2268.
  struct Internals
  {
    PointToCellInternalsType PointToCell;
    CellToPointInternalsType CellToPoint;

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
  const PointToCellInternalsType& GetConnectivity(vtkm::TopologyElementTagPoint,
                                                  vtkm::TopologyElementTagCell) const
  {
    return this->Data->PointToCell;
  }

  const PointToCellInternalsType& GetConnectivity(vtkm::TopologyElementTagPoint,
                                                  vtkm::TopologyElementTagCell)
  {
    return this->Data->PointToCell;
  }

  const CellToPointInternalsType& GetConnectivity(vtkm::TopologyElementTagCell,
                                                  vtkm::TopologyElementTagPoint) const
  {
    return this->Data->CellToPoint;
  }

  const CellToPointInternalsType& GetConnectivity(vtkm::TopologyElementTagCell,
                                                  vtkm::TopologyElementTagPoint)
  {
    return this->Data->CellToPoint;
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

//=============================================================================
// Specializations of serialization related classes
namespace vtkm
{
namespace cont
{

template <typename ShapeST, typename CountST, typename ConnectivityST, typename OffsetST>
struct SerializableTypeString<
  vtkm::cont::CellSetExplicit<ShapeST, CountST, ConnectivityST, OffsetST>>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "CS_Explicit<" +
      SerializableTypeString<vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeST>>::Get() + "_ST," +
      SerializableTypeString<vtkm::cont::ArrayHandle<vtkm::IdComponent, CountST>>::Get() + "_ST," +
      SerializableTypeString<vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityST>>::Get() + "_ST," +
      SerializableTypeString<vtkm::cont::ArrayHandle<vtkm::Id, OffsetST>>::Get() + "_ST>";

    return name;
  }
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <typename ShapeST, typename CountST, typename ConnectivityST, typename OffsetST>
struct Serialization<vtkm::cont::CellSetExplicit<ShapeST, CountST, ConnectivityST, OffsetST>>
{
private:
  using Type = vtkm::cont::CellSetExplicit<ShapeST, CountST, ConnectivityST, OffsetST>;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    vtkmdiy::save(bb, cs.GetName());
    vtkmdiy::save(bb, cs.GetNumberOfPoints());
    vtkmdiy::save(
      bb, cs.GetShapesArray(vtkm::TopologyElementTagPoint{}, vtkm::TopologyElementTagCell{}));
    vtkmdiy::save(
      bb, cs.GetNumIndicesArray(vtkm::TopologyElementTagPoint{}, vtkm::TopologyElementTagCell{}));
    vtkmdiy::save(
      bb, cs.GetConnectivityArray(vtkm::TopologyElementTagPoint{}, vtkm::TopologyElementTagCell{}));
    vtkmdiy::save(
      bb, cs.GetIndexOffsetArray(vtkm::TopologyElementTagPoint{}, vtkm::TopologyElementTagCell{}));
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    std::string name;
    vtkmdiy::load(bb, name);
    vtkm::Id numberOfPoints = 0;
    vtkmdiy::load(bb, numberOfPoints);
    vtkm::cont::ArrayHandle<vtkm::UInt8, ShapeST> shapes;
    vtkmdiy::load(bb, shapes);
    vtkm::cont::ArrayHandle<vtkm::IdComponent, CountST> counts;
    vtkmdiy::load(bb, counts);
    vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityST> connectivity;
    vtkmdiy::load(bb, connectivity);
    vtkm::cont::ArrayHandle<vtkm::Id, OffsetST> offsets;
    vtkmdiy::load(bb, offsets);

    cs = Type(name);
    cs.Fill(numberOfPoints, shapes, counts, connectivity, offsets);
  }
};

} // diy

#include <vtkm/cont/CellSetExplicit.hxx>

#endif //vtk_m_cont_CellSetExplicit_h
