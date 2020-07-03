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
template <typename FetchType, typename ExecObjectType>
struct Fetch<FetchType, vtkm::exec::arg::AspectTagIncidentElementIndices, ExecObjectType>
{
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Device, typename ScatterAndMaskMode>
  VTKM_EXEC auto Load(
    const vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ConnectivityExtrude<Device>,
                                                    ScatterAndMaskMode>& indices,
    const ExecObjectType&) const -> vtkm::Vec<vtkm::Id, 6>
  {
    // std::cout << "opimized fetch for point ids" << std::endl;
    const auto& xgcidx = indices.GetIndicesIncident();
    const vtkm::Id offset1 = (xgcidx.Planes[0] * xgcidx.NumberOfPointsPerPlane);
    const vtkm::Id offset2 = (xgcidx.Planes[1] * xgcidx.NumberOfPointsPerPlane);
    vtkm::Vec<vtkm::Id, 6> result;
    result[0] = offset1 + xgcidx.PointIds[0][0];
    result[1] = offset1 + xgcidx.PointIds[0][1];
    result[2] = offset1 + xgcidx.PointIds[0][2];
    result[3] = offset2 + xgcidx.PointIds[1][0];
    result[4] = offset2 + xgcidx.PointIds[1][1];
    result[5] = offset2 + xgcidx.PointIds[1][2];
    return result;
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ConnectivityType, typename ScatterAndMaskMode>
  VTKM_EXEC auto Load(
    const vtkm::exec::arg::ThreadIndicesTopologyMap<ConnectivityType, ScatterAndMaskMode>& indices,
    const ExecObjectType&) const -> decltype(indices.GetIndicesIncident())
  {
    return indices.GetIndicesIncident();
  }

  template <typename ThreadIndicesType, typename ValueType>
  VTKM_EXEC void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op.
  }
};

//Optimized fetch for point coordinates when iterating the cells of ConnectivityExtrude
template <typename T>
struct Fetch<vtkm::exec::arg::FetchTagArrayTopologyMapIn,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::ArrayPortalExtrude<T>>

{
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VTKM_EXEC auto Load(const ThreadIndicesType& indices,
                      const vtkm::exec::ArrayPortalExtrude<T>& points)
    -> decltype(points.GetWedge(indices.GetIndicesIncident()))
  {
    // std::cout << "opimized fetch for point coordinates" << std::endl;
    return points.GetWedge(indices.GetIndicesIncident());
  }

  template <typename ThreadIndicesType, typename ValueType>
  VTKM_EXEC void Store(const ThreadIndicesType&,
                       const vtkm::exec::ArrayPortalExtrude<T>&,
                       const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

//Optimized fetch for point coordinates when iterating the cells of ConnectivityExtrude
template <typename T>
struct Fetch<vtkm::exec::arg::FetchTagArrayTopologyMapIn,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::ArrayPortalExtrudePlane<T>>
{
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VTKM_EXEC auto Load(const ThreadIndicesType& indices,
                      const vtkm::exec::ArrayPortalExtrudePlane<T>& portal)
    -> decltype(portal.GetWedge(indices.GetIndicesIncident()))
  {
    // std::cout << "opimized fetch for point coordinates" << std::endl;
    return portal.GetWedge(indices.GetIndicesIncident());
  }

  template <typename ThreadIndicesType, typename ValueType>
  VTKM_EXEC void Store(const ThreadIndicesType&,
                       const vtkm::exec::ArrayPortalExtrudePlane<T>&,
                       const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};

//Optimized fetch for point coordinates when iterating the points of ConnectivityExtrude
template <typename T>
struct Fetch<vtkm::exec::arg::FetchTagArrayDirectIn,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::ArrayPortalExtrude<T>>

{
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VTKM_EXEC auto Load(const ThreadIndicesType& indices,
                      const vtkm::exec::ArrayPortalExtrude<T>& points)
    -> decltype(points.Get(indices.GetInputIndex()))
  {
    // std::cout << "optimized fetch for point coordinates" << std::endl;
    return points.Get(indices.GetInputIndex());
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename Device, typename ScatterAndMaskMode>
  VTKM_EXEC auto Load(
    const vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ReverseConnectivityExtrude<Device>,
                                                    ScatterAndMaskMode>& indices,
    const vtkm::exec::ArrayPortalExtrude<T>& points)
    -> decltype(points.Get(indices.GetIndexLogical()))
  {
    // std::cout << "optimized fetch for point coordinates" << std::endl;
    return points.Get(indices.GetIndexLogical());
  }

  template <typename ThreadIndicesType, typename ValueType>
  VTKM_EXEC void Store(const ThreadIndicesType&,
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
