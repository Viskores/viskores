//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef vtk_m_exec_arg_ThreadIndicesBasic3D_h
#define vtk_m_exec_arg_ThreadIndicesBasic3D_h

#include <vtkm/exec/arg/ThreadIndicesBasic.h>

namespace vtkm
{
namespace exec
{
namespace arg
{

/// \brief Container for 3D thread indices in a worklet invocation
///
/// During the execution of a worklet function in an execution environment
/// thread, VTK-m has to manage several indices. To simplify this management
/// and to provide a single place to store them (so that they do not have to be
/// recomputed), \c WorkletInvokeFunctor creates a \c ThreadIndices object.
/// This object gets passed to \c Fetch operations to help them load data.
///
///
class ThreadIndicesBasic3D : public vtkm::exec::arg::ThreadIndicesBasic
{
public:
  VTKM_EXEC
  ThreadIndicesBasic3D(const vtkm::Id3& threadIndex3D,
                       vtkm::Id threadIndex1D,
                       vtkm::Id inIndex,
                       vtkm::IdComponent visitIndex,
                       vtkm::Id outIndex)
    : ThreadIndicesBasic(threadIndex1D, inIndex, visitIndex, outIndex)
    , ThreadIndex3D(threadIndex3D)
  {
  }

  /// \brief The 3D index into the input domain.
  ///
  /// This index refers to the input element (array value, cell, etc.) that
  /// this thread is being invoked for. If the input domain has 2 or 3
  /// dimensional indexing, this result will preserve that. If the domain
  /// indexing is just one dimensional, the result will have the index in the
  /// first component with the remaining components set to 0.
  ///
  VTKM_EXEC
  vtkm::Id3 GetInputIndex3D() const { return this->ThreadIndex3D; }

private:
  vtkm::Id3 ThreadIndex3D;
};
}
}
} // namespace vtkm::exec::arg

#endif //vtk_m_exec_arg_ThreadIndicesBasic3D_h
