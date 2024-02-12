//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_WorkletCellNeighborhood_h
#define vtk_m_worklet_WorkletCellNeighborhood_h

/// \brief Worklet for volume algorithms that require a neighborhood
///
/// WorkletCellNeighborhood executes on every point inside a volume providing
/// access to the 3D neighborhood values. The neighborhood is always cubic in
/// nature and is fixed at compile time.

#include <vtkm/exec/arg/ThreadIndicesCellNeighborhood.h>
#include <vtkm/worklet/DispatcherCellNeighborhood.h>
#include <vtkm/worklet/WorkletNeighborhood.h>

namespace vtkm
{
namespace worklet
{

template <typename WorkletType>
class DispatcherCellNeighborhood;

/// @brief Base class for worklets that map over the cells in a structured grid with neighborhood information.
///
/// The domain of a `WorkletCellNeighborhood` is a `vtkm::cont::CellSetStructured`. It visits
/// all the cells in the mesh and provides access to the cell field values of the visited cell
/// and the field values of the nearby connected neighborhood of a prescribed size.
class WorkletCellNeighborhood : public WorkletNeighborhood
{
public:
  template <typename Worklet>
  using Dispatcher = vtkm::worklet::DispatcherCellNeighborhood<Worklet>;

  /// @defgroup WorkletCellNeighborhoodControlSigTags `ControlSignature` tags
  /// Tags that can be used in the `ControlSignature` of a `WorkletPointNeighborhood`.
  /// @{
#ifdef VTKM_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @copydoc vtkm::worklet::WorkletNeighborhood::CellSetIn
  struct CellSetIn : vtkm::worklet::WorkletNeighborhood::CellSetIn
  {
  };

  /// @copydoc vtkm::worklet::WorkletNeighborhood::FieldIn
  struct FieldIn : vtkm::worklet::WorkletNeighborhood::FieldIn
  {
  };

  /// @copydoc vtkm::worklet::WorkletNeighborhood::FieldInNeighborhood
  struct FieldInNeighborhood : vtkm::worklet::WorkletNeighborhood::FieldInNeighborhood
  {
  };

  /// @copydoc vtkm::worklet::WorkletNeighborhood::FieldOut
  struct FieldOut : vtkm::worklet::WorkletNeighborhood::FieldOut
  {
  };

  /// @copydoc vtkm::worklet::WorkletNeighborhood::FieldInOut
  struct FieldInOut : vtkm::worklet::WorkletNeighborhood::FieldInOut
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::WholeArrayIn
  struct WholeArrayIn : vtkm::worklet::internal::WorkletBase::WholeArrayIn
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::WholeArrayOut
  struct WholeArrayOut : vtkm::worklet::internal::WorkletBase::WholeArrayOut
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::WholeArrayInOut
  struct WholeArrayInOut : vtkm::worklet::internal::WorkletBase::WholeArrayInOut
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::AtomicArrayInOut
  struct AtomicArrayInOut : vtkm::worklet::internal::WorkletBase::AtomicArrayInOut
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::WholeCellSetIn
  template <typename VisitTopology = Cell, typename IncidentTopology = Point>
  struct WholeCellSetIn
    : vtkm::worklet::internal::WorkletBase::WholeCellSetIn<VisitTopology, IncidentTopology>
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::ExecObject
  struct ExecObject : vtkm::worklet::internal::WorkletBase::ExecObject
  {
  };
#endif // VTKM_DOXYGEN_ONLY
  /// @}

  /// @defgroup WorkletCellNeighborhoodExecutionSigTags `ExecutionSignature` tags
  /// Tags that can be used in the `ExecutionSignature` of a `WorkletPointNeighborhood`.
  /// @{
#ifdef VTKM_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @copydoc vtkm::placeholders::Arg
  struct _1 : vtkm::worklet::internal::WorkletBase::_1
  {
  };

  /// @copydoc vtkm::worklet::WorkletNeighborhood::Boundary
  struct Boundary : vtkm::worklet::WorkletNeighborhood::Boundary
  {
  };

  /// @copydoc vtkm::exec::arg::WorkIndex
  struct WorkIndex : vtkm::worklet::internal::WorkletBase::WorkIndex
  {
  };

  /// @copydoc vtkm::exec::arg::VisitIndex
  struct VisitIndex : vtkm::worklet::internal::WorkletBase::VisitIndex
  {
  };

  /// @copydoc vtkm::exec::arg::InputIndex
  struct InputIndex : vtkm::worklet::internal::WorkletBase::InputIndex
  {
  };

  /// @copydoc vtkm::exec::arg::OutputIndex
  struct OutputIndex : vtkm::worklet::internal::WorkletBase::OutputIndex
  {
  };

  /// @copydoc vtkm::exec::arg::ThreadIndices
  struct ThreadIndices : vtkm::worklet::internal::WorkletBase::ThreadIndices
  {
  };

  /// @copydoc vtkm::worklet::internal::WorkletBase::Device
  struct Device : vtkm::worklet::internal::WorkletBase::Device
  {
  };
#endif // VTKM_DOXYGEN_ONLY
  /// @}

  /// Point neighborhood worklets use the related thread indices class.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            vtkm::IdComponent Dimension>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesCellNeighborhood GetThreadIndices(
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
    return vtkm::exec::arg::ThreadIndicesCellNeighborhood(
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
  VTKM_EXEC EnableFnWhen<S && M, vtkm::exec::arg::ThreadIndicesCellNeighborhood> GetThreadIndices(
    vtkm::Id threadIndex1D,
    const vtkm::Id3& threadIndex3D,
    const OutToInArrayType& vtkmNotUsed(outToIn),
    const VisitArrayType& vtkmNotUsed(visit),
    const ThreadToOutArrayType& vtkmNotUsed(threadToOut),
    const InputDomainType& connectivity) const
  {
    return vtkm::exec::arg::ThreadIndicesCellNeighborhood(
      threadIndex3D, threadIndex1D, connectivity);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            bool S = IsScatterIdentity,
            bool M = IsMaskNone>
  VTKM_EXEC EnableFnWhen<!(S && M), vtkm::exec::arg::ThreadIndicesCellNeighborhood>
  GetThreadIndices(vtkm::Id threadIndex1D,
                   const vtkm::Id3& threadIndex3D,
                   const OutToInArrayType& outToIn,
                   const VisitArrayType& visit,
                   const ThreadToOutArrayType& threadToOut,
                   const InputDomainType& connectivity) const
  {
    const vtkm::Id outIndex = threadToOut.Get(threadIndex1D);
    return vtkm::exec::arg::ThreadIndicesCellNeighborhood(threadIndex3D,
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
