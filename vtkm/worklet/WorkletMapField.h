//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_worklet_WorkletMapField_h
#define vtk_m_worklet_WorkletMapField_h

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/arg/TransportTagArrayIn.h>
#include <vtkm/cont/arg/TransportTagArrayInOut.h>
#include <vtkm/cont/arg/TransportTagArrayOut.h>
#include <vtkm/cont/arg/TypeCheckTagArrayIn.h>
#include <vtkm/cont/arg/TypeCheckTagArrayInOut.h>
#include <vtkm/cont/arg/TypeCheckTagArrayOut.h>

#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <vtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>

#include <vtkm/worklet/DispatcherMapField.h>

namespace vtkm
{
namespace worklet
{

/// @brief Base class for worklets that do a simple mapping of field arrays.
///
/// All inputs and outputs are on the same domain. That is, all the arrays are the
/// same size.
///
class WorkletMapField : public vtkm::worklet::internal::WorkletBase
{
public:
  template <typename Worklet>
  using Dispatcher = vtkm::worklet::DispatcherMapField<Worklet>;

  /// @defgroup WorkletMapFieldControlSigTags `ControlSignature` tags
  /// Tags that can be used in the `ControlSignature` of a `WorkletMapField`.
  /// @{

  /// @brief A control signature tag for input fields.
  ///
  /// A `FieldIn` argument expects a `vtkm::cont::ArrayHandle` in the associated
  /// parameter of the invoke. Each invocation of the worklet gets a single value
  /// out of this array.
  ///
  /// This tag means that the field is read only.
  ///
  /// The worklet's `InputDomain` can be set to a `FieldIn` argument. In this case,
  /// the input domain will be the size of the array.
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
  /// Although uncommon, it is possible to set the worklet's `InputDomain` to a
  /// `FieldOut` argument. If this is the case, then the `vtkm::cont::ArrayHandle`
  /// passed as the argument must be allocated before being passed to the invoke,
  /// and the input domain will be the size of the array.
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
  /// The worklet's `InputDomain` can be set to a `FieldInOut` argument. In
  /// this case, the input domain will be the size of the array.
  ///
  struct FieldInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArrayInOut;
    using TransportTag = vtkm::cont::arg::TransportTagArrayInOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectInOut;
  };

#ifdef VTKM_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

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
#endif

  /// @}

  /// @defgroup WorkletMapFieldExecutionSigTags `ExecutionSignature` tags
  /// Tags that can be used in the `ExecutionSignature` of a `WorkletMapField`.
  /// @{

#ifdef VTKM_DOXYGEN_ONLY
  // These redeclarations of superclass features are for documentation purposes only.

  /// @copydoc vtkm::placeholders::Arg
  struct _1 : vtkm::worklet::internal::WorkletBase::_1
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
#endif

  /// @}
};

}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_WorkletMapField_h
