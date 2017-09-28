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
#ifndef vtk_m_worklet_WorkletMapTopology_h
#define vtk_m_worklet_WorkletMapTopology_h

#include <vtkm/worklet/internal/WorkletBase.h>

#include <vtkm/TopologyElementTag.h>
#include <vtkm/TypeListTag.h>

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
#include <vtkm/exec/arg/FromCount.h>
#include <vtkm/exec/arg/FromIndices.h>
#include <vtkm/exec/arg/ThreadIndicesTopologyMap.h>

namespace vtkm
{
namespace worklet
{

namespace detail
{

struct WorkletMapTopologyBase : vtkm::worklet::internal::WorkletBase
{
};

} // namespace detail

/// Base class for worklets that do a simple mapping of field arrays. All
/// inputs and outputs are on the same domain. That is, all the arrays are the
/// same size.
///
template <typename FromTopology, typename ToTopology>
class WorkletMapTopology : public detail::WorkletMapTopologyBase
{
public:
  using FromTopologyType = FromTopology;
  using ToTopologyType = ToTopology;

  /// \brief A control signature tag for input fields.
  ///
  /// This tag takes a template argument that is a type list tag that limits
  /// the possible value types in the array.
  ///
  template <typename TypeList = AllTypes>
  struct FieldInTo : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
    using TransportTag = vtkm::cont::arg::TransportTagTopologyFieldIn<ToTopologyType>;
    using FetchTag = vtkm::exec::arg::FetchTagArrayDirectIn;
  };

  /// \brief A control signature tag for input connectivity.
  ///
  /// This tag takes a template argument that is a type list tag that limits
  /// the possible value types in the array.
  ///
  template <typename TypeList = AllTypes>
  struct FieldInFrom : vtkm::cont::arg::ControlSignatureTagBase
  {
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagArray<TypeList>;
    using TransportTag = vtkm::cont::arg::TransportTagTopologyFieldIn<FromTopologyType>;
    using FetchTag = vtkm::exec::arg::FetchTagArrayTopologyMapIn;
  };

  /// \brief A control signature tag for output fields.
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

  /// \brief A control signature tag for input-output (in-place) fields.
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
    using TypeCheckTag = vtkm::cont::arg::TypeCheckTagCellSet;
    using TransportTag = vtkm::cont::arg::TransportTagCellSetIn<FromTopologyType, ToTopologyType>;
    using FetchTag = vtkm::exec::arg::FetchTagCellSetIn;
  };

  /// \brief An execution signature tag for getting the cell shape.
  ///
  struct CellShape : vtkm::exec::arg::CellShape
  {
  };

  /// \brief An execution signature tag to get the number of from elements.
  ///
  /// In a topology map, there are \em from and \em to topology elements
  /// specified. The scheduling occurs on the \em to elements, and for each \em
  /// to element there is some number of incident \em from elements that are
  /// accessible. This \c ExecutionSignature tag provides the number of these
  /// \em from elements that are accessible.
  ///
  struct FromCount : vtkm::exec::arg::FromCount
  {
  };

  /// \brief An execution signature tag to get the indices of from elements.
  ///
  /// In a topology map, there are \em from and \em to topology elements
  /// specified. The scheduling occurs on the \em to elements, and for each \em
  /// to element there is some number of incident \em from elements that are
  /// accessible. This \c ExecutionSignature tag provides the indices of these
  /// \em from elements that are accessible.
  ///
  struct FromIndices : vtkm::exec::arg::FromIndices
  {
  };

  /// Topology map worklets use topology map indices.
  ///
  VTKM_SUPPRESS_EXEC_WARNINGS
  template <typename T,
            typename OutToInArrayType,
            typename VisitArrayType,
            typename InputDomainType,
            typename G>
  VTKM_EXEC vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType> GetThreadIndices(
    const T& threadIndex,
    const OutToInArrayType& outToIn,
    const VisitArrayType& visit,
    const InputDomainType& connectivity,
    const G& globalThreadIndexOffset) const
  {
    return vtkm::exec::arg::ThreadIndicesTopologyMap<InputDomainType>(
      threadIndex, outToIn, visit, connectivity, globalThreadIndexOffset);
  }
};

/// Base class for worklets that map from Points to Cells.
///
class WorkletMapPointToCell
  : public WorkletMapTopology<vtkm::TopologyElementTagPoint, vtkm::TopologyElementTagCell>
{
public:
  template <typename TypeList = AllTypes>
  using FieldInPoint = FieldInFrom<TypeList>;

  template <typename TypeList = AllTypes>
  using FieldInCell = FieldInTo<TypeList>;

  template <typename TypeList = AllTypes>
  using FieldOutCell = FieldOut<TypeList>;

  template <typename TypeList = AllTypes>
  using FieldInOutCell = FieldInOut<TypeList>;

  using PointCount = FromCount;

  using PointIndices = FromIndices;
};

/// Base class for worklets that map from Cells to Points.
///
class WorkletMapCellToPoint
  : public WorkletMapTopology<vtkm::TopologyElementTagCell, vtkm::TopologyElementTagPoint>
{
public:
  template <typename TypeList = AllTypes>
  using FieldInCell = FieldInFrom<TypeList>;

  template <typename TypeList = AllTypes>
  using FieldInPoint = FieldInTo<TypeList>;

  template <typename TypeList = AllTypes>
  using FieldOutPoint = FieldOut<TypeList>;

  template <typename TypeList = AllTypes>
  using FieldInOutPoint = FieldInOut<TypeList>;

  using CellCount = FromCount;

  using CellIndices = FromIndices;
};
}
} // namespace vtkm::worklet

#endif //vtk_m_worklet_WorkletMapTopology_h
