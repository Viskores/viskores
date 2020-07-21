//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_WorkletNeighborhood_h
#define vtk_m_worklet_WorkletNeighborhood_h

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkm/TopologyElementTag.h>

#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/arg/TransportTagArrayIn.h>
#include <vtkm/cont/arg/TransportTagArrayInOut.h>
#include <vtkm/cont/arg/TransportTagArrayOut.h>
#include <vtkm/cont/arg/TransportTagCellSetIn.h>
#include <vtkm/cont/arg/TypeCheckTagArrayIn.h>
#include <vtkm/cont/arg/TypeCheckTagArrayInOut.h>
#include <vtkm/cont/arg/TypeCheckTagArrayOut.h>
#include <vtkm/cont/arg/TypeCheckTagCellSetStructured.h>

#include <vtkm/exec/arg/Boundary.h>
#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <vtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>
#include <vtkm/exec/arg/FetchTagArrayNeighborhoodIn.h>
#include <vtkm/exec/arg/FetchTagCellSetIn.h>

#include <vtkm/worklet/BoundaryTypes.h>
#include <vtkm/worklet/ScatterIdentity.h>

namespace vtkm
{
namespace worklet
{

class WorkletNeighborhood : public vtkm::worklet::internal::WorkletBase
{
public:
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
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayIn;
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
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayOut;
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
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayInOut;
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
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = vtkm::cont::arg::TransportTagArrayIn;
    using FetchTag = vtkm::exec::arg::FetchTagArrayNeighborhoodIn;
  };
};
} // namespace worklet
} // namespace vtkm

#endif // vtk_m_worklet_WorkletPointNeighborhood_h
