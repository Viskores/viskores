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
  enum OnWhichBoundaries
  {
    NONE = 1,
    X_MIN = 1 << 1,
    X_MAX = 1 << 2,
    Y_MIN = 1 << 3,
    Y_MAX = 1 << 4,
    Z_MIN = 1 << 5,
    Z_MAX = 1 << 6
  };

  VTKM_EXEC
  BoundaryState(const vtkm::Id3& ijk, const vtkm::Id3& pdims, int neighborhoodSize)
    : IJK(ijk)
    , PointDimensions(pdims)
    , Boundaries(OnWhichBoundaries::NONE)
  {
    //Maybe we should use function binding here, we could bind to the correct
    //clamp function based on our boundary condition and if lay on the boundary
    if (ijk[0] - neighborhoodSize < 0)
    {
      this->Boundaries |= OnWhichBoundaries::X_MIN;
    }

    if (ijk[0] + neighborhoodSize >= PointDimensions[0])
    {
      this->Boundaries |= OnWhichBoundaries::X_MAX;
    }

    if (ijk[1] - neighborhoodSize < 0)
    {
      this->Boundaries |= OnWhichBoundaries::Y_MIN;
    }

    if (ijk[1] + neighborhoodSize >= PointDimensions[1])
    {
      this->Boundaries |= OnWhichBoundaries::Y_MAX;
    }

    if (ijk[2] - neighborhoodSize < 0)
    {
      this->Boundaries |= OnWhichBoundaries::Z_MIN;
    }

    if (ijk[2] + neighborhoodSize >= PointDimensions[2])
    {
      this->Boundaries |= OnWhichBoundaries::Z_MAX;
    }
  }

  //Note: Due to C4800 This methods are in the form of ( ) != 0, instead of
  //just returning the value
  ///Returns true if we could access boundary elements in the X positive direction
  VTKM_EXEC
  inline bool OnXPositive() const { return (this->Boundaries & OnWhichBoundaries::X_MAX) != 0; }

  ///Returns true if we could access boundary elements in the X negative direction
  VTKM_EXEC
  inline bool OnXNegative() const { return (this->Boundaries & OnWhichBoundaries::X_MIN) != 0; }

  ///Returns true if we could access boundary elements in the Y positive direction
  VTKM_EXEC
  inline bool OnYPositive() const { return (this->Boundaries & OnWhichBoundaries::Y_MAX) != 0; }

  ///Returns true if we could access boundary elements in the Y negative direction
  VTKM_EXEC
  inline bool OnYNegative() const { return (this->Boundaries & OnWhichBoundaries::Y_MIN) != 0; }

  ///Returns true if we could access boundary elements in the Z positive direction
  VTKM_EXEC
  inline bool OnZPositive() const { return (this->Boundaries & OnWhichBoundaries::Z_MAX) != 0; }

  ///Returns true if we could access boundary elements in the Z negative direction
  VTKM_EXEC
  inline bool OnZNegative() const { return (this->Boundaries & OnWhichBoundaries::Z_MIN) != 0; }


  ///Returns true if we could access boundary elements in the X direction
  VTKM_EXEC
  inline bool OnX() const { return this->OnXPositive() || this->OnXNegative(); }

  ///Returns true if we could access boundary elements in the Y direction
  VTKM_EXEC
  inline bool OnY() const { return this->OnYPositive() || this->OnYNegative(); }

  ///Returns true if we could access boundary elements in the Z direction
  VTKM_EXEC
  inline bool OnZ() const { return this->OnZPositive() || this->OnZNegative(); }

  //todo: This needs to work with BoundaryConstantValue
  //todo: This needs to work with BoundaryPeroidic
  VTKM_EXEC
  void Clamp(vtkm::Id& i, vtkm::Id& j, vtkm::Id& k) const
  {
    //BoundaryClamp implementation
    //Clamp each item to a valid range, the index coming in is offsets from the
    //center IJK index
    i += this->IJK[0];
    j += this->IJK[1];
    k += this->IJK[2];

    if (this->Boundaries != OnWhichBoundaries::NONE)
    {
      i = (i < 0) ? 0 : i;
      i = (i < this->PointDimensions[0]) ? i : (this->PointDimensions[0] - 1);

      j = (j < 0) ? 0 : j;
      j = (j < this->PointDimensions[1]) ? j : (this->PointDimensions[1] - 1);

      k = (k < 0) ? 0 : k;
      k = (k < this->PointDimensions[2]) ? k : (this->PointDimensions[2] - 1);
    }
  }

  VTKM_EXEC
  void Clamp(vtkm::Id3& index) const { this->Clamp(index[0], index[1], index[2]); }


  //todo: This needs to work with BoundaryConstantValue
  //todo: This needs to work with BoundaryPeroidic
  VTKM_EXEC
  vtkm::Id ClampAndFlatten(vtkm::Id i, vtkm::Id j, vtkm::Id k) const
  {
    //BoundaryClamp implementation
    //Clamp each item to a valid range, the index coming in is offsets from the
    //center IJK index
    i += this->IJK[0];
    j += this->IJK[1];
    k += this->IJK[2];

    if (this->Boundaries != OnWhichBoundaries::NONE)
    {
      i = (i < 0) ? 0 : i;
      i = (i < this->PointDimensions[0]) ? i : (this->PointDimensions[0] - 1);

      j = (j < 0) ? 0 : j;
      j = (j < this->PointDimensions[1]) ? j : (this->PointDimensions[1] - 1);

      k = (k < 0) ? 0 : k;
      k = (k < this->PointDimensions[2]) ? k : (this->PointDimensions[2] - 1);
    }

    return (k * this->PointDimensions[1] + j) * this->PointDimensions[0] + i;
  }

  VTKM_EXEC
  vtkm::Id ClampAndFlatten(const vtkm::Id3& index) const
  {
    return this->ClampAndFlatten(index[0], index[1], index[2]);
  }

  vtkm::Id3 IJK;
  vtkm::Id3 PointDimensions;
  vtkm::Int32 Boundaries;
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
