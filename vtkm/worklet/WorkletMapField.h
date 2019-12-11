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
#include <vtkm/cont/arg/TypeCheckTagArray.h>

#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <vtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>

namespace vtkm
{
namespace worklet
{

template <typename WorkletType>
class DispatcherMapField;

/// Base class for worklets that do a simple mapping of field arrays. All
/// inputs and outputs are on the same domain. That is, all the arrays are the
/// same size.
///
class WorkletMapField : public vtkm::worklet::internal::WorkletBase
{
public:
  template <typename Worklet>
  using Dispatcher = vtkm::worklet::DispatcherMapField<Worklet>;


  /// \brief A control signature tag for input fields.
  ///
  /// This tag means that the field is read only.
  ///
  struct FieldIn : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagArrayIn;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for output fields.
  ///
  /// This tag means that the field is write only.
  ///
  struct FieldOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagArrayOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectOut;
  };

  /// \brief A control signature tag for input-output (in-place) fields.
  ///
  /// This tag means that the field is read and write.
  ///
  struct FieldInOut : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray;
    using TransportTag = vtkm::cont::arg::TransportTagArrayInOut;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectInOut;
  };
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_WorkletMapField_h
