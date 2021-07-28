//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_FetchTagArrayTopologyMapIn_h
#define vtk_m_exec_arg_FetchTagArrayTopologyMapIn_h

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ThreadIndicesTopologyMap.h>

#include <vtkm/TopologyElementTag.h>

#include <vtkm/internal/ArrayPortalUniformPointCoordinates.h>

#include <vtkm/VecAxisAlignedPointCoordinates.h>
#include <vtkm/exec/ConnectivityExtrude.h>
#include <vtkm/exec/ConnectivityStructured.h>

#include <vtkm/VecFromPortalPermute.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting array values determined by topology connections.
///
/// \c FetchTagArrayTopologyMapIn is a tag used with the \c Fetch class to
/// retrieve values from an array portal. The fetch uses indexing based on
/// the topology structure used for the input domain.
///
struct FetchTagArrayTopologyMapIn
{
};

/// @cond NONE
namespace detail
{

// This internal class defines how a TopologyMapIn fetch loads from field data
// based on the connectivity class and the object holding the field data. The
// default implementation gets a Vec of indices and an array portal for the
// field and delivers a VecFromPortalPermute. Specializations could have more
// efficient implementations. For example, if the connectivity is structured
// and the field is regular point coordinates, it is much faster to compute the
// field directly.

template <typename ConnectivityType, typename FieldExecObjectType, typename ThreadIndicesType>
struct FetchArrayTopologyMapInImplementation
{
  // stored in a Vec-like object.
  using IndexVecType = typename ThreadIndicesType::IndicesIncidentType;

  // The FieldExecObjectType is expected to behave like an ArrayPortal.
  using PortalType = FieldExecObjectType;

  using ValueType = vtkm::VecFromPortalPermute<IndexVecType, PortalType>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  static ValueType Load(const ThreadIndicesType& indices, const FieldExecObjectType& field)
  {
    // It is important that we give the VecFromPortalPermute (ValueType) a
    // pointer that will stay around during the time the Vec is valid. Thus, we
    // should make sure that indices is a reference that goes up the stack at
    // least as far as the returned VecFromPortalPermute is used.
    return ValueType(indices.GetIndicesIncidentPointer(), field);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  static ValueType Load(const ThreadIndicesType& indices, const FieldExecObjectType* const field)
  {
    // It is important that we give the VecFromPortalPermute (ValueType) a
    // pointer that will stay around during the time the Vec is valid. Thus, we
    // should make sure that indices is a reference that goes up the stack at
    // least as far as the returned VecFromPortalPermute is used.
    return ValueType(indices.GetIndicesIncidentPointer(), field);
  }
};

static inline VTKM_EXEC vtkm::VecAxisAlignedPointCoordinates<1> make_VecAxisAlignedPointCoordinates(
  const vtkm::Vec3f& origin,
  const vtkm::Vec3f& spacing,
  const vtkm::Vec<vtkm::Id, 1>& logicalId)
{
  vtkm::Vec3f offsetOrigin(
    origin[0] + spacing[0] * static_cast<vtkm::FloatDefault>(logicalId[0]), origin[1], origin[2]);
  return vtkm::VecAxisAlignedPointCoordinates<1>(offsetOrigin, spacing);
}

static inline VTKM_EXEC vtkm::VecAxisAlignedPointCoordinates<1> make_VecAxisAlignedPointCoordinates(
  const vtkm::Vec3f& origin,
  const vtkm::Vec3f& spacing,
  vtkm::Id logicalId)
{
  return make_VecAxisAlignedPointCoordinates(origin, spacing, vtkm::Vec<vtkm::Id, 1>(logicalId));
}

static inline VTKM_EXEC vtkm::VecAxisAlignedPointCoordinates<2> make_VecAxisAlignedPointCoordinates(
  const vtkm::Vec3f& origin,
  const vtkm::Vec3f& spacing,
  const vtkm::Id2& logicalId)
{
  vtkm::Vec3f offsetOrigin(origin[0] + spacing[0] * static_cast<vtkm::FloatDefault>(logicalId[0]),
                           origin[1] + spacing[1] * static_cast<vtkm::FloatDefault>(logicalId[1]),
                           origin[2]);
  return vtkm::VecAxisAlignedPointCoordinates<2>(offsetOrigin, spacing);
}

static inline VTKM_EXEC vtkm::VecAxisAlignedPointCoordinates<3> make_VecAxisAlignedPointCoordinates(
  const vtkm::Vec3f& origin,
  const vtkm::Vec3f& spacing,
  const vtkm::Id3& logicalId)
{
  vtkm::Vec3f offsetOrigin(origin[0] + spacing[0] * static_cast<vtkm::FloatDefault>(logicalId[0]),
                           origin[1] + spacing[1] * static_cast<vtkm::FloatDefault>(logicalId[1]),
                           origin[2] + spacing[2] * static_cast<vtkm::FloatDefault>(logicalId[2]));
  return vtkm::VecAxisAlignedPointCoordinates<3>(offsetOrigin, spacing);
}

template <vtkm::IdComponent NumDimensions, typename ThreadIndicesType>
struct FetchArrayTopologyMapInImplementation<
  vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                     vtkm::TopologyElementTagPoint,
                                     NumDimensions>,
  vtkm::internal::ArrayPortalUniformPointCoordinates,
  ThreadIndicesType>

{
  using ConnectivityType = vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                                              vtkm::TopologyElementTagPoint,
                                                              NumDimensions>;

  using ValueType = vtkm::VecAxisAlignedPointCoordinates<NumDimensions>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  static ValueType Load(const ThreadIndicesType& indices,
                        const vtkm::internal::ArrayPortalUniformPointCoordinates& field)
  {
    // This works because the logical cell index is the same as the logical
    // point index of the first point on the cell.
    return vtkm::exec::arg::detail::make_VecAxisAlignedPointCoordinates(
      field.GetOrigin(), field.GetSpacing(), indices.GetIndexLogical());
  }
};

template <typename PermutationPortal, vtkm::IdComponent NumDimensions, typename ThreadIndicesType>
struct FetchArrayTopologyMapInImplementation<
  vtkm::exec::ConnectivityPermutedVisitCellsWithPoints<
    PermutationPortal,
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                       vtkm::TopologyElementTagPoint,
                                       NumDimensions>>,
  vtkm::internal::ArrayPortalUniformPointCoordinates,
  ThreadIndicesType>

{
  using ConnectivityType = vtkm::exec::ConnectivityPermutedVisitCellsWithPoints<
    PermutationPortal,
    vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                       vtkm::TopologyElementTagPoint,
                                       NumDimensions>>;

  using ValueType = vtkm::VecAxisAlignedPointCoordinates<NumDimensions>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  static ValueType Load(const ThreadIndicesType& indices,
                        const vtkm::internal::ArrayPortalUniformPointCoordinates& field)
  {
    // This works because the logical cell index is the same as the logical
    // point index of the first point on the cell.

    // we have a flat index but we need 3d uniform coordinates, so we
    // need to take an flat index and convert to logical index
    return vtkm::exec::arg::detail::make_VecAxisAlignedPointCoordinates(
      field.GetOrigin(), field.GetSpacing(), indices.GetIndexLogical());
  }
};

} // namespace detail
/// @endcond

template <typename ExecObjectType>
struct Fetch<vtkm::exec::arg::FetchTagArrayTopologyMapIn,
             vtkm::exec::arg::AspectTagDefault,
             ExecObjectType>
{

  //using ConnectivityType = typename ThreadIndicesType::Connectivity;
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ThreadIndicesType>
  VTKM_EXEC auto Load(const ThreadIndicesType& indices, const ExecObjectType& field) const
    -> decltype(
      detail::FetchArrayTopologyMapInImplementation<typename ThreadIndicesType::Connectivity,
                                                    ExecObjectType,
                                                    ThreadIndicesType>::Load(indices, field))
  {
    using Implementation =
      detail::FetchArrayTopologyMapInImplementation<typename ThreadIndicesType::Connectivity,
                                                    ExecObjectType,
                                                    ThreadIndicesType>;
    return Implementation::Load(indices, field);
  }

  //Optimized fetch for point arrays when iterating the cells ConnectivityExtrude
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename ScatterAndMaskMode>
  VTKM_EXEC auto Load(
    const vtkm::exec::arg::ThreadIndicesTopologyMap<vtkm::exec::ConnectivityExtrude,
                                                    ScatterAndMaskMode>& indices,
    const ExecObjectType& portal) -> vtkm::Vec<typename ExecObjectType::ValueType, 6>
  {
    // std::cout << "opimized fetch for point values" << std::endl;
    const auto& xgcidx = indices.GetIndicesIncident();
    const vtkm::Id offset1 = (xgcidx.Planes[0] * xgcidx.NumberOfPointsPerPlane);
    const vtkm::Id offset2 = (xgcidx.Planes[1] * xgcidx.NumberOfPointsPerPlane);

    using ValueType = vtkm::Vec<typename ExecObjectType::ValueType, 6>;
    ValueType result;

    result[0] = portal.Get(offset1 + xgcidx.PointIds[0][0]);
    result[1] = portal.Get(offset1 + xgcidx.PointIds[0][1]);
    result[2] = portal.Get(offset1 + xgcidx.PointIds[0][2]);
    result[3] = portal.Get(offset2 + xgcidx.PointIds[1][0]);
    result[4] = portal.Get(offset2 + xgcidx.PointIds[1][1]);
    result[5] = portal.Get(offset2 + xgcidx.PointIds[1][2]);
    return result;
  }


  template <typename ThreadIndicesType, typename T>
  VTKM_EXEC void Store(const ThreadIndicesType&, const ExecObjectType&, const T&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FetchTagArrayTopologyMapIn_h
