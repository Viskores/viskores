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
#include <vtkm/cont/arg/TypeCheckTagArray.h>
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
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagTopologyFieldIn<VisitTopologyType>;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for input fields from the \em incident
  /// topology.
  ///
  struct FieldInIncident : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagTopologyFieldIn<IncidentTopologyType>;
    using FetchTag = vtkm::exec::arg::FetchTagArrayTopologyMapIn;
  };

  /// \brief A control signature tag for output fields.
  ///
  /// This tag takes a template argument that is a type list tag that limits
  /// the possible value types in the array.
  ///
  struct FieldOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagArrayOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectOut;
  };

  /// \brief A control signature tag for input-output (in-place) fields from
  /// the visited topology.
  ///
  struct FieldInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagArrayInOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectInOut;
  };

  /// \brief A control signature tag for input connectivity.
  ///
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
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType> GetThreadIndices(
    vtkm::Id threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const ThreadToOutArrayType& threadToOut,
    const InputDomainType& connectivity,
    vtkm::Id globalThreadIndexOffset) const
  {
    const vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType>(threadIndex,
                                                                      outToIn.Get(outIndex),
                                                                      visit.Get(outIndex),
                                                                      outIndex,
                                                                      connectivity,
                                                                      globalThreadIndexOffset);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType> GetThreadIndices(
    const vtkm::Id3& threadIndex,
    const OutToInArrayType& vtkmNotUsed(outToIn),
    const VisitArrayType& vtkmNotUsed(visit),
    const ThreadToOutArrayType& vtkmNotUsed(threadToOut),
    const InputDomainType& connectivity,
    vtkm::Id globalThreadIndexOffset = 0) const
  {
    using ScatterCheck = std::is_same<ScatterType, vtkm::worklet::ScatterIdentity>;
    VTKM_STATIC_ASSERT_MSG(ScatterCheck::value,
                           "Scheduling on 3D topologies only works with default ScatterIdentity.");
    using MaskCheck = std::is_same<MaskType, vtkm::worklet::MaskNone>;
    VTKM_STATIC_ASSERT_MSG(MaskCheck::value,
                           "Scheduling on 3D topologies only works with default MaskNone.");

    return vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType>(
      threadIndex, connectivity, globalThreadIndexOffset);
  }
};

/// Base class for worklets that map from Points to Cells.
///
class WorkletVisitCellsWithPoints
  : public WorkletMapTopology<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>
{
public:
  using FieldInPoint = FieldInIncident;

  using FieldInCell = FieldInVisit;

  using FieldOutCell = FieldOut;

  using FieldInOutCell = FieldInOut;

  using PointCount = IncidentElementCount;

  using PointIndices = IncidentElementIndices;
};

/// Base class for worklets that map from Cells to Points.
///
class WorkletVisitPointsWithCells
  : public WorkletMapTopology<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>
{
public:
  using FieldInCell = FieldInIncident;

  using FieldInPoint = FieldInVisit;

  using FieldOutPoint = FieldOut;

  using FieldInOutPoint = FieldInOut;

  using CellCount = IncidentElementCount;

  using CellIndices = IncidentElementIndices;
};

// Deprecated signatures for legacy support. These will be removed at some
// point.
using WorkletMapCellToPoint = WorkletVisitPointsWithCells;
using WorkletMapPointToCell = WorkletVisitCellsWithPoints;
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_WorkletMapTopology_h
