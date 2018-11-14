//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2016 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2016 UT-Battelle, LLC.
//  Copyright 2016 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_exec_arg_ThreadIndicesPointNeighborhood_h
#define vtk_m_exec_arg_ThreadIndicesPointNeighborhood_h

#include <vtkm/exec/ConnectivityStructured.h>
#include <vtkm/exec/arg/ThreadIndicesBasic.h>
#include <vtkm/exec/arg/ThreadIndicesTopologyMap.h> //for Deflate and Inflate

#include <vtkm/Math.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief Provides information if the current point is a boundary point
/// Provides functionality for WorkletPointNeighborhood algorithms
/// and Fetch's to determine if they are operating on a boundary point

//Todo we need to have this class handle different BoundaryTypes
struct BoundaryState
{
  VTKM_EXEC
  BoundaryState(const vtkm::Id3& ijk, const vtkm::Id3& pdims, int neighborhoodSize)
    : IJK(ijk)
    , PointDimensions(pdims)
    , NeighborhoodSize(neighborhoodSize)
  {
    for (vtkm::IdComponent dim = 0; dim < 3; ++dim)
    {
      if (neighborhoodSize < ijk[dim])
      {
        this->MinNeighborhood[dim] = -neighborhoodSize;
      }
      else
      {
        this->MinNeighborhood[dim] = static_cast<vtkm::IdComponent>(-ijk[dim]);
      }
      if (neighborhoodSize < pdims[dim] - ijk[dim] - 1)
      {
        this->MaxNeighborhood[dim] = neighborhoodSize;
      }
      else
      {
        this->MaxNeighborhood[dim] = static_cast<vtkm::IdComponent>(pdims[dim] - ijk[dim] - 1);
      }
    }
  }

  /// Returns the minimum neighbor index in the X direction (between -neighborhood size and 0)
  ///
  VTKM_EXEC vtkm::IdComponent MinNeighborX() const { return this->MinNeighborhood[0]; }

  /// Returns the minimum neighbor index in the Z direction (between -neighborhood size and 0)
  ///
  VTKM_EXEC vtkm::IdComponent MinNeighborY() const { return this->MinNeighborhood[1]; }

  /// Returns the minimum neighbor index in the Z direction (between -neighborhood size and 0)
  ///
  VTKM_EXEC vtkm::IdComponent MinNeighborZ() const { return this->MinNeighborhood[2]; }

  /// Returns the maximum neighbor index in the X direction (between 0 and neighborhood size)
  ///
  VTKM_EXEC vtkm::IdComponent MaxNeighborX() const { return this->MaxNeighborhood[0]; }

  /// Returns the maximum neighbor index in the Z direction (between 0 and neighborhood size)
  ///
  VTKM_EXEC vtkm::IdComponent MaxNeighborY() const { return this->MaxNeighborhood[1]; }

  /// Returns the maximum neighbor index in the Z direction (between 0 and neighborhood size)
  ///
  VTKM_EXEC vtkm::IdComponent MaxNeighborZ() const { return this->MaxNeighborhood[2]; }

  /// Returns true if the neighborhood extends past the positive X direction.
  ///
  VTKM_EXEC bool OnXPositive() const { return this->MaxNeighborX() < this->NeighborhoodSize; }

  /// Returns true if the neighborhood extends past the negative X direction.
  ///
  VTKM_EXEC bool OnXNegative() const { return -this->MinNeighborX() < this->NeighborhoodSize; }

  /// Returns true if the neighborhood extends past the positive Y direction.
  ///
  VTKM_EXEC bool OnYPositive() const { return this->MaxNeighborY() < this->NeighborhoodSize; }

  /// Returns true if the neighborhood extends past the negative Y direction.
  ///
  VTKM_EXEC bool OnYNegative() const { return -this->MinNeighborY() < this->NeighborhoodSize; }

  /// Returns true if the neighborhood extends past the positive Z direction.
  ///
  VTKM_EXEC bool OnZPositive() const { return this->MaxNeighborZ() < this->NeighborhoodSize; }

  /// Returns true if the neighborhood extends past the negative Z direction.
  ///
  VTKM_EXEC bool OnZNegative() const { return -this->MinNeighborZ() < this->NeighborhoodSize; }

  /// Returns true if the neighborhood extends past either X boundary.
  ///
  VTKM_EXEC bool OnX() const { return this->OnXNegative() || this->OnXPositive(); }

  /// Returns true if the neighborhood extends past either Y boundary.
  ///
  VTKM_EXEC bool OnY() const { return this->OnYNegative() || this->OnYPositive(); }

  /// Returns true if the neighborhood extends past either Z boundary.
  ///
  VTKM_EXEC bool OnZ() const { return this->OnZNegative() || this->OnZPositive(); }

  //todo: This needs to work with BoundaryConstantValue
  //todo: This needs to work with BoundaryPeroidic

  //@{
  /// Takes a local neighborhood index (in the ranges of -neighborhood size to neighborhood size)
  /// and returns the ijk of the equivalent point in the full data set. If the given value is out
  /// of range, the value is clamped to the nearest boundary. For example, if given a neighbor
  /// index that is past the minimum x range of the data, the index at the minimum x boundary is
  /// returned.
  ///
  VTKM_EXEC vtkm::Id3 NeighborIndexToFullIndexClamp(
    const vtkm::Vec<vtkm::IdComponent, 3>& neighbor) const
  {
    vtkm::Vec<vtkm::IdComponent, 3> clampedNeighbor =
      vtkm::Max(this->MinNeighborhood, vtkm::Min(this->MaxNeighborhood, neighbor));
    return this->IJK + clampedNeighbor;
  }

  VTKM_EXEC vtkm::Id3 NeighborIndexToFullIndexClamp(vtkm::IdComponent neighborI,
                                                    vtkm::IdComponent neighborJ,
                                                    vtkm::IdComponent neighborK) const
  {
    return this->NeighborIndexToFullIndexClamp(vtkm::make_Vec(neighborI, neighborJ, neighborK));
  }
  //@}

  //todo: This needs to work with BoundaryConstantValue
  //todo: This needs to work with BoundaryPeroidic

  //@{
  /// Takes a local neighborhood index (in the ranges of -neighborhood size to neighborhood size)
  /// and returns the flat index of the equivalent point in the full data set. If the given value
  /// is out of range, the value is clamped to the nearest boundary. For example, if given a
  /// neighbor index that is past the minimum x range of the data, the index at the minimum x
  /// boundary is returned.
  ///
  VTKM_EXEC vtkm::Id NeighborIndexToFlatIndexClamp(
    const vtkm::Vec<vtkm::IdComponent, 3>& neighbor) const
  {
    vtkm::Id3 full = this->NeighborIndexToFullIndexClamp(neighbor);

    return (full[2] * this->PointDimensions[1] + full[1]) * this->PointDimensions[0] + full[0];
  }

  VTKM_EXEC vtkm::Id NeighborIndexToFlatIndexClamp(vtkm::IdComponent neighborI,
                                                   vtkm::IdComponent neighborJ,
                                                   vtkm::IdComponent neighborK) const
  {
    return this->NeighborIndexToFlatIndexClamp(vtkm::make_Vec(neighborI, neighborJ, neighborK));
  }
  //@}

  vtkm::Id3 IJK;
  vtkm::Id3 PointDimensions;
  vtkm::Vec<vtkm::IdComponent, 3> MinNeighborhood;
  vtkm::Vec<vtkm::IdComponent, 3> MaxNeighborhood;
  vtkm::IdComponent NeighborhoodSize;
};

namespace detail
{
/// Given a \c Vec of (semi) arbitrary size, inflate it to a vtkm::Id3 by padding with zeros.
///
inline VTKM_EXEC vtkm::Id3 To3D(vtkm::Id3 index)
{
  return index;
}

/// Given a \c Vec of (semi) arbitrary size, inflate it to a vtkm::Id3 by padding with zeros.
/// \overload
inline VTKM_EXEC vtkm::Id3 To3D(vtkm::Id2 index)
{
  return vtkm::Id3(index[0], index[1], 1);
}

/// Given a \c Vec of (semi) arbitrary size, inflate it to a vtkm::Id3 by padding with zeros.
/// \overload
inline VTKM_EXEC vtkm::Id3 To3D(vtkm::Vec<vtkm::Id, 1> index)
{
  return vtkm::Id3(index[0], 1, 1);
}
}

/// \brief Container for thread information in a WorkletPointNeighborhood.
///
///
template <int NeighborhoodSize>
class ThreadIndicesPointNeighborhood
{

public:
  template <typename OutToInArrayType, typename VisitArrayType, vtkm::IdComponent Dimension>
  VTKM_EXEC ThreadIndicesPointNeighborhood(
    const vtkm::Id3& outIndex,
    const OutToInArrayType&,
    const VisitArrayType&,
    const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                             vtkm::TopologyElementTagPoint,
                                             Dimension>& connectivity,
    vtkm::Id globalThreadIndexOffset = 0)
    : State(outIndex, detail::To3D(connectivity.GetPointDimensions()), NeighborhoodSize)
    , InputIndex(0)
    , OutputIndex(0)
    , VisitIndex(0)
    , GlobalThreadIndexOffset(globalThreadIndexOffset)
  {
    using ConnectivityType = vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                                                vtkm::TopologyElementTagPoint,
                                                                Dimension>;
    using ConnRangeType = typename ConnectivityType::SchedulingRangeType;
    const ConnRangeType index = detail::Deflate(outIndex, ConnRangeType());
    this->InputIndex = connectivity.LogicalToFlatToIndex(index);
    this->OutputIndex = this->InputIndex;
  }

  template <typename OutToInArrayType, typename VisitArrayType, vtkm::IdComponent Dimension>
  VTKM_EXEC ThreadIndicesPointNeighborhood(
    const vtkm::Id& outIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                             vtkm::TopologyElementTagPoint,
                                             Dimension>& connectivity,
    vtkm::Id globalThreadIndexOffset = 0)
    : State(detail::To3D(connectivity.FlatToLogicalToIndex(outToIn.Get(outIndex))),
            detail::To3D(connectivity.GetPointDimensions()),
            NeighborhoodSize)
    , InputIndex(outToIn.Get(outIndex))
    , OutputIndex(outIndex)
    , VisitIndex(static_cast<vtkm::IdComponent>(visit.Get(outIndex)))
    , GlobalThreadIndexOffset(globalThreadIndexOffset)
  {
  }

  template <vtkm::IdComponent Dimension>
  VTKM_EXEC ThreadIndicesPointNeighborhood(
    const vtkm::Id& outIndex,
    const vtkm::Id& inIndex,
    const vtkm::IdComponent& visitIndex,
    const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                             vtkm::TopologyElementTagPoint,
                                             Dimension>& connectivity,
    vtkm::Id globalThreadIndexOffset = 0)
    : State(detail::To3D(connectivity.FlatToLogicalToIndex(inIndex)),
            detail::To3D(connectivity.GetPointDimensions()),
            NeighborhoodSize)
    , InputIndex(inIndex)
    , OutputIndex(outIndex)
    , VisitIndex(visitIndex)
    , GlobalThreadIndexOffset(globalThreadIndexOffset)
  {
  }

  VTKM_EXEC
  const BoundaryState& GetBoundaryState() const { return this->State; }

  VTKM_EXEC
  vtkm::Id GetInputIndex() const { return this->InputIndex; }

  VTKM_EXEC
  vtkm::Id3 GetInputIndex3D() const { return this->State.IJK; }

  VTKM_EXEC
  vtkm::Id GetOutputIndex() const { return this->OutputIndex; }

  VTKM_EXEC
  vtkm::IdComponent GetVisitIndex() const { return this->VisitIndex; }

  /// \brief The global index (for streaming).
  ///
  /// Global index (for streaming)
  VTKM_EXEC
  vtkm::Id GetGlobalIndex() const { return (this->GlobalThreadIndexOffset + this->OutputIndex); }

private:
  BoundaryState State;
  vtkm::Id InputIndex;
  vtkm::Id OutputIndex;
  vtkm::IdComponent VisitIndex;
  vtkm::Id GlobalThreadIndexOffset;
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_ThreadIndicesPointNeighborhood_h
