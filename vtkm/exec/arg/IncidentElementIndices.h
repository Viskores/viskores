//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_IncidentElementIndices_h
#define vtk_m_exec_arg_IncidentElementIndices_h

#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>
#include <vtkm/exec/arg/Fetch.h>
#include <vtkm/exec/arg/FetchExtrude.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief The \c ExecutionSignature tag to get the indices of visited elements.
///
/// In a topology map, there are \em visited and \em incident topology elements
/// specified. The scheduling occurs on the \em visited elements, and for each
/// \em visited element there is some number of incident \em incident elements
/// that are accessible. This \c ExecutionSignature tag provides the indices of
/// the \em incident elements that are incident to the current \em visited
/// element.
///
struct IncidentElementIndices : vtkm::exec::arg::ExecutionSignatureTagBase
{
  static constexpr vtkm::IdComponent INDEX = 1;
  using AspectTag = vtkm::exec::arg::AspectTagIncidentElementIndices;
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_IncidentElementIndices_h
