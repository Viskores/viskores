//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_cont_CellSetExtrude_h
#define vtk_m_cont_CellSetExtrude_h

#include <vtkm/TopologyElementTag.h>
#include <vtkm/cont/ArrayHandle.h>
#include <vtkm/cont/ArrayHandleCounting.h>
#include <vtkm/cont/ArrayHandleXGCCoordinates.h>
#include <vtkm/cont/CellSet.h>
#include <vtkm/cont/Invoker.h>
#include <vtkm/exec/ConnectivityExtrude.h>
#include <vtkm/exec/arg/ThreadIndicesExtrude.h>

namespace vtkm
{
namespace cont
{

namespace detail
{

template <typename VisitTopology, typename IncidentTopology>
struct CellSetExtrudeConnectivityChooser;

template <>
struct CellSetExtrudeConnectivityChooser<vtkm::TopologyElementTagCell,
                                         vtkm::TopologyElementTagPoint>
{
  using ExecConnectivityType = vtkm::exec::ConnectivityExtrude;
};

template <>
struct CellSetExtrudeConnectivityChooser<vtkm::TopologyElementTagPoint,
                                         vtkm::TopologyElementTagCell>
{
  using ExecConnectivityType = vtkm::exec::ReverseConnectivityExtrude;
};

} // namespace detail

/// @brief Defines a 3-dimensional extruded mesh representation.
///
/// `CellSetExtrude` takes takes a mesh defined in the XZ-plane and extrudes it along
/// the Y-axis. This plane is repeated in a series of steps and forms wedge cells
/// between them.
///
/// The extrusion can be linear or rotational (e.g., to form a torus).
class VTKM_CONT_EXPORT CellSetExtrude : public CellSet
{
public:
  VTKM_CONT CellSetExtrude();

  VTKM_CONT CellSetExtrude(const vtkm::cont::ArrayHandle<vtkm::Int32>& conn,
                           vtkm::Int32 numberOfPointsPerPlane,
                           vtkm::Int32 numberOfPlanes,
                           const vtkm::cont::ArrayHandle<vtkm::Int32>& nextNode,
                           bool periodic);

  VTKM_CONT CellSetExtrude(const CellSetExtrude& src);
  VTKM_CONT CellSetExtrude(CellSetExtrude&& src) noexcept;

  VTKM_CONT CellSetExtrude& operator=(const CellSetExtrude& src);
  VTKM_CONT CellSetExtrude& operator=(CellSetExtrude&& src) noexcept;

  ~CellSetExtrude() override;

  vtkm::Int32 GetNumberOfPlanes() const;

  vtkm::Id GetNumberOfCells() const override;

  vtkm::Id GetNumberOfPoints() const override;

  vtkm::Id GetNumberOfFaces() const override;

  vtkm::Id GetNumberOfEdges() const override;

  VTKM_CONT vtkm::Id2 GetSchedulingRange(vtkm::TopologyElementTagCell) const;

  VTKM_CONT vtkm::Id2 GetSchedulingRange(vtkm::TopologyElementTagPoint) const;

  vtkm::UInt8 GetCellShape(vtkm::Id id) const override;
  vtkm::IdComponent GetNumberOfPointsInCell(vtkm::Id id) const override;
  void GetCellPointIds(vtkm::Id id, vtkm::Id* ptids) const override;

  std::shared_ptr<CellSet> NewInstance() const override;
  void DeepCopy(const CellSet* src) override;

  void PrintSummary(std::ostream& out) const override;
  void ReleaseResourcesExecution() override;

  const vtkm::cont::ArrayHandle<vtkm::Int32>& GetConnectivityArray() const
  {
    return this->Connectivity;
  }

  vtkm::Int32 GetNumberOfPointsPerPlane() const { return this->NumberOfPointsPerPlane; }

  const vtkm::cont::ArrayHandle<vtkm::Int32>& GetNextNodeArray() const { return this->NextNode; }

  bool GetIsPeriodic() const { return this->IsPeriodic; }

  template <vtkm::IdComponent NumIndices>
  VTKM_CONT void GetIndices(vtkm::Id index, vtkm::Vec<vtkm::Id, NumIndices>& ids) const;

  VTKM_CONT void GetIndices(vtkm::Id index, vtkm::cont::ArrayHandle<vtkm::Id>& ids) const;

  template <typename VisitTopology, typename IncidentTopology>
  using ExecConnectivityType =
    typename detail::CellSetExtrudeConnectivityChooser<VisitTopology,
                                                       IncidentTopology>::ExecConnectivityType;

  vtkm::exec::ConnectivityExtrude PrepareForInput(vtkm::cont::DeviceAdapterId,
                                                  vtkm::TopologyElementTagCell,
                                                  vtkm::TopologyElementTagPoint,
                                                  vtkm::cont::Token&) const;

  vtkm::exec::ReverseConnectivityExtrude PrepareForInput(vtkm::cont::DeviceAdapterId,
                                                         vtkm::TopologyElementTagPoint,
                                                         vtkm::TopologyElementTagCell,
                                                         vtkm::cont::Token&) const;

private:
  void BuildReverseConnectivity();

  bool IsPeriodic;

  vtkm::Int32 NumberOfPointsPerPlane;
  vtkm::Int32 NumberOfCellsPerPlane;
  vtkm::Int32 NumberOfPlanes;
  vtkm::cont::ArrayHandle<vtkm::Int32> Connectivity;
  vtkm::cont::ArrayHandle<vtkm::Int32> NextNode;

  bool ReverseConnectivityBuilt;
  vtkm::cont::ArrayHandle<vtkm::Int32> RConnectivity;
  vtkm::cont::ArrayHandle<vtkm::Int32> ROffsets;
  vtkm::cont::ArrayHandle<vtkm::Int32> RCounts;
  vtkm::cont::ArrayHandle<vtkm::Int32> PrevNode;
};

template <typename T>
CellSetExtrude make_CellSetExtrude(const vtkm::cont::ArrayHandle<vtkm::Int32>& conn,
                                   const vtkm::cont::ArrayHandleXGCCoordinates<T>& coords,
                                   const vtkm::cont::ArrayHandle<vtkm::Int32>& nextNode,
                                   bool periodic = true)
{
  return CellSetExtrude{
    conn, coords.GetNumberOfPointsPerPlane(), coords.GetNumberOfPlanes(), nextNode, periodic
  };
}

template <typename T>
CellSetExtrude make_CellSetExtrude(const std::vector<vtkm::Int32>& conn,
                                   const vtkm::cont::ArrayHandleXGCCoordinates<T>& coords,
                                   const std::vector<vtkm::Int32>& nextNode,
                                   bool periodic = true)
{
  return CellSetExtrude{ vtkm::cont::make_ArrayHandle(conn, vtkm::CopyFlag::On),
                         static_cast<vtkm::Int32>(coords.GetNumberOfPointsPerPlane()),
                         static_cast<vtkm::Int32>(coords.GetNumberOfPlanes()),
                         vtkm::cont::make_ArrayHandle(nextNode, vtkm::CopyFlag::On),
                         periodic };
}

template <typename T>
CellSetExtrude make_CellSetExtrude(std::vector<vtkm::Int32>&& conn,
                                   const vtkm::cont::ArrayHandleXGCCoordinates<T>& coords,
                                   std::vector<vtkm::Int32>&& nextNode,
                                   bool periodic = true)
{
  return CellSetExtrude{ vtkm::cont::make_ArrayHandleMove(std::move(conn)),
                         static_cast<vtkm::Int32>(coords.GetNumberOfPointsPerPlane()),
                         static_cast<vtkm::Int32>(coords.GetNumberOfPlanes()),
                         vtkm::cont::make_ArrayHandleMove(std::move(nextNode)),
                         periodic };
}
}
} // vtkm::cont


//=============================================================================
// Specializations of serialization related classes
/// @cond SERIALIZATION
namespace vtkm
{
namespace cont
{

template <>
struct SerializableTypeString<vtkm::cont::CellSetExtrude>
{
  static VTKM_CONT const std::string& Get()
  {
    static std::string name = "CS_Extrude";
    return name;
  }
};
}
} // vtkm::cont

namespace mangled_diy_namespace
{

template <>
struct Serialization<vtkm::cont::CellSetExtrude>
{
private:
  using Type = vtkm::cont::CellSetExtrude;

public:
  static VTKM_CONT void save(BinaryBuffer& bb, const Type& cs)
  {
    vtkmdiy::save(bb, cs.GetNumberOfPointsPerPlane());
    vtkmdiy::save(bb, cs.GetNumberOfPlanes());
    vtkmdiy::save(bb, cs.GetIsPeriodic());
    vtkmdiy::save(bb, cs.GetConnectivityArray());
    vtkmdiy::save(bb, cs.GetNextNodeArray());
  }

  static VTKM_CONT void load(BinaryBuffer& bb, Type& cs)
  {
    vtkm::Int32 numberOfPointsPerPlane;
    vtkm::Int32 numberOfPlanes;
    bool isPeriodic;
    vtkm::cont::ArrayHandle<vtkm::Int32> conn;
    vtkm::cont::ArrayHandle<vtkm::Int32> nextNode;

    vtkmdiy::load(bb, numberOfPointsPerPlane);
    vtkmdiy::load(bb, numberOfPlanes);
    vtkmdiy::load(bb, isPeriodic);
    vtkmdiy::load(bb, conn);
    vtkmdiy::load(bb, nextNode);

    cs = Type{ conn, numberOfPointsPerPlane, numberOfPlanes, nextNode, isPeriodic };
  }
};

} // diy
/// @endcond SERIALIZATION

#endif // vtk_m_cont_CellSetExtrude.h
