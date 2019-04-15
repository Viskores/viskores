//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_BasicArg_h
#define vtk_m_exec_arg_BasicArg_h

#include <vtkm/Types.h>

#include <vtkm/exec/arg/AspectTagDefault.h>
#include <vtkm/exec/arg/ExecutionSignatureTagBase.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief The underlying tag for basic \c ExecutionSignature arguments.
///
/// The basic \c ExecutionSignature arguments of _1, _2, etc. are all
/// subclasses of \c BasicArg. They all make available the components of
/// this class.
///
template <vtkm::IdComponent ControlSignatureIndex>
struct BasicArg : vtkm::exec::arg::ExecutionSignatureTagBase
{
  static constexpr vtkm::IdComponent INDEX = ControlSignatureIndex;
  using AspectTag = vtkm::exec::arg::AspectTagDefault;
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_BasicArg_h
