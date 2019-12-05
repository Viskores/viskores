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

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkm/TopologyElementTag.h>

#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/arg/TransportTagArrayIn.h>
#include <vtkm/cont/arg/TransportTagArrayInOut.h>
#include <vtkm/cont/arg/TransportTagArrayOut.h>
#include <vtkm/cont/arg/TransportTagCellSetIn.h>
#include <vtkm/cont/arg/TypeCheckTagArray.h>
#include <vtkm/cont/arg/TypeCheckTagCellSetStructured.h>

#include <vtkm/exec/arg/Boundary.h>
#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <vtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>
#include <vtkm/exec/arg/FetchTagArrayNeighborhoodIn.h>
#include <vtkm/exec/arg/FetchTagCellSetIn.h>
#include <vtkm/exec/arg/ThreadIndicesPointNeighborhood.h>

#include <vtkm/worklet/ScatterIdentity.h>


namespace vtkm
{
namespace worklet
{

template <typename WorkletType>
class DispatcherPointNeighborhood;


/// \brief Clamps boundary values to the nearest valid i,j,k value
///
/// BoundaryClamp always returns the nearest valid i,j,k value when at an
/// image boundary. This is a commonly used when solving differential equations.
///
/// For example, when used with WorkletPointNeighborhood3x3x3 when centered
/// on the point 1:
/// \code
///               * * *
///               * 1 2 (where * denotes points that lie outside of the image boundary)
///               * 3 5
/// \endcode
/// returns the following neighborhood of values:
/// \code
///              1 1 2
///              1 1 2
///              3 3 5
/// \endcode
struct BoundaryClamp
{
};

class WorkletPointNeighborhoodBase : public vtkm::worklet::internal::WorkletBase
{
public:
  template <typename Worklet>
  using Dispatcher = vtkm::worklet::DispatcherPointNeighborhood<Worklet>;

  /// \brief The \c ExecutionSignature tag to query if the current iteration is inside the boundary.
  ///
  /// A \c WorkletPointNeighborhood operates by iterating over all points using a defined
  /// neighborhood. This \c ExecutionSignature tag provides a \c BoundaryState object that allows
  /// you to query whether the neighborhood of the current iteration is completely inside the
  /// bounds of the mesh or if it extends beyond the mesh. This is important as when you are on a
  /// boundary the neighboordhood will contain empty values for a certain subset of values, and in
  /// this case the values returned will depend on the boundary behavior.
  ///
  struct Boundary : vtkm::exec::arg::Boundary
  {
  };

  /// All worklets must define their scatter operation.
  using ScatterType = vtkm::worklet::ScatterIdentity;

  /// All neighborhood worklets must define their boundary type operation.
  /// The boundary type determines how loading on boundaries will work.
  using BoundaryType = vtkm::worklet::BoundaryClamp;

  /// In addition to defining the boundary type, the worklet must produce the
  /// boundary condition. The default BoundaryClamp has no state, so just return an
  /// instance.
  /// Note: Currently only BoundaryClamp is implemented
  VTKM_CONT
  BoundaryType GetBoundaryCondition() const { return BoundaryType(); }

  /// \brief A control signature tag for input point fields.
  ///
  /// This tag takes a template argument that is a type list tag that limits
  /// the possible value types in the array.
  ///
  struct FieldIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagArrayIn;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for output point fields.
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

  /// \brief A control signature tag for input-output (in-place) point fields.
  ///
  /// This tag takes a template argument that is a type list tag that limits
  /// the possible value types in the array.
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
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagCellSetStructured;
    using TransportTag = vtkm::cont::arg::TransportTagCellSetIn<vtkm::TopologyElementTagPoint,
                                                                vtkm::TopologyElementTagCell>;
    using FetchTag = vtkm::exec::arg::FetchTagCellSetIn;
  };
};

class WorkletPointNeighborhood : public WorkletPointNeighborhoodBase
{
public:
  /// \brief A control signature tag for neighborhood input values.
  ///
  /// A \c WorkletPointNeighborhood operates allowing access to a adjacent point
  /// values in a NxNxN patch called a neighborhood.
  /// No matter the size of the neighborhood it is symmetric across its center
  /// in each axis, and the current point value will be at the center
  /// For example a 3x3x3 neighborhood would
  ///
  /// This tag specifies an \c ArrayHandle object that holds the values. It is
  /// an input array with entries for each point.
  ///
  struct FieldInNeighborhood : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagArrayIn;
    using FetchTag = vtkm::exec::arg::FetchTagArrayNeighborhoodIn;
  };

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
                                             Dimension>& inputDomain, //this should be explicitly
    vtkm::Id globalThreadIndexOffset = 0) const
  {
    const vtkm::Id outIndex = threadToOut.Get(threadIndex);
    return vtkm::exec::arg::ThreadIndicesPointNeighborhood(threadIndex,
                                                           outToIn.Get(outIndex),
                                                           visit.Get(outIndex),
                                                           outIndex,
                                                           inputDomain,
                                                           globalThreadIndexOffset);
  }

  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename OutToInArrayType,
            typename VisitArrayType,
            typename ThreadToOutArrayType,
            typename InputDomainType>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesPointNeighborhood GetThreadIndices(
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

    return vtkm::exec::arg::ThreadIndicesPointNeighborhood(
      threadIndex, connectivity, globalThreadIndexOffset);
  }
};
}
}

#endif
