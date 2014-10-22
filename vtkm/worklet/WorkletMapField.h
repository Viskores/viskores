//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2014 Sandia Corporation.
//  Copyright 2014 UT-Battelle, LLC.
//  Copyright 2014. Los Alamos National Security
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_WorkletMapField_h
#define vtk_m_worklet_WorkletMapField_h

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkm/cont/arg/TransportTagArrayIn.h>
#include <vtkm/cont/arg/TransportTagArrayOut.h>
#include <vtkm/cont/arg/TypeCheckTagArray.h>

#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>

namespace vtkm {
namespace worklet {

/// Base class for worklets that do a simple mapping of field arrays. All
/// inputs and outputs are on the same domain. That is, all the arrays are the
/// same size.
///
class WorkletMapField : public vtkm::worklet::internal::WorkletBase
{
public:
  /// A control signature tag for input fields.
  struct FieldIn {
    typedef vtkm::cont::arg::TypeCheckTagArray TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagArrayIn TransportTag;
    typedef vtkm::exec::arg::FetchTagArrayDirectIn FetchTag;
  };

  /// A control signature tag for output fields.
  struct FieldOut {
    typedef vtkm::cont::arg::TypeCheckTagArray TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagArrayOut TransportTag;
    typedef vtkm::exec::arg::FetchTagArrayDirectOut FetchTag;
  };
};

}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_WorkletMapField_h
