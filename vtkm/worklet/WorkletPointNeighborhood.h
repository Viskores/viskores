//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_WorkletPointNeighborhood_h
#define vtk_m_worklet_WorkletPointNeighborhood_h

/// \brief Worklet for volume algorithms that require a neighborhood
///
/// WorkletPointNeighborhood executes on every point inside a volume providing
/// access to the 3D neighborhood values. The neighborhood is always cubic in
/// nature and is fixed at compile time.
#include <vtkm/exec/arg/ThreadIndicesPointNeighborhood.h>
#include <vtkm/worklet/DispatcherPointNeighborhood.h>
#include <vtkm/worklet/WorkletNeighborhood.h>

namespace vtkm
{
namespace worklet
{

class WorkletPointNeighborhood : public WorkletNeighborhood
{
public:
  template <typename Worklet>
  using Dispatcher = vtkm::worklet::DispatcherPointNeighborhood<Worklet>;

  /// Point neighborhood worklets use the related thread indices class.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            vtkm::IdComponent Dimension>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesPointNeighborhood GetThreadIndices(
    vtkm::Id threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagPoint,
                                             vtkm::TopologyElementTagCell,
                                             Dimension>& inputDomain //this should be explicit
  ) const
  {
    const vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesPointNeighborhood(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, inputDomain);
  }


  /// In the remaining methods and `constexpr` we determine at compilation time
  /// which method definition will be actually used for GetThreadIndices.
  ///
  /// We want to avoid further function calls when we use WorkletMapTopology in which
  /// ScatterType is set as ScatterIdentity and MaskType as MaskNone.
  /// Otherwise, we call the default method defined at the bottom of this class.
private:
  static constexpr bool IsScatterIdentity =
    std::is_same<ScatterType, vtkm::worklet::ScatterIdentity>::value;
  static constexpr bool IsMaskNone = std::is_same<MaskType, vtkm::worklet::MaskNone>::value;

public:
  template <bool Cond, typename ReturnType>
  using EnableFnWhen = typename std::enable_if<Cond, ReturnType>::type;

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            bool S = IsScatterIdentity,
            bool M = IsMaskNone>
  VTKM_EXEC EnableFnWhen<S && M, vtkm::exec::arg::ThreadIndicesPointNeighborhood> GetThreadIndices(
    vtkm::Id threadIndex1D,
    const vtkm::Id3& threadIndex3D,
    const OutToInArrayType& vtkmNotUsed(outToIn),
    const VisitArrayType& vtkmNotUsed(visit),
    const ThreadToOutArrayType& vtkmNotUsed(threadToOut),
    const InputDomainType& connectivity) const
  {
    return vtkm::exec::arg::ThreadIndicesPointNeighborhood(
      threadIndex3D, threadIndex1D, connectivity);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            bool S = IsScatterIdentity,
            bool M = IsMaskNone>
  VTKM_EXEC EnableFnWhen<!(S && M), vtkm::exec::arg::ThreadIndicesPointNeighborhood>
  GetThreadIndices(vtkm::Id threadIndex1D,
                   const vtkm::Id3& threadIndex3D,
                   const OutToInArrayType& outToIn,
                   const VisitArrayType& visit,
                   const ThreadToOutArrayType& threadToOut,
                   const InputDomainType& connectivity) const
  {
    const vtkm::Id outIndex = threadToOut.Get(threadIndex1D);
    return vtkm::exec::arg::ThreadIndicesPointNeighborhood(threadIndex3D,
                                                           threadIndex1D,
                                                           outToIn.Get(outIndex),
                                                           visit.Get(outIndex),
                                                           outIndex,
                                                           connectivity);
  }
};
}
}

#endif
