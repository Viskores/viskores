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
  /// @brief The `ExecutionSignature` tag to query if the current iteration is inside the boundary.
  ///
  /// This `ExecutionSignature` tag provides a `vtkm::exec::BoundaryState` object that provides
  /// information about where the local neighborhood is in relationship to the full mesh. It allows
  /// you to query whether the neighborhood of the current worklet call is completely inside the
  /// bounds of the mesh or if it extends beyond the mesh. This is important as when you are on a
  /// boundary the neighboordhood will contain empty values for a certain subset of values, and in
  /// this case the values returned will depend on the boundary behavior.
  ///
  struct Boundary : vtkm::exec::arg::Boundary
  {
  };

  /// All worklets must define their scatter operation.
  using ScatterType = vtkm::worklet::ScatterIdentity;

  VTKM_DEPRECATED_SUPPRESS_BEGIN
  /// All neighborhood worklets must define their boundary type operation.
  /// The boundary type determines how loading on boundaries will work.
  using BoundaryType VTKM_DEPRECATED(2.2, "Never fully supported, so being removed.") =
    vtkm::worklet::BoundaryClamp;

  /// In addition to defining the boundary type, the worklet must produce the
  /// boundary condition. The default BoundaryClamp has no state, so just return an
  /// instance.
  /// Note: Currently only BoundaryClamp is implemented
  VTKM_DEPRECATED(2.2, "Never fully supported, so being removed.")
  VTKM_CONT BoundaryType GetBoundaryCondition() const { return BoundaryType(); }
  VTKM_DEPRECATED_SUPPRESS_END

  /// @brief A control signature tag for input fields.
  ///
  /// A `FieldIn` argument expects a `vtkm::cont::ArrayHandle` in the associated
  /// parameter of the invoke. Each invocation of the worklet gets a single value
  /// out of this array.
  ///
  /// This tag means that the field is read only.
  ///
  struct FieldIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayIn;
    using TransportTag = vtkm::cont::arg::TransportTagArrayIn;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// @brief A control signature tag for output fields.
  ///
  /// A `FieldOut` argument expects a `vtkm::cont::ArrayHandle` in the associated
  /// parameter of the invoke. The array is resized before scheduling begins, and
  /// each invocation of the worklet sets a single value in the array.
  ///
  /// This tag means that the field is write only.
  ///
  struct FieldOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayOut;
    using TransportTag = vtkm::cont::arg::TransportTagArrayOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectOut;
  };

  /// @brief A control signature tag for input-output (in-place) fields.
  ///
  /// A `FieldInOut` argument expects a `vtkm::cont::ArrayHandle` in the
  /// associated parameter of the invoke. Each invocation of the worklet gets a
  /// single value out of this array, which is replaced by the resulting value
  /// after the worklet completes.
  ///
  /// This tag means that the field is read and write.
  ///
  struct FieldInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayInOut;
    using TransportTag = vtkm::cont::arg::TransportTagArrayInOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectInOut;
  };

  /// @brief A control signature tag for input connectivity.
  ///
  /// This tag represents the cell set that defines the collection of points the
  /// map will operate on. A `CellSetIn` argument expects a `vtkm::cont::CellSetStructured`
  /// object in the associated parameter of the invoke.
  ///
  /// There must be exactly one `CellSetIn` argument, and the worklet's `InputDomain` must
  /// be set to this argument.
  struct CellSetIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagCellSetStructured;
    using TransportTag = vtkm::cont::arg::TransportTagCellSetIn<vtkm::TopologyElementTagPoint,
                                                                vtkm::TopologyElementTagCell>;
    using FetchTag = vtkm::exec::arg::FetchTagCellSetIn;
  };

  /// @brief A control signature tag for neighborhood input values.
  ///
  /// A neighborhood worklet operates by allowing access to a adjacent element
  /// values in a NxNxN patch called a neighborhood.
  /// No matter the size of the neighborhood it is symmetric across its center
  /// in each axis, and the current point value will be at the center
  /// For example a 3x3x3 neighborhood would have local indices ranging from -1 to 1
  /// in each dimension.
  ///
  /// This tag specifies a `vtkm::cont::ArrayHandle` object that holds the values. It is
  /// an input array with entries for each element.
  ///
  /// What differentiates `FieldInNeighborhood` from `FieldIn` is that `FieldInNeighborhood`
  /// allows the worklet function to access the field value at the element it is visiting and
  /// the field values in the neighborhood around it. Thus, instead of getting a single value
  /// out of the array, each invocation of the worklet gets a `vtkm::exec::FieldNeighborhood`
  /// object. These objects allow retrieval of field values using indices relative to the
  /// visited element.
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
