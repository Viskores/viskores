//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_arg_FetchTagArrayNeighborhoodIn_h
#define vtk_m_exec_arg_FetchTagArrayNeighborhoodIn_h

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/ThreadIndicesPointNeighborhood.h>
#include <vtkm/internal/ArrayPortalUniformPointCoordinates.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief \c Fetch tag for getting values of neighborhood around a point.
///
/// \c FetchTagArrayNeighborhoodIn is a tag used with the \c Fetch class to retrieve
/// values from an neighborhood.
///
template <int NeighborhoodSize>
struct FetchTagArrayNeighborhoodIn
{
};

template <int NeighborhoodSize, typename ExecObjectType>
struct Neighborhood
{
  VTKM_EXEC
  Neighborhood(const ExecObjectType& portal, const vtkm::exec::arg::BoundaryState& boundary)
    : Boundary(&boundary)
    , Portal(portal)
  {
  }

  using ValueType = typename ExecObjectType::ValueType;

  VTKM_EXEC
  ValueType Get(vtkm::IdComponent i, vtkm::IdComponent j, vtkm::IdComponent k) const
  {
    VTKM_ASSERT(i <= NeighborhoodSize && i >= -NeighborhoodSize);
    VTKM_ASSERT(j <= NeighborhoodSize && j >= -NeighborhoodSize);
    VTKM_ASSERT(k <= NeighborhoodSize && k >= -NeighborhoodSize);
    return Portal.Get(this->Boundary->ClampAndFlatten(i, j, k));
  }

  VTKM_EXEC
  ValueType Get(const vtkm::Id3& ijk) const
  {
    VTKM_ASSERT(ijk[0] <= NeighborhoodSize && ijk[0] >= -NeighborhoodSize);
    VTKM_ASSERT(ijk[1] <= NeighborhoodSize && ijk[1] >= -NeighborhoodSize);
    VTKM_ASSERT(ijk[2] <= NeighborhoodSize && ijk[2] >= -NeighborhoodSize);
    return Portal.Get(this->Boundary->ClampAndFlatten(ijk));
  }

  vtkm::exec::arg::BoundaryState const* const Boundary;
  ExecObjectType Portal;
};

/// \brief Specialization of Neighborhood for ArrayPortalUniformPointCoordinates
/// We can use fast paths inside ArrayPortalUniformPointCoordinates to allow
/// for very fast computation of the coordinates reachable by the neighborhood
template <int NeighborhoodSize>
struct Neighborhood<NeighborhoodSize, vtkm::internal::ArrayPortalUniformPointCoordinates>
{
  VTKM_EXEC
  Neighborhood(const vtkm::internal::ArrayPortalUniformPointCoordinates& portal,
               const vtkm::exec::arg::BoundaryState& boundary)
    : Boundary(&boundary)
    , Portal(portal)
  {
  }

  using ValueType = vtkm::internal::ArrayPortalUniformPointCoordinates::ValueType;

  VTKM_EXEC
  ValueType Get(vtkm::Id i, vtkm::Id j, vtkm::Id k) const
  {
    this->Boundary->Clamp(i, j, k);
    return Portal.Get(vtkm::Id3(i, j, k));
  }

  VTKM_EXEC
  ValueType Get(vtkm::Id3 ijk) const
  {
    this->Boundary->Clamp(ijk);
    return Portal.Get(ijk);
  }

  vtkm::exec::arg::BoundaryState const* const Boundary;
  vtkm::internal::ArrayPortalUniformPointCoordinates Portal;
};

template <int NeighborhoodSize, typename ExecObjectType>
struct Fetch<vtkm::exec::arg::FetchTagArrayNeighborhoodIn<NeighborhoodSize>,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::exec::arg::ThreadIndicesPointNeighborhood<NeighborhoodSize>,
             ExecObjectType>
{
  using ThreadIndicesType = vtkm::exec::arg::ThreadIndicesPointNeighborhood<NeighborhoodSize>;
  using ValueType = Neighborhood<NeighborhoodSize, ExecObjectType>;

  VTKM_SUPPRESS_EXEC_WARNINGS
  VTKM_EXEC
  ValueType Load(const ThreadIndicesType& indices, const ExecObjectType& arrayPortal) const
  {
    return ValueType(arrayPortal, indices.GetBoundaryState());
  }

  VTKM_EXEC
  void Store(const ThreadIndicesType&, const ExecObjectType&, const ValueType&) const
  {
    // Store is a no-op for this fetch.
  }
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FetchTagArrayNeighborhoodIn_h
