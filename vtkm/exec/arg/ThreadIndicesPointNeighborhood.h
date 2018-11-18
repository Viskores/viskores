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
  BoundaryState(const vtkm::Id3& ijk, const vtkm::Id3& pdims)
    : IJK(ijk)
    , PointDimensions(pdims)
  {
  }

  //@{
  /// Returns true if a neighborhood of the given number of layers is contained within the bounds
  /// of the cell set in the X, Y, or Z direction. Returns false if the neighborhood extends ouside
  /// of the boundary of the data in the X, Y, or Z direction.
  ///
  /// The number of layers defines the size of the neighborhood in terms of how far away it extends
  /// from the center. So if there is 1 layer, the neighborhood extends 1 unit away from the center
  /// in each direction and is 3x3x3. If there are 2 layers, the neighborhood extends 2 units for a
  /// size of 5x5x5.
  ///
  VTKM_EXEC bool InXBoundary(vtkm::IdComponent numLayers) const
  {
    return (((this->IJK[0] - numLayers) >= 0) &&
            ((this->IJK[0] + numLayers) < this->PointDimensions[0]));
  }
  VTKM_EXEC bool InYBoundary(vtkm::IdComponent numLayers) const
  {
    return (((this->IJK[1] - numLayers) >= 0) &&
            ((this->IJK[1] + numLayers) < this->PointDimensions[1]));
  }
  VTKM_EXEC bool InZBoundary(vtkm::IdComponent numLayers) const
  {
    return (((this->IJK[2] - numLayers) >= 0) &&
            ((this->IJK[2] + numLayers) < this->PointDimensions[2]));
  }
  //@}

  /// Returns true if a neighborhood of the given number of layers is contained within the bounds
  /// of the cell set. Returns false if the neighborhood extends ouside of the boundary of the
  /// data.
  ///
  /// The number of layers defines the size of the neighborhood in terms of how far away it extends
  /// from the center. So if there is 1 layer, the neighborhood extends 1 unit away from the center
  /// in each direction and is 3x3x3. If there are 2 layers, the neighborhood extends 2 units for a
  /// size of 5x5x5.
  ///
  VTKM_EXEC bool InBoundary(vtkm::IdComponent numLayers) const
  {
    return this->InXBoundary(numLayers) && this->InYBoundary(numLayers) &&
      this->InZBoundary(numLayers);
  }

  /// Returns the minimum neighborhood indices that are within the bounds of the data.
  ///
  VTKM_EXEC vtkm::Vec<vtkm::IdComponent, 3> MinNeighborIndices(vtkm::IdComponent numLayers) const
  {
    vtkm::Vec<vtkm::IdComponent, 3> minIndices;

    for (vtkm::IdComponent component = 0; component < 3; ++component)
    {
      if (this->IJK[component] >= numLayers)
      {
        minIndices[component] = -numLayers;
      }
      else
      {
        minIndices[component] = static_cast<vtkm::IdComponent>(-this->IJK[component]);
      }
    }

    return minIndices;
  }

  /// Returns the minimum neighborhood indices that are within the bounds of the data.
  ///
  VTKM_EXEC vtkm::Vec<vtkm::IdComponent, 3> MaxNeighborIndices(vtkm::IdComponent numLayers) const
  {
    vtkm::Vec<vtkm::IdComponent, 3> maxIndices;

    for (vtkm::IdComponent component = 0; component < 3; ++component)
    {
      if ((this->PointDimensions[component] - this->IJK[component] - 1) >= numLayers)
      {
        maxIndices[component] = numLayers;
      }
      else
      {
        maxIndices[component] = static_cast<vtkm::IdComponent>(this->PointDimensions[component] -
                                                               this->IJK[component] - 1);
      }
    }

    return maxIndices;
  }

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
    vtkm::Id3 fullIndex = this->IJK + neighbor;

    return vtkm::Max(vtkm::Id3(0), vtkm::Min(this->PointDimensions - vtkm::Id3(1), fullIndex));
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
    : State(outIndex, detail::To3D(connectivity.GetPointDimensions()))
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
            detail::To3D(connectivity.GetPointDimensions()))
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
            detail::To3D(connectivity.GetPointDimensions()))
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
