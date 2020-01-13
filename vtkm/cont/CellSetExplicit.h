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
#include <vtkm/cont/ArrayHandleDecorator.h>
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

template <typename CellSetType, typename VisitTopology, typename IncidentTopology>
struct CellSetExplicitConnectivityChooser
{
  using ConnectivityType = vtkm::cont::internal::ConnectivityExplicitInternals<>;
};

// Used with ArrayHandleDecorator to recover the NumIndices array from the
// offsets.
struct NumIndicesDecorator
{
  template <typename OffsetsPortal>
  struct Functor
  {
    OffsetsPortal Offsets;

    VTKM_EXEC_CONT
    vtkm::IdComponent operator()(vtkm::Id cellId) const
    {
      return static_cast<vtkm::IdComponent>(this->Offsets.Get(cellId + 1) -
                                            this->Offsets.Get(cellId));
    }
  };

  template <typename OffsetsPortal>
  static VTKM_CONT Functor<typename std::decay<OffsetsPortal>::type> CreateFunctor(
    OffsetsPortal&& portal)
  {
    return { std::forward<OffsetsPortal>(portal) };
  }
};

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

template <typename S1, typename S2>
void ConvertNumIndicesToOffsets(const vtkm::cont::ArrayHandle<vtkm::Id, S1>& numIndices,
                                vtkm::cont::ArrayHandle<vtkm::Id, S2>& offsets)
{
  vtkm::cont::Algorithm::ScanExtended(numIndices, offsets);
}

template <typename T, typename S1, typename S2>
void ConvertNumIndicesToOffsets(const vtkm::cont::ArrayHandle<T, S1>& numIndices,
                                vtkm::cont::ArrayHandle<vtkm::Id, S2>& offsets)
{
  const auto castCounts = vtkm::cont::make_ArrayHandleCast<vtkm::Id>(numIndices);
  ConvertNumIndicesToOffsets(castCounts, offsets);
}

template <typename T, typename S1, typename S2>
void ConvertNumIndicesToOffsets(const vtkm::cont::ArrayHandle<T, S1>& numIndices,
                                vtkm::cont::ArrayHandle<vtkm::Id, S2>& offsets,
                                vtkm::Id& connectivitySize /* outparam */)
{
  ConvertNumIndicesToOffsets(numIndices, offsets);
  connectivitySize = vtkm::cont::ArrayGetValue(offsets.GetNumberOfValues() - 1, offsets);
}

template <typename T, typename S>
vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumIndicesToOffsets(
  const vtkm::cont::ArrayHandle<T, S>& numIndices)
{
  vtkm::cont::ArrayHandle<vtkm::Id> offsets;
  ConvertNumIndicesToOffsets(numIndices, offsets);
  return offsets;
}

template <typename T, typename S>
vtkm::cont::ArrayHandle<vtkm::Id> ConvertNumIndicesToOffsets(
  const vtkm::cont::ArrayHandle<T, S>& numIndices,
  vtkm::Id& connectivityLength /* outparam */)
{
  vtkm::cont::ArrayHandle<vtkm::Id> offsets;
  ConvertNumIndicesToOffsets(numIndices, offsets, connectivityLength);
  return offsets;
}

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
    using Chooser = typename detail::CellSetExplicitConnectivityChooser<Thisclass,
                                                                        VisitTopology,
                                                                        IncidentTopology>;

  public:
    using ConnectivityType = typename Chooser::ConnectivityType;
    using ShapesArrayType = typename ConnectivityType::ShapesArrayType;
    using ConnectivityArrayType = typename ConnectivityType::ConnectivityArrayType;
    using OffsetsArrayType = typename ConnectivityType::OffsetsArrayType;

    using NumIndicesArrayType =
      vtkm::cont::ArrayHandleDecorator<detail::NumIndicesDecorator, OffsetsArrayType>;
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

  VTKM_CONT virtual ~CellSetExplicit() override;

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

  VTKM_CONT vtkm::UInt8 GetCellShape(vtkm::Id cellid) const override;

  template <vtkm::IdComponent NumIndices>
  VTKM_CONT void GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id, NumIndices>& ids) const;

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
            const vtkm::cont::ArrayHandle<vtkm::UInt8, ShapesStorageTag>& cellTypes,
            const vtkm::cont::ArrayHandle<vtkm::Id, ConnectivityStorageTag>& connectivity,
            const vtkm::cont::ArrayHandle<vtkm::Id, OffsetsStorageTag>& offsets);

  template <typename Device, typename VisitTopology, typename IncidentTopology>
  struct ExecutionTypes
  {
  private:
    VTKM_IS_DEVICE_ADAPTER_TAG(Device);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(VisitTopology);
    VTKM_IS_TOPOLOGY_ELEMENT_TAG(IncidentTopology);

    using Chooser = ConnectivityChooser<VisitTopology, IncidentTopology>;

    using ShapesAT = typename Chooser::ShapesArrayType;
    using ConnAT = typename Chooser::ConnectivityArrayType;
    using OffsetsAT = typename Chooser::OffsetsArrayType;

    using ShapesET = typename ShapesAT::template ExecutionTypes<Device>;
    using ConnET = typename ConnAT::template ExecutionTypes<Device>;
    using OffsetsET = typename OffsetsAT::template ExecutionTypes<Device>;

  public:
    using ShapesPortalType = typename ShapesET::PortalConst;
    using ConnectivityPortalType = typename ConnET::PortalConst;
    using OffsetsPortalType = typename OffsetsET::PortalConst;

    using ExecObjectType =
      vtkm::exec::ConnectivityExplicit<ShapesPortalType, ConnectivityPortalType, OffsetsPortalType>;
  };

  template <typename Device, typename VisitTopology, typename IncidentTopology>
  VTKM_CONT typename ExecutionTypes<Device, VisitTopology, IncidentTopology>::ExecObjectType
    PrepareForInput(Device, VisitTopology, IncidentTopology) const;

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
                                   vtkm::TopologyElementTagPoint) const;

  VTKM_CONT void BuildConnectivity(vtkm::cont::DeviceAdapterId,
                                   vtkm::TopologyElementTagPoint,
                                   vtkm::TopologyElementTagCell) const;

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

#include <vtkm/cont/CellSetExplicit.hxx>

#endif //vtk_m_cont_CellSetExplicit_h
