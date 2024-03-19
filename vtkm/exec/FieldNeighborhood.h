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

/// @brief Retrieves field values from a neighborhood.
///
/// `FieldNeighborhood` manages the retrieval of field values within the neighborhood of a
/// `vtkm::worklet::WorkletPointNeighborhood` worklet. The `Get` methods take ijk indices
/// relative to the neighborhood (with 0, 0, 0 being the element visted) and return the
/// field value at that part of the neighborhood. If the requested neighborhood is outside
/// the boundary, the value at the nearest boundary will be returned. A `vtkm::exec::BoundaryState`
/// object can be used to determine if the neighborhood extends beyond the boundary of the mesh.
///
/// This class is typically constructed using the `FieldInNeighborhood` tag in an
/// `ExecutionSignature`. There is little reason to construct this in user code.
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

  /// @brief Retrieve a field value relative to the visited element.
  ///
  /// The index is given as three dimensional i, j, k indices. These indices are relative
  /// to the currently visited element. So, calling `Get(0, 0, 0)` retrieves the field
  /// value at the visited element. Calling `Get(-1, 0, 0)` retrieves the value to the
  /// "left" and `Get(1, 0, 0)` retrieves the value to the "right."
  ///
  /// If the relative index points outside the bounds of the mesh, `Get` will return the
  /// value closest to the boundary (i.e. clamping behvior). For example, if the visited
  /// element is at the leftmost index of the mesh, `Get(-1, 0, 0)` will refer to a value
  /// outside the bounds of the mesh. In this case, `Get` will return the value at the
  /// visited index, which is the closest element at that boundary.
  ///
  /// When referring to values in a mesh of less than 3 dimensions (such as a 2D structured),
  /// simply use 0 for the unused dimensions.
  VTKM_EXEC
  ValueType Get(vtkm::IdComponent i, vtkm::IdComponent j, vtkm::IdComponent k) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFlatIndexClamp(i, j, k));
  }

  /// @brief Retrieve a field value relative to the visited element without bounds checking.
  ///
  /// `GetUnchecked` behaves the same as `Get` except that no bounds checking is done
  /// before retrieving the field value. If the relative index is out of bounds of the
  /// mesh, the results are undefined.
  ///
  /// `GetUnchecked` is useful in circumstances where the bounds have already be checked.
  /// This prevents wasting time repeating checks.
  VTKM_EXEC
  ValueType GetUnchecked(vtkm::IdComponent i, vtkm::IdComponent j, vtkm::IdComponent k) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFlatIndex(i, j, k));
  }

  /// @copydoc Get
  VTKM_EXEC
  ValueType Get(const vtkm::Id3& ijk) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFlatIndexClamp(ijk));
  }

  /// @copydoc GetUnchecked
  VTKM_EXEC
  ValueType GetUnchecked(const vtkm::Id3& ijk) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFlatIndex(ijk));
  }

  /// The `vtkm::exec::BoundaryState` used to find field values from local indices.
  vtkm::exec::BoundaryState const* const Boundary;

  /// The array portal containing field values.
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
  ValueType GetUnchecked(vtkm::IdComponent i, vtkm::IdComponent j, vtkm::IdComponent k) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFullIndex(i, j, k));
  }

  VTKM_EXEC
  ValueType Get(const vtkm::IdComponent3& ijk) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFullIndexClamp(ijk));
  }

  VTKM_EXEC
  ValueType GetUnchecked(const vtkm::IdComponent3& ijk) const
  {
    return Portal.Get(this->Boundary->NeighborIndexToFullIndex(ijk));
  }

  vtkm::exec::BoundaryState const* const Boundary;
  vtkm::internal::ArrayPortalUniformPointCoordinates Portal;
};
}
} // namespace vtkm::exec

#endif //vtk_m_exec_FieldNeighborhood_h
