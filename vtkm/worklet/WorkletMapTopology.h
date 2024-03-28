//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_WorkletMapTopology_h
#define vtk_m_worklet_WorkletMapTopology_h

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkm/TopologyElementTag.h>

#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/arg/TransportTagArrayInOut.h>
#include <vtkm/cont/arg/TransportTagArrayOut.h>
#include <vtkm/cont/arg/TransportTagCellSetIn.h>
#include <vtkm/cont/arg/TransportTagTopologyFieldIn.h>
#include <vtkm/cont/arg/TypeCheckTagArrayIn.h>
#include <vtkm/cont/arg/TypeCheckTagArrayInOut.h>
#include <vtkm/cont/arg/TypeCheckTagArrayOut.h>
#include <vtkm/cont/arg/TypeCheckTagCellSet.h>

#include <vtkm/exec/arg/CellShape.h>
#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <vtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>
#include <vtkm/exec/arg/FetchTagArrayTopologyMapIn.h>
#include <vtkm/exec/arg/FetchTagCellSetIn.h>
#include <vtkm/exec/arg/IncidentElementCount.h>
#include <vtkm/exec/arg/IncidentElementIndices.h>
#include <vtkm/exec/arg/ThreadIndicesTopologyMap.h>

#include <vtkm/worklet/DispatcherMapTopology.h>

namespace vtkm
{
namespace worklet
{

template <typename WorkletType>
class DispatcherMapTopology;

namespace detail
{

struct WorkletMapTopologyBase : vtkm::worklet::internal::WorkletBase
{
  template <typename Worklet>
  using Dispatcher = vtkm::worklet::DispatcherMapTopology<Worklet>;
};

} // namespace detail

/// @brief Base class for worklets that map topology elements onto each other.
///
/// The template parameters for this class must be members of the
/// TopologyElementTag group. The VisitTopology indicates the elements of a
/// cellset that will be visited, and the IncidentTopology will be mapped onto
/// the VisitTopology.
///
/// For instance,
/// `WorkletMapTopology<TopologyElementTagPoint, TopologyElementCell>` will
/// execute one instance per point, and provides convenience methods for
/// gathering information about the cells incident to the current point.
///
template <typename VisitTopology, typename IncidentTopology>
class WorkletMapTopology : public detail::WorkletMapTopologyBase
{
public:
  using VisitTopologyType = VisitTopology;
  using IncidentTopologyType = IncidentTopology;

  /// \brief A control signature tag for input fields from the \em visited
  /// topology.
  ///
  struct FieldInVisit : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = vtkm::cont::arg::TransportTagTopologyFieldIn<VisitTopologyType>;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for input fields from the \em incident
  /// topology.
  ///
  struct FieldInIncident : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = vtkm::cont::arg::TransportTagTopologyFieldIn<IncidentTopologyType>;
    using FetchTag = vtkm::exec::arg::FetchTagArrayTopologyMapIn;
  };

  /// \brief A control signature tag for output fields.
  ///
  struct FieldOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayOut;
    using TransportTag = vtkm::cont::arg::TransportTagArrayOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectOut;
  };

  /// \brief A control signature tag for input-output (in-place) fields from
  /// the visited topology.
  ///
  struct FieldInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayInOut;
    using TransportTag = vtkm::cont::arg::TransportTagArrayInOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectInOut;
  };

  /// @brief A control signature tag for input connectivity.
  ///
  /// The associated parameter of the invoke should be a subclass of `vtkm::cont::CellSet`.
  ///
  /// There should be exactly one `CellSetIn` argument in the `ControlSignature`,
  /// and the `InputDomain` must point to it.
  struct CellSetIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagCellSet;
    using TransportTag =
      vtkm::cont::arg::TransportTagCellSetIn<VisitTopologyType, IncidentTopologyType>;
    using FetchTag = vtkm::exec::arg::FetchTagCellSetIn;
  };

  /// \brief An execution signature tag for getting the cell shape. This only
  /// makes sense when visiting cell topologies.
  ///
  struct CellShape : vtkm::exec::arg::CellShape
  {
  };

  /// \brief An execution signature tag to get the number of \em incident
  /// elements.
  ///
  /// In a topology map, there are \em visited and \em incident topology
  /// elements specified. The scheduling occurs on the \em visited elements,
  /// and for each \em visited element there is some number of incident \em
  /// mapped elements that are accessible. This \c ExecutionSignature tag
  /// provides the number of these \em mapped elements that are accessible.
  ///
  struct IncidentElementCount : vtkm::exec::arg::IncidentElementCount
  {
  };

  /// \brief An execution signature tag to get the indices of from elements.
  ///
  /// In a topology map, there are \em visited and \em incident topology
  /// elements specified. The scheduling occurs on the \em visited elements,
  /// and for each \em visited element there is some number of incident \em
  /// mapped elements that are accessible. This \c ExecutionSignature tag
  /// provides the indices of the \em mapped elements that are incident to the
  /// current \em visited element.
  ///
  struct IncidentElementIndices : vtkm::exec::arg::IncidentElementIndices
  {
  };

  /// Topology map worklets use topology map indices.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType,
                                                      vtkm::exec::arg::CustomScatterOrMaskTag>
  GetThreadIndices(vtkm::Id threadIndex,
                   const OutToInArrayType& outToIn,
                   const VisitArrayType& visit,
                   const ThreadToOutArrayType& threadToOut,
                   const InputDomainType& connectivity) const
  {
    const vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType,
                                                     vtkm::exec::arg::CustomScatterOrMaskTag>(
      threadIndex, outToIn.Get(outIndex), visit.Get(outIndex), outIndex, connectivity);
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

  template <bool Cond, typename ReturnType>
  using EnableFnWhen = typename std::enable_if<Cond, ReturnType>::type;

public:
  /// Optimized for ScatterIdentity and MaskNone
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            bool S = IsScatterIdentity,
            bool M = IsMaskNone>
  VTKM_EXEC EnableFnWhen<
    S && M,
    vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType,
                                              vtkm::exec::arg::DefaultScatterAndMaskTag>>
  GetThreadIndices(vtkm::Id threadIndex1D,
                   const vtkm::Id3& threadIndex3D,
                   const OutToInArrayType& vtkmNotUsed(outToIn),
                   const VisitArrayType& vtkmNotUsed(visit),
                   const ThreadToOutArrayType& vtkmNotUsed(threadToOut),
                   const InputDomainType& connectivity) const
  {
    return vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType,
                                                     vtkm::exec::arg::DefaultScatterAndMaskTag>(
      threadIndex3D, threadIndex1D, connectivity);
  }

  /// Default version
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType,
            bool S = IsScatterIdentity,
            bool M = IsMaskNone>
  VTKM_EXEC
    EnableFnWhen<!(S && M),
                 vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType,
                                                           vtkm::exec::arg::CustomScatterOrMaskTag>>
    GetThreadIndices(vtkm::Id threadIndex1D,
                     const vtkm::Id3& threadIndex3D,
                     const OutToInArrayType& outToIn,
                     const VisitArrayType& visit,
                     const ThreadToOutArrayType& threadToOut,
                     const InputDomainType& connectivity) const
  {
    const vtkm::Id outIndex = threadToOut.Get(threadIndex1D);
    return vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType,
                                                     vtkm::exec::arg::CustomScatterOrMaskTag>(
      threadIndex3D,
      threadIndex1D,
      outToIn.Get(outIndex),
      visit.Get(outIndex),
      outIndex,
      connectivity);
  }
};

/// Base class for worklets that map from Points to Cells.
///
class WorkletVisitCellsWithPoints
  : public WorkletMapTopology<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>
{
public:
#ifndef VTKM_DOXYGEN_ONLY
  using FieldInPoint = FieldInIncident;

  using FieldInCell = FieldInVisit;

  using FieldOutCell = FieldOut;

  using FieldInOutCell = FieldInOut;

  using PointCount = IncidentElementCount;

  using PointIndices = IncidentElementIndices;
#else  // VTKM_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @defgroup WorkletVisitCellsWithPointsControlSigTags `ControlSignature` tags
  /// Tags that can be used in the `ControlSignature` of a `WorkletVisitCellsWithPoints`.
  /// @{

  /// @copydoc vtkm::worklet::WorkletMapTopology::CellSetIn
  struct CellSetIn
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::CellSetIn
  {
  };

  /// @brief A control signature tag for input fields on the cells of the topology.
  ///
  /// The associated parameter of the invoke should be a `vtkm::cont::ArrayHandle` that has
  /// the same number of values as the cells of the provided `CellSet`.
  /// The worklet gets a single value that is the field at that cell.
  struct FieldInCell : FieldInVisit
  {
  };

  /// @brief A control signature tag for input fields on the points of the topology.
  ///
  /// The associated parameter of the invoke should be a `vtkm::cont::ArrayHandle` that has
  /// the same number of values as the points of the provided `CellSet`.
  /// The worklet gets a Vec-like object containing the field values on all incident points.
  struct FieldInPoint : FieldInIncident
  {
  };

  /// @brief A control signature tag for input fields from the visited topology.
  ///
  /// For `WorkletVisitCellsWithPoints`, this is the same as `FieldInCell`.
  struct FieldInVisit
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldInVisit
  {
  };

  /// @brief A control signature tag for input fields from the incident topology.
  ///
  /// For `WorkletVisitCellsWithPoints`, this is the same as `FieldInPoint`.
  struct FieldInIncident
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldInIncident
  {
  };

  /// @brief A control signature tag for output fields.
  ///
  /// A `WorkletVisitCellsWithPoints` always has the output on the cells of the topology.
  /// The associated parameter of the invoke should be a `vtkm::cont::ArrayHandle`, and it will
  /// be resized to the number of cells in the provided `CellSet`.
  struct FieldOutCell : FieldOut
  {
  };

  /// @copydoc FieldOutCell
  struct FieldOut
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldOut
  {
  };

  /// @brief A control signature tag for input-output (in-place) fields.
  ///
  /// A `WorkletVisitCellsWithPoints` always has the output on the cells of the topology.
  /// The associated parameter of the invoke should be a `vtkm::cont::ArrayHandle`, and it must
  /// have the same number of values as the number of cells of the topology.
  struct FieldInOutCell : FieldInOut
  {
  };

  /// @copydoc FieldInOutCell
  struct FieldInOut
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldInOut
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

  /// @}

  /// @defgroup WorkletVisitCellsWithPointsExecutionSigTags `ExecutionSignature` tags
  /// Tags that can be used in the `ExecutionSignature` of a `WorkletVisitCellsWithPoints`.
  /// @{

  /// @copydoc vtkm::placeholders::Arg
  struct _1 : vtkm::worklet::internal::WorkletBase::_1
  {
  };

  /// @brief An execution signature tag to get the shape of the visited cell.
  ///
  /// This tag causes a `vtkm::UInt8` to be passed to the worklet containing containing an
  /// id for the shape of the cell being visited.
  struct CellShape
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::CellShape
  {
  };

  /// @brief An execution signature tag to get the number of incident points.
  ///
  /// Each cell in a `vtkm::cont::CellSet` can be incident on a number of points. This
  /// tag causes a `vtkm::IdComponent` to be passed to the worklet containing the number
  /// of incident points.
  struct PointCount
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType,
                                        IncidentTopologyType>::IncidentElementCount
  {
  };

  /// @brief An execution signature tag to get the indices of the incident points.
  ///
  /// The indices will be provided in a Vec-like object containing `vtkm::Id` indices for the
  /// cells in the data set.
  struct PointIndices
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType,
                                        IncidentTopologyType>::IncidentElementIndices
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

/// @}
#endif // VTKM_DOXYGEN_ONLY
};

/// Base class for worklets that map from Cells to Points.
///
class WorkletVisitPointsWithCells
  : public WorkletMapTopology<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>
{
public:
#ifndef VTKM_DOXYGEN_ONLY
  using FieldInCell = FieldInIncident;

  using FieldInPoint = FieldInVisit;

  using FieldOutPoint = FieldOut;

  using FieldInOutPoint = FieldInOut;

  using CellCount = IncidentElementCount;

  using CellIndices = IncidentElementIndices;
#else  // VTKM_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @defgroup WorkletVisitPointsWithCellsControlSigTags `ControlSignature` tags
  /// Tags that can be used in the `ControlSignature` of a `WorkletVisitPointsWithCells`.
  /// @{

  /// @copydoc vtkm::worklet::WorkletMapTopology::CellSetIn
  struct CellSetIn
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::CellSetIn
  {
  };

  /// @brief A control signature tag for input fields on the points of the topology.
  ///
  /// The associated parameter of the invoke should be a `vtkm::cont::ArrayHandle` that has
  /// the same number of values as the points of the provided `CellSet`.
  /// The worklet gets a single value that is the field at that point.
  struct FieldInPoint : FieldInVisit
  {
  };

  /// @brief A control signature tag for input fields on the cells of the topology.
  ///
  /// The associated parameter of the invoke should be a `vtkm::cont::ArrayHandle` that has
  /// the same number of values as the cells of the provided `CellSet`.
  /// The worklet gets a Vec-like object containing the field values on all incident cells.
  struct FieldInCell : FieldInIncident
  {
  };

  /// @brief A control signature tag for input fields from the visited topology.
  ///
  /// For `WorkletVisitPointsWithCells`, this is the same as `FieldInPoint`.
  struct FieldInVisit
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldInVisit
  {
  };

  /// @brief A control signature tag for input fields from the incident topology.
  ///
  /// For `WorkletVisitPointsWithCells`, this is the same as `FieldInCell`.
  struct FieldInIncident
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldInIncident
  {
  };

  /// @brief A control signature tag for output fields.
  ///
  /// A `WorkletVisitPointsWithCells` always has the output on the points of the topology.
  /// The associated parameter of the invoke should be a `vtkm::cont::ArrayHandle`, and it will
  /// be resized to the number of points in the provided `CellSet`.
  struct FieldOutPoint : FieldOut
  {
  };

  /// @copydoc FieldOutPoint
  struct FieldOut
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldOut
  {
  };

  /// @brief A control signature tag for input-output (in-place) fields.
  ///
  /// A `WorkletVisitPointsWithCells` always has the output on the points of the topology.
  /// The associated parameter of the invoke should be a `vtkm::cont::ArrayHandle`, and it must
  /// have the same number of values as the number of points of the topology.
  struct FieldInOutPoint : FieldInOut
  {
  };

  /// @copydoc FieldInOutPoint
  struct FieldInOut
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType, IncidentTopologyType>::FieldInOut
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

  /// @}

  /// @defgroup WorkletVisitPointsWithCellsExecutionSigTags `ExecutionSignature` tags
  /// Tags that can be used in the `ExecutionSignature` of a `WorkletVisitPointsWithCells`.
  /// @{

  /// @copydoc vtkm::placeholders::Arg
  struct _1 : vtkm::worklet::internal::WorkletBase::_1
  {
  };

  /// @brief An execution signature tag to get the number of incident cells.
  ///
  /// Each point in a `vtkm::cont::CellSet` can be incident on a number of cells. This
  /// tag causes a `vtkm::IdComponent` to be passed to the worklet containing the number
  /// of incident cells.
  struct CellCount
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType,
                                        IncidentTopologyType>::IncidentElementCount
  {
  };

  /// @brief An execution signature tag to get the indices of the incident cells.
  ///
  /// The indices will be provided in a Vec-like object containing `vtkm::Id` indices for the
  /// points in the data set.
  struct CellIndices
    : vtkm::worklet::WorkletMapTopology<VisitTopologyType,
                                        IncidentTopologyType>::IncidentElementIndices
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

  /// @}
#endif // VTKM_DOXYGEN_ONLY
};

}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_WorkletMapTopology_h
