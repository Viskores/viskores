//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_ConnectivityExplicit_h
#define vtk_m_exec_ConnectivityExplicit_h

#include <vtkm/CellShape.h>
#include <vtkm/Types.h>

#include <vtkm/VecFromPortal.h>

namespace vtkm
{
namespace exec
{

template <typename ShapesPortalType, typename ConnectivityPortalType, typename OffsetsPortalType>
class ConnectivityExplicit
{
public:
  using SchedulingRangeType = vtkm::Id;

  ConnectivityExplicit() {}

  ConnectivityExplicit(const ShapesPortalType& shapesPortal,
                       const ConnectivityPortalType& connPortal,
                       const OffsetsPortalType& offsetsPortal)
    : Shapes(shapesPortal)
    , Connectivity(connPortal)
    , Offsets(offsetsPortal)
  {
  }

  VTKM_EXEC
  SchedulingRangeType GetNumberOfElements() const { return this->Shapes.GetNumberOfValues(); }

  using CellShapeTag = vtkm::CellShapeTagGeneric;

  VTKM_EXEC
  CellShapeTag GetCellShape(vtkm::Id index) const { return CellShapeTag(this->Shapes.Get(index)); }

  VTKM_EXEC
  vtkm::IdComponent GetNumberOfIndices(vtkm::Id index) const
  {
    return static_cast<vtkm::IdComponent>(this->Offsets.Get(index + 1) - this->Offsets.Get(index));
  }

  using IndicesType = vtkm::VecFromPortal<ConnectivityPortalType>;

  /// Returns a Vec-like object containing the indices for the given index.
  /// The object returned is not an actual array, but rather an object that
  /// loads the indices lazily out of the connectivity array. This prevents
  /// us from having to know the number of indices at compile time.
  ///
  VTKM_EXEC
  IndicesType GetIndices(vtkm::Id index) const
  {
    const vtkm::Id offset = this->Offsets.Get(index);
    const vtkm::Id endOffset = this->Offsets.Get(index + 1);
    const auto length = static_cast<vtkm::IdComponent>(endOffset - offset);

    return IndicesType(this->Connectivity, length, offset);
  }

private:
  ShapesPortalType Shapes;
  ConnectivityPortalType Connectivity;
  OffsetsPortalType Offsets;
};

} // namespace exec
} // namespace vtkm

#endif //  vtk_m_exec_ConnectivityExplicit_h
