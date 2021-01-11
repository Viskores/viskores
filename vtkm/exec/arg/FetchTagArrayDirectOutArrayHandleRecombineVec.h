//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_FetchTagArrayDirectOutArrayHandleRecombineVec_h
#define vtk_m_exec_arg_FetchTagArrayDirectOutArrayHandleRecombineVec_h

#include <vtkm/exec/arg/FetchTagArrayDirectInOut.h>
#include <vtkm/exec/arg/FetchTagArrayDirectOut.h>

// The `Fetch` for direct array out breaks for `ArrayHandleRecombineVec` because the `Load`
// method attempts to create a `vtkm::internal::RecombineVec` with a default constructor,
// which does not exist. Instead, have the direct out `Fetch` behave like the direct in/out
// `Fetch`, which loads the initial value from the array. The actual load will not load the
// data but rather set up the portals in the returned object, which is necessary for the
// later `Store` to work anyway.

// This file is included from ArrayHandleRecombineVec.h

namespace vtkm
{
namespace exec
{
namespace arg
{

template <typename SourcePortalType>
struct Fetch<vtkm::exec::arg::FetchTagArrayDirectOut,
             vtkm::exec::arg::AspectTagDefault,
             vtkm::internal::ArrayPortalRecombineVec<SourcePortalType>>
  : Fetch<vtkm::exec::arg::FetchTagArrayDirectInOut,
          vtkm::exec::arg::AspectTagDefault,
          vtkm::internal::ArrayPortalRecombineVec<SourcePortalType>>
{
};

}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_FetchTagArrayDirectOutArrayHandleRecombineVec_h
