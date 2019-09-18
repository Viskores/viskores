//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_FetchExtrude_h
#define vtk_m_exec_arg_FetchExtrude_h

#include <vtkm/exec/ConnectivityExtrude.h>
#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <vtkm/exec/arg/FetchTagArrayTopologyMapIn.h>
#include <vtkm/exec/arg/IncidentElementIndices.h>

//optimized fetches for ArrayPortalExtrude for
// - 3D Scheduling
// - WorkletNeighboorhood
namespace vtkm
{
namespace exec
{
namespace arg
{

//Optimized fetch for point ids when iterating the cells ConnectivityExtrude
template <typename FetchType, typename Device, typename ExecObjectType>
struct Fetch<FetchType,
             vtkm::exec::arg::AspectTagIncidentElementIndices,
             vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ConnectivityExtrude<Device>>,
             ExecObjectType>
{
  using ThreadIndicesType =
    vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ConnectivityExtrude<Device>>;

  using ValueType = vtkm::Vec<vtkm::Id, 6>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType&) const
  {
    // std::cout << "opimized fetch for point ids" << std::endl;
    const auto& xgcidx = indices.GetIndicesIncident();
    const vtkm::Id offset1 = (xgcidx.Planes[0] * xgcidx.NumberOfPointsPerPlane);
    const vtkm::Id offset2 = (xgcidx.Planes[1] * xgcidx.NumberOfPointsPerPlane);
    ValueType result;
    result[0] = offset1 + xgcidx.PointIds[0][0];
    result[1] = offset1 + xgcidx.PointIds[0][1];
    result[2] = offset1 + xgcidx.PointIds[0][2];
    result[3] = offset2 + xgcidx.PointIds[1][0];
    result[4] = offset2 + xgcidx.PointIds[1][1];
    result[5] = offset2 + xgcidx.PointIds[1][2];
    return result;
  }

  VTKM_EXEC
  void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op.
  }
};

//Optimized fetch for point arrays when iterating the cells ConnectivityExtrude
template <typename Device, typename PortalType>
struct Fetch<vtkm::exec::arg::FetchTagArrayTopologyMapIn,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ConnectivityExtrude<Device>>,
             PortalType>
{
  using ThreadIndicesType =
    vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ConnectivityExtrude<Device>>;
  using ValueType = vtkm::Vec<typename PortalType::ValueType, 6>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const PortalType& portal)
  {
    // std::cout << "opimized fetch for point values" << std::endl;
    const auto& xgcidx = indices.GetIndicesIncident();
    const vtkm::Id offset1 = (xgcidx.Planes[0] * xgcidx.NumberOfPointsPerPlane);
    const vtkm::Id offset2 = (xgcidx.Planes[1] * xgcidx.NumberOfPointsPerPlane);
    ValueType result;
    result[0] = portal.Get(offset1 + xgcidx.PointIds[0][0]);
    result[1] = portal.Get(offset1 + xgcidx.PointIds[0][1]);
    result[2] = portal.Get(offset1 + xgcidx.PointIds[0][2]);
    result[3] = portal.Get(offset2 + xgcidx.PointIds[1][0]);
    result[4] = portal.Get(offset2 + xgcidx.PointIds[1][1]);
    result[5] = portal.Get(offset2 + xgcidx.PointIds[1][2]);
    return result;
  }

  VTKM_EXEC
  void Store(const ThreadIndicesType&, const PortalType&, const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

//Optimized fetch for point coordinates when iterating the cells of ConnectivityExtrude
template <typename Device, typename T>
struct Fetch<vtkm::exec::arg::FetchTagArrayTopologyMapIn,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ConnectivityExtrude<Device>>,
             vtkm::exec::ArrayPortalExtrude<T>>

{
  using ThreadIndicesType =
    vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ConnectivityExtrude<Device>>;
  using ValueType = vtkm::Vec<typename vtkm::exec::ArrayPortalExtrude<T>::ValueType, 6>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const vtkm::exec::ArrayPortalExtrude<T>& points)
  {
    // std::cout << "opimized fetch for point coordinates" << std::endl;
    return points.GetWedge(indices.GetIndicesIncident());
  }

  VTKM_EXEC
  void Store(const ThreadIndicesType&,
             const vtkm::exec::ArrayPortalExtrude<T>&,
             const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

//Optimized fetch for point coordinates when iterating the cells of ConnectivityExtrude
template <typename Device, typename T>
struct Fetch<vtkm::exec::arg::FetchTagArrayTopologyMapIn,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ConnectivityExtrude<Device>>,
             vtkm::exec::ArrayPortalExtrudePlane<T>>
{
  using ThreadIndicesType =
    vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ConnectivityExtrude<Device>>;
  using ValueType = vtkm::Vec<typename vtkm::exec::ArrayPortalExtrudePlane<T>::ValueType, 6>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices,
                 const vtkm::exec::ArrayPortalExtrudePlane<T>& portal)
  {
    // std::cout << "opimized fetch for point coordinates" << std::endl;
    return portal.GetWedge(indices.GetIndicesIncident());
  }

  VTKM_EXEC
  void Store(const ThreadIndicesType&,
             const vtkm::exec::ArrayPortalExtrudePlane<T>&,
             const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

//Optimized fetch for point coordinates when iterating the points of ConnectivityExtrude
template <typename Device, typename T>
struct Fetch<
  vtkm::exec::arg::FetchTagArrayDirectIn,
  vtkm::exec::arg::AspectTagDefault,
  vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ReverseConnectivityExtrude<Device>>,
  vtkm::exec::ArrayPortalExtrude<T>>

{
  using ThreadIndicesType =
    vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ReverseConnectivityExtrude<Device>>;
  using ValueType = typename vtkm::exec::ArrayPortalExtrude<T>::ValueType;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const vtkm::exec::ArrayPortalExtrude<T>& points)
  {
    // std::cout << "opimized fetch for point coordinates" << std::endl;
    return points.Get(indices.GetIndexLogical());
  }

  VTKM_EXEC
  void Store(const ThreadIndicesType&,
             const vtkm::exec::ArrayPortalExtrude<T>&,
             const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
}


#endif
