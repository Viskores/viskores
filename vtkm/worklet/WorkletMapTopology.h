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
//  Copyright 2014 Los Alamos National Security.
//
//  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
//  the U.S. Government retains certain rights in this software.
//
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//============================================================================
#ifndef vtk_m_worklet_WorkletMapTopology_h
#define vtk_m_worklet_WorkletMapTopology_h

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkm/TypeListTag.h>

#include <vtkm/cont/arg/ControlSignatureTagBase.h>
#include <vtkm/cont/arg/TransportTagArrayIn.h>
#include <vtkm/cont/arg/TransportTagArrayInOut.h>
#include <vtkm/cont/arg/TransportTagArrayOut.h>
#include <vtkm/cont/arg/TransportTagTopologyIn.h>
#include <vtkm/cont/arg/TypeCheckTagArray.h>
#include <vtkm/cont/arg/TypeCheckTagTopology.h>

#include <vtkm/exec/arg/CellShape.h>
#include <vtkm/exec/arg/FetchTagArrayDirectIn.h>
#include <vtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>
#include <vtkm/exec/arg/FetchTagTopologyIn.h>
#include <vtkm/exec/arg/FetchTagArrayTopologyMapIn.h>
#include <vtkm/exec/arg/FromCount.h>
#include <vtkm/exec/arg/FromIndices.h>

namespace vtkm {
namespace worklet {

/// Base class for worklets that do a simple mapping of field arrays. All
/// inputs and outputs are on the same domain. That is, all the arrays are the
/// same size.
///
/// TODO: Although the nomenclature of this class suggests it handles general
/// topological links, it really only handles point to cell operations for
/// now. This worklet needs to be templated to handle generic from/to topology
/// links. I also suggest having convenience subclasses for common (supported?)
/// link directions.
///
class WorkletMapTopology : public vtkm::worklet::internal::WorkletBase
{
public:
  /// \brief A control signature tag for input fields.
  ///
  /// This tag takes a template argument that is a type list tag that limits
  /// the possible value types in the array.
  ///
  template<typename TypeList = AllTypes>
  struct FieldInTo : vtkm::cont::arg::ControlSignatureTagBase {
    typedef vtkm::cont::arg::TypeCheckTagArray<TypeList> TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagArrayIn TransportTag;
    typedef vtkm::exec::arg::FetchTagArrayDirectIn FetchTag;
  };

  /// \brief A control signature tag for input connectivity.
  ///
  /// This tag takes a template argument that is a type list tag that limits
  /// the possible value types in the array.
  ///
  template<typename TypeList = AllTypes>
  struct FieldInFrom : vtkm::cont::arg::ControlSignatureTagBase {
    typedef vtkm::cont::arg::TypeCheckTagArray<TypeList> TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagArrayIn TransportTag;
    typedef vtkm::exec::arg::FetchTagArrayTopologyMapIn FetchTag;
  };

  /// \brief A control signature tag for input connectivity.
  ///
  struct TopologyIn : vtkm::cont::arg::ControlSignatureTagBase {
    typedef vtkm::cont::arg::TypeCheckTagTopology TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagTopologyIn TransportTag;
    typedef vtkm::exec::arg::FetchTagTopologyIn FetchTag;
  };

  /// \brief A control signature tag for output fields.
  ///
  /// This tag takes a template argument that is a type list tag that limits
  /// the possible value types in the array.
  ///
  template<typename TypeList = AllTypes>
  struct FieldOut : vtkm::cont::arg::ControlSignatureTagBase {
    typedef vtkm::cont::arg::TypeCheckTagArray<TypeList> TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagArrayOut TransportTag;
    typedef vtkm::exec::arg::FetchTagArrayDirectOut FetchTag;
  };

  /// \brief A control signature tag for input-output (in-place) fields.
  ///
  /// This tag takes a template argument that is a type list tag that limits
  /// the possible value types in the array.
  ///
  template<typename TypeList = AllTypes>
  struct FieldInOut : vtkm::cont::arg::ControlSignatureTagBase {
    typedef vtkm::cont::arg::TypeCheckTagArray<TypeList> TypeCheckTag;
    typedef vtkm::cont::arg::TransportTagArrayInOut TransportTag;
    typedef vtkm::exec::arg::FetchTagArrayDirectInOut FetchTag;
  };

  /// \brief An execution signature tag for getting the cell shape.
  ///
  struct CellShape : vtkm::exec::arg::CellShape {  };

  /// \brief An execution signature tag to get the number of from elements.
  ///
  /// In a topology map, there are \em from and \em to topology elements
  /// specified. The scheduling occurs on the \em to elements, and for each \em
  /// to element there is some number of incident \em from elements that are
  /// accessible. This \c ExecutionSignature tag provides the number of these
  /// \em from elements that are accessible.
  ///
  struct FromCount : vtkm::exec::arg::FromCount {  };

  /// \brief An execution signature tag to get the indices of from elements.
  ///
  /// In a topology map, there are \em from and \em to topology elements
  /// specified. The scheduling occurs on the \em to elements, and for each \em
  /// to element there is some number of incident \em from elements that are
  /// accessible. This \c ExecutionSignature tag provides the indices of these
  /// \em from elements that are accessible.
  ///
  struct FromIndices : vtkm::exec::arg::FromIndices {  };
};

}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_WorkletMapTopology_h
