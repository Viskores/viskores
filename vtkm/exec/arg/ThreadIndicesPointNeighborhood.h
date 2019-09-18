//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_ThreadIndicesPointNeighborhood_h
#define vtk_m_exec_arg_ThreadIndicesPointNeighborhood_h

#include <vtkm/exec/BoundaryState.h>
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
  template <vtkm::IdComponent Dimension>
  VTKM_EXEC ThreadIndicesPointNeighborhood(
    const vtkm::Id3& outIndex,
    const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                             vtkm::TopologyElementTagCell,
                                             Dimension>& connectivity,
    vtkm::Id globalThreadIndexOffset = 0)
    : State(outIndex, detail::To3D(connectivity.GetPointDimensions()))
    , GlobalThreadIndexOffset(globalThreadIndexOffset)
  {
    using ConnectivityType = vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                                                vtkm::TopologyElementTagCell,
                                                                Dimension>;
    using ConnRangeType = typename ConnectivityType::SchedulingRangeType;
    const ConnRangeType index = detail::Deflate(outIndex, ConnRangeType());
    this->ThreadIndex = connectivity.LogicalToFlatToIndex(index);
    this->InputIndex = this->ThreadIndex;
    this->VisitIndex = 0;
    this->OutputIndex = this->ThreadIndex;
  }

  template <vtkm::IdComponent Dimension>
  VTKM_EXEC ThreadIndicesPointNeighborhood(
    vtkm::Id threadIndex,
    vtkm::Id inputIndex,
    vtkm::IdComponent visitIndex,
    vtkm::Id outputIndex,
    const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                             vtkm::TopologyElementTagCell,
                                             Dimension>& connectivity,
    vtkm::Id globalThreadIndexOffset = 0)
    : State(detail::To3D(connectivity.FlatToLogicalToIndex(inputIndex)),
            detail::To3D(connectivity.GetPointDimensions()))
    , ThreadIndex(threadIndex)
    , InputIndex(inputIndex)
    , OutputIndex(outputIndex)
    , VisitIndex(visitIndex)
    , GlobalThreadIndexOffset(globalThreadIndexOffset)
  {
  }

  VTKM_EXEC
  const vtkm::exec::BoundaryState& GetBoundaryState() const { return this->State; }

  VTKM_EXEC
  vtkm::Id GetThreadIndex() const { return this->ThreadIndex; }

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
  vtkm::exec::BoundaryState State;
  vtkm::Id ThreadIndex;
  vtkm::Id InputIndex;
  vtkm::Id OutputIndex;
  vtkm::IdComponent VisitIndex;
  vtkm::Id GlobalThreadIndexOffset;
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_ThreadIndicesPointNeighborhood_h
