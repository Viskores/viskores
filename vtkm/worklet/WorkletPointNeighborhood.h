//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_WorkletPointNeigborhood_h
#define vtk_m_worklet_WorkletPointNeigborhood_h

/// \brief Worklet for volume algorithms that require a neighborhood
///
/// WorkletPointNeighborhood executes on every point inside a volume providing
/// access to the 3D neighborhood values. The neighborhood is always cubic in
/// nature and is fixed at compile time.

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkm/TopologyElementTag.h>
#include <vtkm/TypeListTag.h>

#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/arg/TransportTagArrayIn.h>
#include <vtkm/cont/arg/TransportTagArrayInOut.h>
#include <vtkm/cont/arg/TransportTagArrayOut.h>
#include <vtkm/cont/arg/TransportTagCellSetIn.h>
#include <vtkm/cont/arg/TypeCheckTagArray.h>
#include <vtkm/cont/arg/TypeCheckTagCellSetStructured.h>

#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <vtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>
#include <vtkm/exec/arg/FetchTagArrayNeighborhoodIn.h>
#include <vtkm/exec/arg/FetchTagCellSetIn.h>
#include <vtkm/exec/arg/OnBoundary.h>
#include <vtkm/exec/arg/ThreadIndicesPointNeighborhood.h>

#include <vtkm/worklet/ScatterIdentity.h>


namespace vtkm
{
namespace worklet
{

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
  /// \brief The \c ExecutionSignature tag to get if you the current iteration is on a boundary.
  ///
  /// A \c WorkletPointNeighborhood operates by iterating over all points using
  /// a defined neighborhood. This \c ExecutionSignature tag provides different
  /// types when you are on or off a boundary, allowing for separate code paths
  /// just for handling boundaries.
  ///
  /// This is important as when you are on a boundary the neighboordhood will
  /// contain empty values for a certain subset of values
  struct OnBoundary : vtkm::exec::arg::OnBoundary
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
  template <typename TypeList = AllTypes>
  struct FieldIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
    using TransportTag = vtkm::cont::arg::TransportTagArrayIn;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for output point fields.
  ///
  /// This tag takes a template argument that is a type list tag that limits
  /// the possible value types in the array.
  ///
  template <typename TypeList = AllTypes>
  struct FieldOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
    using TransportTag = vtkm::cont::arg::TransportTagArrayOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectOut;
  };

  /// \brief A control signature tag for input-output (in-place) point fields.
  ///
  /// This tag takes a template argument that is a type list tag that limits
  /// the possible value types in the array.
  ///
  template <typename TypeList = AllTypes>
  struct FieldInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
    using TransportTag = vtkm::cont::arg::TransportTagArrayInOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectInOut;
  };

  /// \brief A control signature tag for input connectivity.
  ///
  struct CellSetIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagCellSetStructured;
    using TransportTag = vtkm::cont::arg::TransportTagCellSetIn<vtkm::TopologyElementTagCell,
                                                                vtkm::TopologyElementTagPoint>;
    using FetchTag = vtkm::exec::arg::FetchTagCellSetIn;
  };
};

template <int Neighborhood_>
class WorkletPointNeighborhood : public WorkletPointNeighborhoodBase
{
public:
  static constexpr vtkm::IdComponent Neighborhood = Neighborhood_;

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
  template <typename TypeList = AllTypes>
  struct FieldInNeighborhood : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
    using TransportTag = vtkm::cont::arg::TransportTagArrayIn;
    using FetchTag = vtkm::exec::arg::FetchTagArrayNeighborhoodIn<Neighborhood>;
  };

  /// Point neighborhood worklets use the related thread indices class.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T,
            typename IndexType,
            typename OutToInArrayType,
            typename VisitArrayType,
            vtkm::IdComponent Dimension>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesPointNeighborhood<Neighborhood> GetThreadIndices(
    const IndexType& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const vtkm::exec::ConnectivityStructured<vtkm::TopologyElementTagCell,
                                             vtkm::TopologyElementTagPoint,
                                             Dimension>& inputDomain, //this should be explicitly
    const T& globalThreadIndexOffset = 0) const
  {
    return vtkm::exec::arg::ThreadIndicesPointNeighborhood<Neighborhood>(
      threadIndex, outToIn, visit, inputDomain, globalThreadIndexOffset);
  }
};


using WorkletPointNeighborhood3x3x3 = WorkletPointNeighborhood<1>;
using WorkletPointNeighborhood5x5x5 = WorkletPointNeighborhood<2>;
}
}

#endif
