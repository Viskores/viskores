//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_FieldNeighborhood_h
#define vtk_m_exec_FieldNeighborhood_h

#include <vtkm/exec/BoundaryState.h>
#include <vtkm/internal/ArrayPortalUniformPointCoordinates.h>

namespace vtkm
{
namespace exec
{

/// \brief Retrieves field values from a neighborhood.
///
/// \c FieldNeighborhood manages the retrieval of field values within the neighborhood of a
/// \c WorkletPointNeighborhood worklet. The \c Get methods take ijk indices relative to the
/// neighborhood (with 0, 0, 0 being the element visted) and return the field value at that part of
/// the neighborhood. If the requested neighborhood is outside the boundary, a different value will
/// be returned determined by the boundary behavior. A \c BoundaryState object can be used to
/// determine if the neighborhood extends beyond the boundary of the mesh.
///
/// This class is typically constructued using the \c FieldInNeighborhood tag in an
/// \c ExecutionSignature. There is little reason to construct this in user code.
///
/// \c FieldNeighborhood is templated on the array portal from which field values are retrieved.
///
template <typename FieldPortalType>
struct FieldNeighborhood
{
  VTKM_EXEC
  FieldNeighborhood(const FieldPortalType& portal, const vtkm::exec::BoundaryState& boundary)
    : Boundary(&boundary)
    , Portal(portal)
  {
  }

  using ValueType = typename FieldPortalType::ValueType;

  VTKM_EXEC
  ValueType Get(vtkm::IdComponent i, vtkm::IdComponent j, vtkm::IdComponent k) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFlatIndexClamp(i, j, k));
  }

  VTKM_EXEC
  ValueType Get(const vtkm::Id3& ijk) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFlatIndexClamp(ijk));
  }

  vtkm::exec::BoundaryState const* const Boundary;
  FieldPortalType Portal;
};

/// \brief Specialization of Neighborhood for ArrayPortalUniformPointCoordinates
/// We can use fast paths inside ArrayPortalUniformPointCoordinates to allow
/// for very fast computation of the coordinates reachable by the neighborhood
template <>
struct FieldNeighborhood<vtkm::internal::ArrayPortalUniformPointCoordinates>
{
  VTKM_EXEC
  FieldNeighborhood(const vtkm::internal::ArrayPortalUniformPointCoordinates& portal,
                    const vtkm::exec::BoundaryState& boundary)
    : Boundary(&boundary)
    , Portal(portal)
  {
  }

  using ValueType = vtkm::internal::ArrayPortalUniformPointCoordinates::ValueType;

  VTKM_EXEC
  ValueType Get(vtkm::IdComponent i, vtkm::IdComponent j, vtkm::IdComponent k) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFullIndexClamp(i, j, k));
  }

  VTKM_EXEC
  ValueType Get(const vtkm::IdComponent3& ijk) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFullIndexClamp(ijk));
  }

  vtkm::exec::BoundaryState const* const Boundary;
  vtkm::internal::ArrayPortalUniformPointCoordinates Portal;
};
}
} // namespace vtkm::exec

#endif //vtk_m_exec_FieldNeighborhood_h
